#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <ctype.h>

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
    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "GET") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }

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

    const char *ignored_keys[] = {"username", "password", NULL};

    FILE *fp = fopen(file_path, "r");
    if (!fp){
        printf("{\"items\": []}");
        return 1;
    }
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

            int skip = 0;
            for (int k = 0; ignored_keys[k] != NULL; k++)
            {
                if (strcmp(key, ignored_keys[k]) == 0)
                {
                    skip = 1;
                    break;
                }
            }
            if (skip)
                continue;

            escape_json(eq + 1, value);

            int is_number = 1;
            int dot_count = 0;

            if (strlen(value) > 1 && value[0] == '0')
                is_number = 0;

            for (char *p = value; *p; p++)
            {
                if (*p == '.')
                {
                    dot_count++;
                    if (dot_count > 1)
                    {
                        is_number = 0;
                        break;
                    }
                }
                else if (!isdigit(*p) && *p != '-' && *p != '+')
                {
                    is_number = 0;
                    break;
                }
            }
            if (is_number)
                printf("\"%s\":%s", key, value);
            else
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