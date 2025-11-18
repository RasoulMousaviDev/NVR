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

#define PID_FILE "/mnt/mmcblk0p1/recorder.pid"
#define KEY_FILE "/mnt/mmcblk0p1/keys/enc.key"
#define KEY_INFO_FILE "/mnt/mmcblk0p1/keys/enc.keyinfo"

int is_recording()
{
    FILE *fp = fopen(PID_FILE, "r");
    pid_t pid;
    if (fp)
    {
        if (fscanf(fp, "%d", &pid) == 1)
        {
            fclose(fp);
            if (kill(pid, 0) == 0 || errno != ESRCH)
            {
                return pid;
            }
        }
        else
        {
            fclose(fp);
        }
    }
    return 0;
}

int stop_recording()
{
    pid_t pid = is_recording();

    if (pid > 0)
    {
        if (kill(pid, SIGTERM) == 0)
        {
            waitpid(pid, NULL, 0);
            remove(PID_FILE);
            return 0;
        }
    }
    return -1;
}

int start_recording()
{
    if (is_recording())
    {
        return 0;
    }

    const char *key_info = KEY_INFO_FILE;
    const char *rtsp_url = "rtsp://admin:admin@192.168.1.239:554/stream";
    const char *video_path = "/mnt/mmcblk0p1/videos";
    const char *filename = "%Y/%m/%d/%H/%m-%S.ts";
    const char *dummy_m3u8 = "/mnt/mmcblk0p1/keys/dummy.m3u8";
    const char *log_path = "/mnt/mmcblk0p1/ffmpeg_output.log";

    const char *cmd_template =
        "(/mnt/mmcblk0p1/bin/ffmpeg -loglevel error -rtsp_transport tcp -i %s -c copy -map 0 -f hls -hls_time 10 -hls_list_size 0 -strftime 1 -strftime_mkdir 1 "
        "-hls_key_info_file %s -hls_segment_filename %s/%s %s > %s 2>&1 & echo $! > " PID_FILE " ) &";
    char FFMPEG_DAEMON_CMD[2048];

    snprintf(FFMPEG_DAEMON_CMD, sizeof(FFMPEG_DAEMON_CMD),
             cmd_template,
             rtsp_url,
             key_info,
             video_path,
             filename,
             dummy_m3u8,
             log_path);
    printf("%s", FFMPEG_DAEMON_CMD);

    if (system(FFMPEG_DAEMON_CMD) == -1)
    {
        return -1;
    }

    return 0;
}

int main(void)
{
    char *request_method = getenv("REQUEST_METHOD");

    printf("Content-type: application/json\r\n\r\n");

    if (!request_method)
    {
        printf("{\"ok\":\"false\",\"message\":\"Missing REQUEST_METHOD\"}");
        return 1;
    }

    if (strcmp(request_method, "POST") == 0)
    {
        if (start_recording() == 0)
        {
            printf("{\"ok\":\"true\",\"action\":\"recording_started\"}");
        }
        else
        {
            printf("{\"ok\":\"false\",\"message\":\"Failed to start recording\"}");
        }
    }
    else if (strcmp(request_method, "DELETE") == 0)
    {
        if (stop_recording() == 0)
        {
            printf("{\"ok\":\"true\",\"action\":\"recording_stopped\"}");
        }
        else
        {
            printf("{\"ok\":\"false\",\"message\":\"Failed to stop recording or process not found\"}");
        }
    }
    else
    {
        printf("{\"ok\":\"false\",\"message\":\"Method %s not supported\"}", request_method);
    }
    return 0;
}