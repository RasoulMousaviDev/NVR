#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <curl/curl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define FILE_PATH "cameras.txt"
#define MAX_BODY 65536
#define ONVIF_PORT 8899

int find_camera(FILE *fp, const char *id, char *ip)
{
    char line[4096];
    while (fgets(line, sizeof(line), fp))
    {
        char *idpos = strstr(line, "id=");
        if (!idpos)
            continue;

        char idbuf[64];
        if (sscanf(idpos, "id=%63[^&]", idbuf) == 1)
        {
            if (strcmp(idbuf, id) == 0)
            {
                char *ippos = strstr(line, "ip=");
                if (!ippos)
                {
                    fclose(fp);
                    return 0;
                }

                if (sscanf(ippos, "ip=%255[^&]", ip) == 1)
                {
                    fclose(fp);
                    return 1;
                }
            }
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "POST") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }

    if (argc != 2)
    {
        printf("Status: 400 Bad Request\r\n\r\n");
        return 1;
    }

    char *id = argv[1];
    int content_length = atoi(getenv("CONTENT_LENGTH"));
    char *body = malloc(content_length + 1);
    fread(body, 1, content_length, stdin);
    body[content_length] = '\0';

    char username[256] = {0}, password[256] = {0};
    char *p;

    if ((p = strstr(body, "username=")))
        sscanf(p, "username=%127[^&]", username);
    if ((p = strstr(body, "password=")))
        sscanf(p, "password=%127[^&]", password);

    free(body);

    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return 1;
    exe_path[len] = '\0';

    p = strstr(exe_path, "/[id]/credentials");
    if (p)
        *p = '\0';

    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", exe_path, FILE_PATH);

    FILE *fp = fopen(file_path, "r+");
    if (!fp)
    {
        printf("Status: 500 Internal Server Error\r\n\r\n file not found");
        return 0;
    }

    printf("Status: 200 OK\r\n\r\n");
    char ip[64] = {0};
    find_camera(fp, id, ip);

    char cmd[512];
    sprintf(cmd, "/mnt/mmcblk0p1/ffmpeg -rtsp_transport tcp -i rtsp://%s:%s@%s:%d/ -t 1 -f null - 2>&1",
            username, password, ip, ONVIF_PORT);

    FILE *fc = popen(cmd, "r");
    
    char buf[512];
    while (fgets(buf, sizeof(buf), fc) != NULL)
        printf("%s", buf);

    return 0;
}