#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define FILE_PATH "cameras.txt"

// Send WS-Discovery probe and capture response IP
void send_ws_discovery(char *buffer, int *size)
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return;

    int ttl = 4;
    setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(3702);
    inet_pton(AF_INET, "239.255.255.250", &dest.sin_addr);

    char uuid[64];
    snprintf(uuid, sizeof(uuid), "%ld-%d", (long)time(NULL), getpid());

    const char *probe_template =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        "<e:Envelope xmlns:e=\"http://www.w3.org/2003/05/soap-envelope\""
        " xmlns:w=\"http://schemas.xmlsoap.org/ws/2004/08/addressing\""
        " xmlns:d=\"http://schemas.xmlsoap.org/ws/2005/04/discovery\">"
        "<e:Header><w:MessageID>uuid:%s</w:MessageID>"
        "<w:To>urn:schemas-xmlsoap-org:ws:2005/04/discovery</w:To>"
        "<w:Action>http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe</w:Action></e:Header>"
        "<e:Body><d:Probe/></e:Body></e:Envelope>";

    char msg[1024];
    snprintf(msg, sizeof(msg), probe_template, uuid);

    struct sockaddr_in bindaddr = {0};
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(3702);
    bindaddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) < 0)
    {
        close(sock);
        return;
    }

    sendto(sock, msg, strlen(msg), 0, (struct sockaddr *)&dest, sizeof(dest));

    struct timeval tv = {2, 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);

    while (1)
    {
        int r = recvfrom(sock, buffer, 4096, 0, (struct sockaddr *)&from, &fromlen);
        if (r < 0)
            break;

        buffer[r] = '\0';
        char ipbuf[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &from.sin_addr, ipbuf, sizeof(ipbuf));
        strcpy(buffer + r, ipbuf);
        *size = r;
    }
    close(sock);
}

// Check RTSP availability
int check_rtsp(const char *ip)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(554);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    int ok = (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);
    close(sock);
    return ok;
}

// ONVIF device info fetch placeholder
void extract_tag(const char *xml, const char *tag, char *out, int outsz)
{
    char open[64], close[64];
    snprintf(open, sizeof(open), "<%s>", tag);
    snprintf(close, sizeof(close), "</%s>", tag);

    char *start = strstr(xml, open);
    char *end = strstr(xml, close);

    if (!start || !end || end <= start)
    {
        out[0] = 0;
        return;
    }

    start += strlen(open);
    int len = end - start;
    if (len >= outsz)
        len = outsz - 1;

    strncpy(out, start, len);
    out[len] = 0;
}

int http_post(const char *ip, const char *body, char *response, int maxlen)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(80);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return -1;
    }

    char req[2048];
    snprintf(req, sizeof(req),
             "POST /onvif/device_service HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Content-Type: application/soap+xml; charset=utf-8\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             ip, strlen(body), body);

    send(sock, req, strlen(req), 0);

    int total = 0;
    while (1)
    {
        int r = recv(sock, response + total, maxlen - total - 1, 0);
        if (r <= 0)
            break;
        total += r;
        if (total >= maxlen - 1)
            break;
    }

    response[total] = 0;
    close(sock);
    return total;
}

int http_post_port(const char *ip, int port, const char *body, char *response, int maxlen)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port); // پورت واقعی
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        close(sock);
        return -1;
    }

    char req[2048];
    snprintf(req, sizeof(req),
             "POST /onvif/device_service HTTP/1.1\r\n"
             "Host: %s:%d\r\n"
             "Content-Type: application/soap+xml; charset=utf-8\r\n"
             "Content-Length: %d\r\n"
             "Connection: close\r\n\r\n"
             "%s",
             ip, port, strlen(body), body);

    send(sock, req, strlen(req), 0);

    int total = 0;
    while (1)
    {
        int r = recv(sock, response + total, maxlen - total - 1, 0);
        if (r <= 0)
            break;
        total += r;
        if (total >= maxlen - 1)
            break;
    }

    response[total] = 0;
    close(sock);
    return total;
}

