#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define BASE_PATH "../routes"

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

int main(void)
{
    char *query = getenv("REQUEST_URI");
    if (!query)
    {
        send_500();
        return 1;
    }

    if (strncmp(query, "/cgi-bin/api", 12) != 0)
    {
        send_404();
        return 1;
    }

    char path[1024];
    snprintf(path, sizeof(path), "%s%s", BASE_PATH, query + 12);

    char final_path[1024];
    int i = 0, j = 0;
    while (path[i])
    {
        if (path[i] == ':')
        {
            final_path[j++] = '[';
            i++;
            while (path[i] && path[i] != '/')
            {
                final_path[j++] = path[i++];
            }
            final_path[j++] = ']';
        }
        else
        {
            final_path[j++] = path[i++];
        }
    }
    final_path[j] = '\0';

    struct stat st;
    if (stat(final_path, &st) != 0)
    {
        send_404();
        return 1;
    }

    if (S_ISDIR(st.st_mode))
    {
        char index_path[1042];
        snprintf(index_path, sizeof(index_path), "%s/index", final_path);
        if (access(index_path, X_OK) != 0)
        {
            send_404();
            return 1;
        }
        strcpy(final_path, index_path);
    }
    else if (!S_ISREG(st.st_mode) || access(final_path, X_OK) != 0)
    {
        send_404();
        return 1;
    }

    char cmd[2048];
    snprintf(cmd, sizeof(cmd), "%s 2>log.txt", final_path);

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
