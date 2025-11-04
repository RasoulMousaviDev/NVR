/*
 * modular_camera_scanner.c
 *
 * Modular ONVIF/RTSP camera scanner for local network.
 * - Stores only cameras responding to RTSP
 * - Prevents duplicate IP entries (UUID ignored if same IP)
 * - Tracks active/inactive cameras
 * - SQLite database with username/password columns
 * - Logs only when device appears or disappears
 *
 * Build:
 *   gcc -o modular_camera_scanner modular_camera_scanner.c -lsqlite3 -lpthread
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
#define DISCOVER_INTERVAL 3
#define CLEANUP_INTERVAL 6
#define OFFLINE_THRESHOLD 10
#define RTSP_PORT 554
#define DB_FILE "cameras.db"

/* ------------------------------ Utils ------------------------------ */
static long now_seconds(void)
{
    return (long)time(NULL);
}

static void now_iso8601(char *buf, size_t buflen)
{
    struct tm gm;
    time_t t = time(NULL);
    gmtime_r(&t, &gm);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &gm);
}

/* ------------------------------ Database --------------------------- */
static sqlite3 *db = NULL;

static void db_init(const char *path)
{
    if (sqlite3_open(path, &db) != SQLITE_OK)
    {
        fprintf(stderr, "sqlite3_open: %s\n", sqlite3_errmsg(db));
        exit(1);
    }
    const char *schema =
        "CREATE TABLE IF NOT EXISTS cameras("
        " ip TEXT PRIMARY KEY,"
        " uuid TEXT,"
        " active INTEGER,"
        " username TEXT,"
        " password TEXT,"
        " last_seen INTEGER"
        ");";
    char *err = NULL;
    if (sqlite3_exec(db, schema, NULL, NULL, &err) != SQLITE_OK)
    {
        fprintf(stderr, "sqlite3_exec: %s\n", err);
        sqlite3_free(err);
        exit(1);
    }
}

static void db_upsert(const char *ip, const char *uuid)
{
    const char *sql =
        "INSERT INTO cameras(ip, uuid, active, username, password, last_seen)"
        " VALUES(?, ?, 1, NULL, NULL, ?)"
        " ON CONFLICT(ip) DO UPDATE SET active=1, last_seen=excluded.last_seen;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
    {
        fprintf(stderr, "db prepare: %s\n", sqlite3_errmsg(db));
        return;
    }
    sqlite3_bind_text(stmt, 1, ip, -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, uuid, -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, now_seconds());
    if (sqlite3_step(stmt) != SQLITE_DONE)
        fprintf(stderr, "db step: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
}

static void db_mark_offline(long threshold)
{
    const char *sql =
        "UPDATE cameras SET active=0 WHERE last_seen < ? AND active=1;";
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK)
        return;
    sqlite3_bind_int64(stmt, 1, now_seconds() - threshold);
    if (sqlite3_step(stmt) != SQLITE_DONE)
        fprintf(stderr, "db cleanup step: %s\n", sqlite3_errmsg(db));
    sqlite3_finalize(stmt);
}

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
        return 1; // treat 401 as valid RTSP
    return 0;
}

/* ------------------------- WS-Discovery ---------------------------- */
typedef void (*discover_handler_t)(const char *ip, const char *uuid);

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
    snprintf(uuid, sizeof(uuid), "%ld-%d", now_seconds(), getpid());

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

        char uuid_from[128] = {0};
        const char *ud = strstr(buf, "uuid:");
        if (ud)
        {
            ud += 5;
            int i = 0;
            while (ud[i] && ud[i] != '<' && i < (int)sizeof(uuid_from) - 1)
            {
                uuid_from[i] = ud[i];
                i++;
            }
        }

        handler(ipbuf, uuid_from[0] ? uuid_from : uuid);
    }

    close(sock);
}

/* --------------------------- Discovery Handler ------------------------ */
static void handle_probe(const char *ip, const char *uuid)
{
    static char blacklist[1024][INET_ADDRSTRLEN];
    static int bl_count = 0;

    // skip blacklisted IPs
    for (int i = 0; i < bl_count; i++)
    {
        if (strcmp(blacklist[i], ip) == 0)
            return;
    }

    if (!rtsp_check(ip))
    {
        if (bl_count < 1024)
        {
            strncpy(blacklist[bl_count++], ip, INET_ADDRSTRLEN);
        }
        return;
    }

    db_upsert(ip, uuid);
}

/* ------------------------- Background Threads ------------------------ */
static volatile int running = 1;

static void *discover_thread(void *arg)
{
    (void)arg;
    while (running)
    {
        do_ws_discover_round(handle_probe);
        sleep(DISCOVER_INTERVAL);
    }
    return NULL;
}

static void *cleanup_thread(void *arg)
{
    (void)arg;
    while (running)
    {
        sleep(CLEANUP_INTERVAL);
        db_mark_offline(OFFLINE_THRESHOLD);
    }
    return NULL;
}

/* ------------------------------ Main ------------------------------- */
int main(void)
{
    db_init(DB_FILE);

    pthread_t dthr, cthr;
    pthread_create(&dthr, NULL, discover_thread, NULL);
    pthread_create(&cthr, NULL, cleanup_thread, NULL);

    printf("Camera scanner running.");
    while (1)
        pause();

    running = 0;
    pthread_join(dthr, NULL);
    pthread_join(cthr, NULL);
    sqlite3_close(db);

    return 0;
}
