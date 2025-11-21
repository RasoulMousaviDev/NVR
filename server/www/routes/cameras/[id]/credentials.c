#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

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

#define FILE_PATH "cameras.txt"
#define MAX_BODY 65536
#define RTSP_PORT 554

#define FFMPG_BIN "/mnt/mmcblk0p1/bin/ffmpeg"
#define TEMP_OUTPUT_FILE "/tmp/ffmpeg_output.txt"

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

        int fd = open(TEMP_OUTPUT_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
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

        execvp(FFMPG_BIN, args);
        _exit(1);
    }

    else
    {
        if (waitpid(pid, &status, 0) == -1)
        {
            result = 2;
        }

        FILE *fp = fopen(TEMP_OUTPUT_FILE, "r");
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
            {
                result = 1;
            }
            else if (auth_found && WIFEXITED(status) && WEXITSTATUS(status) == 0)
            {
                result = 0;
            }
            else
            {
                result = 2;
            }
        }

        remove(TEMP_OUTPUT_FILE);

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
        return 1;
    }

    char ip[64] = {0};
    find_camera(fp, id, ip);

    char url[512];
    sprintf(url, "rtsp://%s:%s@%s:%d/", username, password, ip, RTSP_PORT);
    int result_success = check_credentials(url);

    printf("Content-Type: application/json\r\n");
    fclose(fp);

    if (result_success == 0)
    {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd),
                 "sed -i '/id=%s/ { s|username=[^&]*|username=%s|g; s|password=[^&]*|password=%s|g; }' %s",
                 id, username, password, file_path);
        int status = system(cmd);
        printf("Status: 200 OK\r\n\r\n");
        printf("{\"ok\": true}");
    }
    else
    {
        printf("Status: 401 Unauthorized\r\n\r\n");
        printf("{\"ok\": false}");
    }


    return 0;
}