#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "/home/rasoul/Projects/NVR/api/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>

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
                    return 0;
                }

                if (sscanf(ippos, "ip=%255[^&]", ip) == 1)
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int check_credentials(const char *rtspUrl)
{
    char *base_path = getenv("BASE_PATH");
    char temp_file[128];
    snprintf(temp_file, sizeof(temp_file), "%s/tmp/ffmpeg.txt", base_path);

    pid_t pid;
    int status;
    int result = 2;

    pid = fork();

    if (pid == -1)
    {
        return 2;
    }

    else if (pid == 0)
    {

        int fd = open(temp_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1)
        {
            _exit(1);
        }

        dup2(fd, 1);
        dup2(fd, 2);
        close(fd);

        char *args[] = {
            "ffmpeg",
            "-i", (char *)rtspUrl,
            "-frames:v", "1",
            "-f", "null",
            "-",
            NULL};

        char ffmpeg[128];
        snprintf(ffmpeg, sizeof(ffmpeg), "%s/bin/ffmpeg", base_path);
        execvp(ffmpeg, args);
        _exit(1);
    }

    else
    {
        if (waitpid(pid, &status, 0) == -1)
        {
            result = 2;
        }

        FILE *fp = fopen(temp_file, "r");
        if (fp != NULL)
        {
            char buffer[256];
            int auth_found = 0;
            int unauthorized_found = 0;

            while (fgets(buffer, sizeof(buffer), fp) != NULL)
            {
                if (strstr(buffer, "401 Unauthorized") != NULL)
                {
                    unauthorized_found = 1;
                }
                if (strstr(buffer, "RTSP Session") != NULL)
                {
                    auth_found = 1;
                }
            }
            fclose(fp);

            if (unauthorized_found)
                result = 1;
            else if (auth_found && WIFEXITED(status) && WEXITSTATUS(status) == 0)
                result = 0;
            else
                result = 2;
        }

        remove(temp_file);

        return result;
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

    logger("Camera credentials checking ...");

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

    char *base_path = getenv("BASE_PATH");

    char file_path[64];
    snprintf(file_path, sizeof(file_path), "%s/www/routes/cameras/list.txt", base_path);

    FILE *fp = fopen(file_path, "r+");
    if (!fp)
    {
        printf("Status: 500 Internal Server Error\r\n\r\n file not found");
        return 1;
    }

    char ip[64] = {0};
    find_camera(fp, id, ip);

    char url[512];
    sprintf(url, "rtsp://%s:%s@%s:%d/", username, password, ip, 554);
    int result_success = check_credentials(url);

    fclose(fp);

    if (result_success == 0)
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "sed -i '/id=%s/ { s|username=[^&]*|username=%s|g; s|password=[^&]*|password=%s|g; }' %s",
                 id, username, password, file_path);
        int status = system(cmd);
        printf("Status: 200 OK\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"ok\": true}");

        logger("Camera credentials success");
    }
    else
    {
        printf("Status: 403 Unauthorized\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"ok\": false}");

        logger("Camera credentials failed");
    }

    return 0;
}