void fetch_onvif(const char *ip, int port, char *man, char *model, char *fw,
                 char *sn, char *hw, int *audio)
{
    const char *soap =
        "<?xml version=\"1.0\"?>"
        "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\">"
        "<s:Body>"
        "<GetDeviceInformation xmlns=\"http://www.onvif.org/ver10/device/wsdl\"/>"
        "</s:Body></s:Envelope>";

    char buf[16384];
    int r = http_post_port(ip, port, soap, buf, sizeof(buf));
    if (r <= 0)
    {
        strcpy(man, "unknown");
        strcpy(model, "unknown");
        strcpy(fw, "unknown");
        strcpy(sn, "unknown");
        strcpy(hw, "unknown");
        *audio = 0;
        return;
    }

    char *xml = strstr(buf, "\r\n\r\n");
    xml = xml ? xml + 4 : buf;

    extract_tag(xml, "tds:Manufacturer", man, 128);
    extract_tag(xml, "tds:Model", model, 128);
    extract_tag(xml, "tds:FirmwareVersion", fw, 128);
    extract_tag(xml, "tds:SerialNumber", sn, 128);
    extract_tag(xml, "tds:HardwareId", hw, 128);

    *audio = strstr(xml, "Audio") ? 1 : 0;
}

// Ensure file exists
void ensure_file(char *file_path)
{
    FILE *fp = fopen(file_path, "a+");
    fclose(fp);
}

// Find existing camera by IP
int find_camera(FILE *fp, const char *ip, long *pos)
{
    char line[1024];
    int id = 1;

    rewind(fp);
    while (fgets(line, sizeof(line), fp))
    {
        char *p = strstr(line, "ip=");
        if (p && strncmp(p + 3, ip, strlen(ip)) == 0)
        {
            *pos = ftell(fp) - strlen(line);
            return id;
        }
        id++;
    }

    return 0;
}

// Set connect=0 for all cameras
void reset_connect_flags(FILE *fp)
{
    FILE *tmp = fopen("reset.txt", "w");
    char line[1024];

    rewind(fp);
    while (fgets(line, sizeof(line), fp))
    {
        char *p = strstr(line, "connect=");
        if (p)
        {
            char *amp = strchr(p, '&');
            if (amp)
                *amp = '\0';
            sprintf(p, "connect=0%s", amp ? amp + 1 : "");
        }
        fputs(line, tmp);
    }

    fclose(tmp);
    remove(FILE_PATH);
    rename("reset.tmp", FILE_PATH);
}

// Add new camera
void write_camera(FILE *fp, int id, const char *ip,
                  const char *man, const char *model,
                  const char *fw, const char *sn, const char *hw,
                  int audio)
{
    char line[2048];

    sprintf(line,
            "id=%d&ip=%s&manufacturer=%s&model=%s&firmware_version=%s&serial_number=%s&hardware_id=%s"
            "&username=&password=&record=0&audio=%d&image_quality=medium&audio_quality=%s&duration=60"
            "&connect=1",
            id,
            ip, man, model, fw, sn, hw, audio,
            audio ? "medium" : "off");

    fseek(fp, 0, SEEK_END);
    fputs(line, fp);
    fflush(fp);
}

// Update connect=1 for an existing camera
void update_connect(FILE *fp, long pos)
{
    FILE *tmp = fopen("update.txt", "a+");
    char line[1024];

    rewind(fp);
    long cur = 0;

    while (fgets(line, sizeof(line), fp))
    {
        long start = cur;
        cur = ftell(fp);

        if (start == pos)
        {
            char *p = strstr(line, "connect=");
            if (p)
            {
                char *amp = strchr(p, '&');
                if (amp)
                    *amp = '\0';
                sprintf(p, "connect=1%s", amp ? amp + 1 : "");
            }
        }

        fputs(line, tmp);
    }

    fclose(tmp);
    remove(FILE_PATH);
    rename("update.txt", FILE_PATH);
}

int main()
{
    char *method = getenv("REQUEST_METHOD");

    if (strcmp(method, "POST") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n");
        printf("Content-Type: text/plain\r\n\r\n");
        printf("Method Not Allowed\n");
        return 1;
    }

    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return 1;
    exe_path[len] = '\0';

    char *last_slash = strrchr(exe_path, '/');
    if (last_slash)
        *last_slash = '\0';

    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", exe_path, FILE_PATH);

    ensure_file(file_path);
    FILE *fp = fopen(file_path, "a+");

    // reset_connect_flags(fp);

    char buffer[4096];
    int size = 5;

    strcpy(buffer, "Probe");
    send_ws_discovery(buffer, &size);

    if (size > 0)
    {
        char ip[64];
        strcpy(ip, buffer + size);
        if (check_rtsp(ip))
        {
            char man[64], model[64], fw[64], sn[64], hw[64];
            int audio, port = 8899;

            fetch_onvif(ip, port, man, model, fw, sn, hw, &audio);

            long pos;
            int id = find_camera(fp, ip, &pos);

            if (id)
            {
                update_connect(fp, pos);
            }
            else
            {
                id++;
                write_camera(fp, id, ip, man, model, fw, sn, hw, audio);
            }
        }
    }

    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");
    printf("{\"ok\": true}\n");

    fclose(fp);
    return 0;
}
