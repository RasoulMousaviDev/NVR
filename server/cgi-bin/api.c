#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#define BASE_PATH "../routes"

void send_401()
{
    printf("Status: 401 Unauthorized\r\n");
    printf("Content-Type: text/plain\r\n\r\n");
    printf("Unauthorized\n");
}

void send_404()
{
    printf("Status: 404 Not Found\r\n");
    printf("Content-Type: text/plain\r\n\r\n");
    printf("404 Not Found\n");
}

void send_500()
{
    printf("Status: 500 Internal Server Error\r\n");
    printf("Content-Type: text/plain\r\n\r\n");
    printf("500 Internal Server Error\n");
}

int autheticate()
{
    char *cookie = getenv("HTTP_COOKIE");
    if (!cookie)
        return 0;

    char token[128] = {0};
    char *p = strstr(cookie, "auth_token=");
    if (!p)
        return 0;

    sscanf(p, "auth_token=%127s", token);

    FILE *fp = fopen("/tmp/auth_token", "r");
    if (!fp)
        return 0;

    char saved[128] = {0};
    fgets(saved, sizeof(saved), fp);
    fclose(fp);

    if (strcmp(saved, token) == 0)
        return 1;

    return 0;
}

int main(void)
{
    char *uri = getenv("REQUEST_URI");
    if (!uri)
    {
        send_500();
        return 1;
    }

    if (!autheticate() && strcmp(uri, "/cgi-bin/api/login") != 0)
    {
        send_401();
        return 1;
    }

    char arguments[32];
    char path[1023];
    snprintf(path, sizeof(path), "%s", BASE_PATH);

    char *query_start = strchr(uri, '?');
    if (query_start != NULL)
    {
        *query_start = '\0';
    }

    char *token = strtok(uri + 12, "/");

    while (token != NULL)
    {
        char newpath[1279];

        char fullpath[1024];
        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, token);
        struct stat st;
        if (stat(fullpath, &st) == 0)
        {
            snprintf(newpath, sizeof(newpath), "%s/%s", path, token);
            strncpy(path, newpath, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        }
        else
        {
            DIR *d = opendir(path);
            if (d)
            {
                struct dirent *entry;
                while ((entry = readdir(d)) != NULL)
                {
                    char *s = entry->d_name;
                    size_t len = strlen(s);
                    if (len >= 2 && s[0] == '[' && s[len - 1] == ']')
                    {
                        snprintf(newpath, sizeof(newpath), "%s/%s", path, entry->d_name);
                        strncpy(path, newpath, sizeof(path) - 1);
                        path[sizeof(path) - 1] = '\0';

                        snprintf(arguments, sizeof(arguments), "%s ", token);
                        break;
                    }
                }
                closedir(d);
            }
        }
        token = strtok(NULL, "/");
    }

    struct stat st;
    if (stat(path, &st) != 0)
    {
        send_404();
        return 1;
    }

    if (S_ISDIR(st.st_mode))
    {
        char index_path[1042];
        snprintf(index_path, sizeof(index_path), "%s/index", path);
        if (access(index_path, X_OK) != 0)
        {
            send_404();
            return 1;
        }
        strcpy(path, index_path);
    }
    else if (!S_ISREG(st.st_mode) || access(path, X_OK) != 0)
    {
        send_404();
        return 1;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s %s", path, arguments);

    FILE *fp = popen(cmd, "r");
    if (!fp)
    {
        printf("Failed to execute\n");
        return 1;
    }

    char buffer[9024];
    while (fgets(buffer, sizeof(buffer), fp))
    {
        printf("%s", buffer);
    }

    pclose(fp);

    return 0;
}
