#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

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

    char img_q[128] = "", aud_q[128] = "", dur[128] = "";
    char *p;

    if ((p = strstr(body, "image_quality=")))
        sscanf(p, "image_quality=%127[^&]", img_q);
    if ((p = strstr(body, "audio_quality=")))
        sscanf(p, "audio_quality=%127[^&]", aud_q);
    if ((p = strstr(body, "duration=")))
        sscanf(p, "duration=%127[^&]", dur);
    free(body);

    char *base_path = getenv("BASE_PATH");

    char file_path[64];
    snprintf(file_path, sizeof(file_path), "%s/www/routes/cameras/list.txt", base_path);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "sed -i '/id=%s/ { s|image_quality=[^&]*|image_quality=%s|g; s|audio_quality=[^&]*|audio_quality=%s|g; s|duration=[^&]*|duration=%s|g; }' %s",
             id, img_q, aud_q, dur, file_path);
    int status = system(cmd);

    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");
    printf("{\"ok\": true}\n");

    return 0;
}
