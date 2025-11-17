#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>

#define FILE_PATH "cameras.txt"

void update_value(char *line, const char *key, const char *value)
{
    if (!value || strlen(value) == 0)
        return;
    char *p = strstr(line, key);
    if (!p)
        return;
    char *eq = strchr(p, '=');
    if (!eq)
        return;
    eq++;
    char *amp = strchr(eq, '&');
    if (!amp)
        amp = eq + strlen(eq);
    size_t old_len = amp - eq;
    size_t new_len = strlen(value);

    if (new_len <= old_len)
    {
        memcpy(eq, value, new_len);
        memmove(eq + new_len, eq + old_len, strlen(eq + old_len) + 1);
    }
    else
    {

        if ((eq - line) + new_len + strlen(amp) + 1 < 1024)
        {
            memmove(eq + new_len, amp, strlen(amp) + 1);
            memcpy(eq, value, new_len);
        }
    }
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

    char img_q[128] = "", aud_q[128] = "", dur[128] = "";
    char *p;

    if ((p = strstr(body, "image_quality=")))
        sscanf(p, "image_quality=%127[^&]", img_q);
    if ((p = strstr(body, "audio_quality=")))
        sscanf(p, "audio_quality=%127[^&]", aud_q);
    if ((p = strstr(body, "duration=")))
        sscanf(p, "duration=%127[^&]", dur);
    free(body);

    char exe_path[1024];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1)
        return 1;
    exe_path[len] = '\0';

    p = strstr(exe_path, "/[id]/settings");
    if (p)
        *p = '\0';

    char file_path[PATH_MAX];
    snprintf(file_path, sizeof(file_path), "%s/%s", exe_path, FILE_PATH);

    FILE *f = fopen(file_path, "r+");

    if (!f)
    {
        printf("Status: 500 Internal Server Error\r\n\r\n");
        return 0;
    }

    char buffer[10000] = {0};
    char line[1024];
    long cur_pos = 0;

    rewind(f);
    while (fgets(line, sizeof(line), f))
    {
        long start_pos = cur_pos;
        cur_pos = ftell(f);

        char copy[1024];
        strncpy(copy, line, sizeof(copy));
        char *pid = strstr(copy, "id=");
        if (pid)
        {
            pid += 3;
            char cid[128];
            sscanf(pid, "%127[^&]", cid);
            if (strcmp(cid, id) == 0)
            {
                update_value(line, "image_quality", img_q);
                update_value(line, "audio_quality", aud_q);
                update_value(line, "duration", dur);
            }
        }
        strncat(buffer, line, sizeof(buffer) - strlen(buffer) - 1);
    }

    rewind(f);
    ftruncate(fileno(f), 0);
    fputs(buffer, f);
    fflush(f);
    fclose(f);

    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");
    printf("{\"ok\": true}\n");

    return 0;
}
