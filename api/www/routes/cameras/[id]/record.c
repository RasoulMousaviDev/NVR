#include "/home/rasoul/Projects/NVR/server/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>

int get_file_path(char *id, char *file_path)
{
    char model[32];
    camera_get(id, "model", model);

    char *base_path = getenv("BASE_PATH");

    snprintf(file_path, sizeof(file_path), "%s/tmp/%s-record.pid", base_path, model);

    return 0;
}

int is_recording(char *id)
{
    char file_path[64];
    get_file_path(id, file_path);

    FILE *fp = fopen(file_path, "r");
    pid_t pid;

    if (!fp)
        return 0;

    if (fscanf(fp, "%d", &pid) == 1)
    {
        fclose(fp);
        if (kill(pid, 0) == 0 || errno != ESRCH)
            return pid;
    }

    fclose(fp);

    return 0;
}

int stop_recording(char *id)
{
    pid_t pid = is_recording(id);

    if (pid > 0)
    {
        if (kill(pid, SIGTERM) == 0)
        {
            waitpid(pid, NULL, 0);

            char file_path[64];
            get_file_path(id, file_path);
            remove(file_path);

            return 0;
        }
    }
    return -1;
}

int start_recording(char *id)
{
    if (is_recording(id))
        return 0;

    char *base_path = getenv("BASE_PATH");

    char model[32], ip[16], username[16], password[16], duration[2];
    camera_get(id, "model", model);
    camera_get(id, "ip", ip);
    camera_get(id, "username", username);
    camera_get(id, "password", password);
    camera_get(id, "duration", duration);

    logger("Camera(%s) record starting ...", model);

    char record_cmd[2048];
    snprintf(record_cmd, sizeof(record_cmd),
             "(%s/bin/ffmpeg -loglevel error -rtsp_transport tcp -i rtsp://%s:%s@%s:%s/stream "
             "-c copy -map 0 -f segment -segment_time %s -reset_timestamps 1"
             "-reset_timestamps 1 -strftime 1 -segment_format mp4 -segment_list_type flat "
             "-segment_format_options movflags=+frag_keyframe+empty_moov -segment_list %s/tmp/%s-segments.txt "
             "%s/tmp/%s-%%Y-%%m-%%d-%%H-%%m-%%S.mp4 >> %s/tmp/%s-log.txt 2>&1 & echo $! > %s/tmp/%s-record.pid ) &",
             base_path, username, password, ip, 554,
             duration,
             base_path, model,
             base_path, model, base_path, model, base_path, model);

    if (system(record_cmd) == -1)
    {
        logger("Camera(%s) record failed", model);
        return -1;
    }

    char encrypt_cmd[2048];
    snprintf(encrypt_cmd, sizeof(encrypt_cmd), "%s/bin/encrypt %s &", base_path, model);

    if (system(encrypt_cmd) == -1)
    {
        logger("File encrypt run failed");
        logger("Camera(%s) record failed", model);
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Status: 400 Bad Request\r\n\r\n");
        return 1;
    }
    char *id = argv[1];

    char *method = getenv("REQUEST_METHOD");
    if (strcmp(method, "POST") == 0)
    {
        printf("Status: 200 OK\r\n");
        printf("Content-type: application/json\r\n\r\n");
        if (start_recording(id) == 0)
            printf("{\"ok\":\"true\",\"action\":\"recording started\"}");
        else
            printf("{\"ok\":\"false\",\"message\":\"Failed to start recording\"}");
    }
    else if (strcmp(method, "DELETE") == 0)
    {
        printf("Status: 200 OK\r\n");
        printf("Content-type: application/json\r\n\r\n");
        if (stop_recording(id) == 0)
            printf("{\"ok\":\"true\",\"action\":\"recording stopped\"}");
        else
            printf("{\"ok\":\"false\",\"message\":\"Failed to stop recording or process not found\"}");
    }
    else
        printf("Status: 405 Method Not Allowed\r\n\r\n");

    return 0;
}