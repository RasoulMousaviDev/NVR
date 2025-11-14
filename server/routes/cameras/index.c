#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#define MAX_LINE 2048
#define MAX_KV 128
#define FILE_PATH "cameras.txt"

void escape_json(const char *src, char *dst)
{
    while (*src)
    {
        if (*src == '\"')
        {
            *dst++ = '\\';
            *dst++ = '\"';
        }
        else if (*src == '\\')
        {
            *dst++ = '\\';
            *dst++ = '\\';
        }
        else if (*src == '\n')
        {
            *dst++ = '\\';
            *dst++ = 'n';
        }
        else if (*src == '\r')
        {
            *dst++ = '\\';
            *dst++ = 'r';
        }
        else if (*src == '\t')
        {
            *dst++ = '\\';
            *dst++ = 't';
        }
        else
            *dst++ = *src;
        src++;
    }
    *dst = 0;
}

int main()
{
    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");

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

    FILE *fp = fopen(file_path, "r");
    if (!fp)
        return 1;
    char line[MAX_LINE];
    printf("{\"items\": [");
    int first_line = 1;
    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = 0;
        if (!first_line)
            printf(",");
        first_line = 0;
        printf("{");
        char *kv_list[MAX_KV];
        int kv_count = 0;
        char *token = strtok(line, "&");
        while (token && kv_count < MAX_KV)
        {
            kv_list[kv_count++] = token;
            token = strtok(NULL, "&");
        }
        for (int i = 0; i < kv_count; i++)
        {
            char *eq = strchr(kv_list[i], '=');
            if (!eq)
                continue;
            *eq = 0;
            char key[1024], value[2048];
            escape_json(kv_list[i], key);
            escape_json(eq + 1, value);
            printf("\"%s\":\"%s\"", key, value);
            if (i != kv_count - 1)
                printf(",");
        }
        printf("}");
    }

    printf("]}");
    fclose(fp);

    return 0;
}