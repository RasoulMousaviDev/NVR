#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void sanitize_path(char *path)
{
    char *p;
    while ((p = strstr(path, "..")) != NULL)
    {
        memmove(p, p + 2, strlen(p + 2) + 1);
    }
}

void append_json_entry(char *json, const char *name, const char *type, int *first)
{
    if (!(*first))
        strcat(json, ",");
    strcat(json, "{\"name\":\"");
    strcat(json, name);
    strcat(json, "\",\"type\":\"");
    strcat(json, type);
    strcat(json, "\"}");
    *first = 0;
}

void list_dir(const char *path, char *json, int *first)
{
    DIR *dir = opendir(path);
    if (!dir)
        return;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(full_path, &st) == -1)
            continue;

        const char *type = S_ISDIR(st.st_mode) ? "folder" : "file";
        append_json_entry(json, entry->d_name, type, first);
    }
    closedir(dir);
}

int main()
{
    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "GET") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }
    
    char *base_path = getenv("BASE_PATH");

    char videos_path[64];
    snprintf(videos_path, sizeof(videos_path), "%s/videos/", base_path);

    char path[1024];
    strcpy(path, videos_path);

    char *query = getenv("QUERY_STRING");
    if (query && strlen(query) > 10)
    {
        for (int i = 0; query[i] != '\0'; i++)
        {
            if (query[i] == ',')
            {
                query[i] = '/';
            }
        }
        strcat(path, query + 10);
        strcat(path, "/");
    }

    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");

    char json[8192];
    strcpy(json, "[");
    int first = 1;
    list_dir(path, json, &first);
    strcat(json, "]");

    printf("%s\n", json);
    return 0;
}
