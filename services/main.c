#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <net/if.h>

#define MCAST_ADDR "239.255.255.250"
#define MCAST_PORT 3702
#define PROBE_INTERVAL 15 /* seconds between sending probes */
#define GONE_TIMEOUT 90   /* seconds since last seen to consider device gone */
#define RECV_BUF 8192
#define MAX_IFACES 32

struct device
{
    char uuid[256];
    char ip[64];
    char xaddrs[512];
    time_t last_seen;
    struct device *next;
};

static struct device *devices = NULL;

/* store IPv4 addresses for interfaces we will use */
struct iface_addr
{
    char name[IF_NAMESIZE];
    struct in_addr addr;
};

static struct iface_addr ifaces[MAX_IFACES];
static int if_count = 0;

static void now_iso8601(char *buf, size_t buflen)
{
    time_t t = time(NULL);
    struct tm gm;
    gmtime_r(&t, &gm);
    strftime(buf, buflen, "%Y-%m-%dT%H:%M:%SZ", &gm);
}

static void log_seen(const char *uuid, const char *ip, const char *xaddrs)
{
    char ts[64];
    now_iso8601(ts, sizeof(ts));
    printf("{\"ts\":\"%s\",\"event\":\"seen\",\"uuid\":\"%s\",\"ip\":\"%s\",\"xaddrs\":\"%s\"}\n", ts, uuid, ip, xaddrs ? xaddrs : "");
    fflush(stdout);
}

static void log_gone(const char *uuid, time_t last_seen)
{
    char ts[64], ls[64];
    now_iso8601(ts, sizeof(ts));
    struct tm gm;
    gmtime_r(&last_seen, &gm);
    strftime(ls, sizeof(ls), "%Y-%m-%dT%H:%M:%SZ", &gm);
    printf("{\"ts\":\"%s\",\"event\":\"gone\",\"uuid\":\"%s\",\"last_seen\":\"%s\"}\n", ts, uuid, ls);
    fflush(stdout);
}

/* naive XML extraction helpers: find text between <tag> and </tag> */
static int extract_tag(const char *xml, const char *tag, char *out, size_t outlen)
{
    char open[64], close[64];
    snprintf(open, sizeof(open), "<%s>", tag);
    snprintf(close, sizeof(close), "</%s>", tag);
    const char *p = strstr(xml, open);
    if (!p)
        return 0;
    p += strlen(open);
    const char *q = strstr(p, close);
    if (!q)
        return 0;
    size_t len = q - p;
    if (len >= outlen)
        len = outlen - 1;
    memcpy(out, p, len);
    out[len] = '\0';
    return 1;
}

static int extract_any_address(const char *xml, char *out, size_t outlen)
{
    const char *c = xml;
    while ((c = strchr(c, '<')) != NULL)
    {
        c++;
        if (*c == '/')
        {
            c++;
            continue;
        }
        while (*c == ' ' || *c == '	')
            c++;
        char token[64] = {0};
        int i = 0;
        while (*c && *c != '>' && *c != ' ' && i < 63)
        {
            token[i++] = *c++;
        }
        token[i] = 0;
        if (strstr(token, "Address") || strstr(token, "EndpointReference"))
        {
            const char *gt = strchr(c, '>');
            if (!gt)
                return 0;
            const char *endtag = strstr(gt + 1, "</");
            if (!endtag)
                return 0;
            size_t len = endtag - (gt + 1);
            if (len >= outlen)
                len = outlen - 1;
            memcpy(out, gt + 1, len);
            out[len] = 0;
            return 1;
        }
    }
    return 0;
}

static int extract_xaddrs(const char *xml, char *out, size_t outlen)
{
    const char *p = strstr(xml, "XAddrs");
    if (!p)
        return 0;
    const char *gt = strchr(p, '>');
    if (!gt)
        return 0;
    const char *end = strstr(gt + 1, "</");
    if (!end)
        return 0;
    size_t len = end - (gt + 1);
    if (len >= outlen)
        len = outlen - 1;
    memcpy(out, gt + 1, len);
    out[len] = 0;
    return 1;
}

static struct device *find_device(const char *uuid)
{
    struct device *d = devices;
    while (d)
    {
        if (strcmp(d->uuid, uuid) == 0)
            return d;
        d = d->next;
    }
    return NULL;
}

static struct device *create_device(const char *uuid)
{
    struct device *d = calloc(1, sizeof(*d));
    if (!d)
        return NULL;
    strncpy(d->uuid, uuid, sizeof(d->uuid) - 1);
    d->last_seen = time(NULL);
    d->next = devices;
    devices = d;
    return d;
}

static void touch_device(struct device *d, const char *ip, const char *xaddrs)
{
    if (ip)
        strncpy(d->ip, ip, sizeof(d->ip) - 1);
    if (xaddrs)
        strncpy(d->xaddrs, xaddrs, sizeof(d->xaddrs) - 1);
    d->last_seen = time(NULL);
}

static void cleanup_devices()
{
    struct device **pp = &devices;
    time_t now = time(NULL);
    while (*pp)
    {
        struct device *d = *pp;
        if (now - d->last_seen > GONE_TIMEOUT)
        {
            log_gone(d->uuid, d->last_seen);
            *pp = d->next;
            free(d);
        }
        else
        {
            pp = &(*pp)->next;
        }
    }
}

