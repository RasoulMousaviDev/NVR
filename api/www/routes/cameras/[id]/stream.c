#include "/home/rasoul/Projects/NVR/api/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Status: 400 Bad Request\r\n\r\n");
        return 1;
    }

    char *id = argv[1];

    char *base_path = getenv("BASE_PATH");

    char model[32], ip[32], username[32], password[32];
    camera_get(id, "model", model, sizeof(model));

    char path[256];
    snprintf(path, sizeof(path), "%s/www/hls/%s", base_path, model);

    char *method = getenv("REQUEST_METHOD");
    if (strcmp(method, "POST") == 0)
    {
        printf("Status: 200 OK\r\n");
        printf("Content-type: application/json\r\n\r\n");

        camera_get(id, "ip", ip, sizeof(ip));
        camera_get(id, "username", username, sizeof(username));
        camera_get(id, "password", password, sizeof(password));

        if (strlen(username) == 0)
        {
            printf("{\"ok\":false,\"error\":\"Unauthorized\"}");
            exit(0);
        }

        logger("Camera(%s) stream starting ...", model);

        char mkdir_cmd[512];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", path);

        system(mkdir_cmd);

        char ffmpeg[512];
        snprintf(ffmpeg, sizeof(ffmpeg),
                 "%s/bin/ffmpeg",
                 base_path);

        char rtsp_url[512];
        snprintf(rtsp_url, sizeof(rtsp_url),
                 "rtsp://%s:%s@%s:554/stream",
                 username, password, ip);

        char output_path[512];
        snprintf(output_path, sizeof(output_path),
                 "%s/stream.m3u8",
                 path);

        pid_t pid = fork();

        if (pid == 0)
        {
            freopen("/dev/null", "r", stdin);
            freopen("/dev/null", "w", stdout);
            freopen("/dev/null", "w", stderr);

            execl(ffmpeg,
                  "ffmpeg",
                  "-rtsp_transport", "tcp",
                  "-i", rtsp_url,
                  "-c:v", "copy",
                  "-an",
                  "-r", "3",
                  "-f", "hls",
                  "-hls_time", "1",
                  "-hls_list_size", "2",
                  "-hls_wrap", "2",
                  output_path,
                  NULL);
            exit(1);
        }

        char pid_file[512];
        snprintf(pid_file, sizeof(pid_file),
                 "%s/tmp/%s-stream.pid",
                 base_path, model);
        FILE *f = fopen(pid_file, "w");
        if (f)
        {
            fprintf(f, "%d", pid);
            fclose(f);
        }

        printf("{\"ok\":true,\"message\":\"Stream started\"}");

        fflush(stdout);
        sleep(3);
        return 0;
    }
    else if (strcmp(method, "DELETE") == 0)
    {
        printf("Status: 200 OK\r\n");
        printf("Content-type: application/json\r\n\r\n");

        char kill_cmd[128];
        snprintf(kill_cmd, sizeof(kill_cmd),
                 "kill -9 $(cat %s/tmp/%s-stream.pid) && rm -r %s/*",
                 base_path, model, path);
        system(kill_cmd);

        logger("Camera(%s) stream stopped", model);

        printf("{\"ok\":true,\"message\":\"Stream stopped\"}");

        return 0;
    }
    else
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }

    return 0;
}
