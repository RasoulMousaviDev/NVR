/*
 * modular_camera_scanner.c
 *
 * Modular ONVIF/RTSP camera scanner for local network.
 * - Stores only cameras responding to RTSP
 * - Prevents duplicate IP entries (UUID ignored if same IP)
 *
 * Build:
 *   gcc -o scan scan.c
 */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sqlite3.h>
#include <ctype.h>

/* ------------------------------ Config ------------------------------ */
#define WS_DISCOVER_ADDR "239.255.255.250"
#define WS_DISCOVER_PORT 3702
#define RTSP_PORT 554

/* ------------------------------ RTSP Check --------------------------- */

static int rtsp_check(const char *ip)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return 0;

    struct sockaddr_in sa;
    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(RTSP_PORT);
    if (inet_pton(AF_INET, ip, &sa.sin_addr) <= 0)
    {
        close(sock);
        return 0;
    }

    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    if (connect(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0)
    {
        close(sock);
        return 0;
    }

    char req[128];
    snprintf(req, sizeof(req), "OPTIONS rtsp://%s/ RTSP/1.0\r\nCSeq:1\r\n\r\n", ip);
    if (send(sock, req, strlen(req), 0) < 0)
    {
        close(sock);
        return 0;
    }

    char buf[256];
    int r = recv(sock, buf, sizeof(buf) - 1, 0);
    if (r <= 0)
    {
        close(sock);
        return 0;
    }
    buf[r] = '\0';
    close(sock);

    if (strstr(buf, "RTSP/1.0") || strstr(buf, "401"))
        if(strstr(buf, "audio"))
            return 2;
        return 1; // treat 401 as valid RTSP
    return 0;
}

/* ------------------------- WS-Discovery ---------------------------- */
typedef void (*discover_handler_t)(const char *ip);

static const char *probe_template =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
    "<e:Envelope xmlns:e=\"http://www.w3.org/2003/05/soap-envelope\""
    " xmlns:w=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
    " xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
    "<e:Header><w:MessageID>uuid:%s</w:MessageID>"
    "<w:To>urn:schemas-xmlsoap-org:ws:2005/04/discovery</w:To>"
    "<w:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</w:Action></e:Header>"
    "<e:Body><d:Probe/></e:Body></e:Envelope>";

static void do_ws_discover_round(discover_handler_t handler)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    int ttl = 4;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(WS_DISCOVER_PORT);
    inet_pton(AF_INET, WS_DISCOVER_ADDR, &dest.sin_addr);

    char uuid[64];
    snprintf(uuid, sizeof(uuid), "%ld-%d", (long)time(NULL), getpid());

    char msg[1024];
    snprintf(msg, sizeof(msg), probe_template, uuid);

    // bind socket to INADDR_ANY on MCAST port so responses come back correctly
    struct sockaddr_in bindaddr = {0};
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(WS_DISCOVER_PORT);
    bindaddr.sin_addr.s_addr = INADDR_ANY;
    if (bind(sock, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0)
    {
        perror("bind sock");
        close(sock);
        return;
    }

    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));

    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    char buf[4096];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    while (1)
    {
        int r = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);
        if (r < 0)
            break;

        buf[r] = '\0';
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));

        handler(ipbuf);
    }

    close(sock);
}

/* --------------------------- Discovery Handler ------------------------ */
static void handle_probe(const char *ip)
{
    static char blacklist[1024][INET_ADDRSTRLEN];
    static int bl_count = 0;

    // skip blacklisted IPs
    for (int i = 0; i < bl_count; i++)
    {
        if (strcmp(blacklist[i], ip) == 0)
            return;
    }

    int res = rtsp_check(ip);
    if (!res)
    {
        if (bl_count < 1024)
        {
            strncpy(blacklist[bl_count++], ip, INET_ADDRSTRLEN);
        }
        return;
    }

    printf("%s %d\n", ip, res - 1);
}

/* ------------------------------ Main ------------------------------- */
int main(void)
{
    do_ws_discover_round(handle_probe);

    return 0;
}