/* enumerate IPv4 interfaces (non-loopback, UP) and collect addresses */
static void enum_ifaces(void)
{
    struct ifaddrs *ifap, *ifa;
    if_count = 0;
    if (getifaddrs(&ifap) != 0)
        return;
    for (ifa = ifap; ifa; ifa = ifa->ifa_next)
    {
        if (!ifa->ifa_addr)
            continue;
        if (ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;
        if (if_count >= MAX_IFACES)
            break;
        struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
        strncpy(ifaces[if_count].name, ifa->ifa_name, IF_NAMESIZE - 1);
        ifaces[if_count].addr = sin->sin_addr;
        if_count++;
    }
    freeifaddrs(ifap);
}

/* join multicast group on each interface we've enumerated */
static void join_multicast_on_ifaces(int sock)
{
    for (int i = 0; i < if_count; i++)
    {
        struct ip_mreq mreq;
        inet_pton(AF_INET, MCAST_ADDR, &mreq.imr_multiaddr);
        mreq.imr_interface = ifaces[i].addr;
        if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        {
            fprintf(stderr, "warning: IP_ADD_MEMBERSHIP on %s (%s) failed: %s", ifaces[i].name,
                    inet_ntoa(ifaces[i].addr), strerror(errno));
        }
        else
        {
            fprintf(stderr, "joined multicast on %s (%s)\n", ifaces[i].name, inet_ntoa(ifaces[i].addr));
        }
    }
}

/* send probe on a specific interface by setting IP_MULTICAST_IF */
static int send_probe_on_iface(int sock, struct in_addr ifaddr)
{
    char msgid[64];
    snprintf(msgid, sizeof(msgid), "%08x-%04x-%04x-%04x-%012x",
             rand(), rand() & 0xffff, rand() & 0xffff, rand() & 0xffff, rand());
    char probe[1024];
    snprintf(probe, sizeof(probe),
             "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
             "<e:Envelope xmlns:e=\"http://www.w3.org/2003/05/soap-envelope\""
             " xmlns:w=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
             " xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
             "<e:Header><w:MessageID>uuid:%s</w:MessageID>"
             "<w:To>urn:schemas-xmlsoap-org:ws:2005:04:discovery</w:To>"
             "<w:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</w:Action></e:Header>"
             "<e:Body><d:Probe/></e:Body></e:Envelope>",
             msgid);

    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_IF, &ifaddr, sizeof(ifaddr)) < 0)
    {
        fprintf(stderr, "warning: IP_MULTICAST_IF %s failed: %s", inet_ntoa(ifaddr), strerror(errno));
        /* continue trying to send anyway */
    }

    struct sockaddr_in mcast;
    memset(&mcast, 0, sizeof(mcast));
    mcast.sin_family = AF_INET;
    mcast.sin_port = htons(MCAST_PORT);
    inet_pton(AF_INET, MCAST_ADDR, &mcast.sin_addr);

    ssize_t sent = sendto(sock, probe, strlen(probe), 0, (struct sockaddr *)&mcast, sizeof(mcast));
    if (sent < 0)
    {
        fprintf(stderr, "sendto failed on %s: %s", inet_ntoa(ifaddr), strerror(errno));
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    int sock;
    struct sockaddr_in addr;
    char buf[RECV_BUF];

    srand(time(NULL) ^ getpid());

    /* enumerate interfaces first */
    enum_ifaces();
    if (if_count == 0)
    {
        fprintf(stderr, "no usable IPv4 interfaces found");
        return 1;
    }

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR");
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(MCAST_PORT);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        close(sock);
        return 1;
    }

    join_multicast_on_ifaces(sock);

    int rcvbuf = 256 * 1024;
    setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &rcvbuf, sizeof(rcvbuf));

    time_t last_probe = 0;

    while (1)
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock, &rfds);
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        int rv = select(sock + 1, &rfds, NULL, NULL, &tv);
        if (rv < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            perror("select");
            break;
        }
        if (rv > 0 && FD_ISSET(sock, &rfds))
        {
            struct sockaddr_in from;
            socklen_t fromlen = sizeof(from);
            ssize_t len = recvfrom(sock, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&from, &fromlen);
            if (len > 0)
            {
                buf[len] = 0;
                char uuid[256] = {0};
                char xaddrs[512] = {0};
                if (!extract_any_address(buf, uuid, sizeof(uuid)))
                {
                    extract_tag(buf, "Address", uuid, sizeof(uuid));
                }
                extract_xaddrs(buf, xaddrs, sizeof(xaddrs));
                char ipstr[64];
                inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));

                if (uuid[0])
                {
                    struct device *d = find_device(uuid);
                    if (!d)
                    {
                        d = create_device(uuid);
                        if (d)
                            log_seen(uuid, ipstr, xaddrs);
                    }
                    if (d)
                        touch_device(d, ipstr, xaddrs);
                }
                else
                {
                    char fake_uuid[128];
                    snprintf(fake_uuid, sizeof(fake_uuid), "ip:%s", ipstr);
                    struct device *d = find_device(fake_uuid);
                    if (!d)
                    {
                        d = create_device(fake_uuid);
                        if (d)
                            log_seen(fake_uuid, ipstr, xaddrs);
                    }
                    if (d)
                        touch_device(d, ipstr, xaddrs);
                }
            }
        }

        time_t now = time(NULL);
        if (now - last_probe >= PROBE_INTERVAL)
        {
            /* send a probe on each interface */
            for (int i = 0; i < if_count; i++)
            {
                send_probe_on_iface(sock, ifaces[i].addr);
            }
            last_probe = now;
        }

        cleanup_devices();
    }

    close(sock);
    return 0;
}
