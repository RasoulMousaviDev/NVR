#include "/home/rasoul/Projects/NVR/server/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

volatile sig_atomic_t run = 1;

void stop(int sig)
{
    run = 0;
}

int main(int argc, char *argv[])
{
    signal(SIGPIPE, stop);
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "GET") != 0)
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
    printf("Status: 200 OK\r\n\r\n");

    char *base_path = getenv("BASE_PATH");
    char model[32], ip[32], username[32], password[32];
    camera_get(id, "model", model);
    camera_get(id, "ip", ip);
    camera_get(id, "username", username);
    camera_get(id, "password", password);

    logger("Camera(%s) stream starting ...", model);

    char path[256];
    snprintf(path, sizeof(path), "%s/www/hls/%s", base_path, model);

    char mkdir_cmd[512];
    snprintf(mkdir_cmd, sizeof(mkdir_cmd), "mkdir -p %s", path);

    system(mkdir_cmd);

    char stream_cmd[1024];
    snprintf(stream_cmd, sizeof(stream_cmd),
             "(%s/bin/ffmpeg -rtsp_transport tcp -i rtsp://%s:%s@%s:%d/stream "
             "-c:v copy -an -r 3 -f hls -hls_time 0 -hls_list_size 1 -hls_wrap 1 "
             "%s/stream.m3u8 &) & echo $! > %s/tmp/%s-stream.pid",
             base_path, username, password, ip, 554, path, base_path, model);

    FILE *pipe = popen(stream_cmd, "r");
    if (!pipe)
    {
        printf("Failed to start ffmpeg\n");
        return 1;
    }

    while (run)
    {
        if (feof(stdout))
            break;

        usleep(500000);
    }

    char kill_cmd[128];
    snprintf(kill_cmd, sizeof(kill_cmd),
             "kill -9 $(cat %s/tmp/%s-stream.pid) && rm -r %s/*",
             base_path, model, path);

    system(kill_cmd);

    logger("Camera(%s) stream stopped", model);

    return 0;
}
