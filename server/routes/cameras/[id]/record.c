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

#define HEX_KEY "2cc4c6c797ead8baf874a4bf14ab04d0c9c2a43d1b629011375fc98f0c1e4ae3"
#define PID_FILE "/mnt/mmcblk0p1/record.pid"
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

    const char *rtsp_url = "rtsp://admin:admin@192.168.1.239:554/stream";
    const char *video_path = "/mnt/mmcblk0p1/videos";
    const char *filename = "%Y-%m-%d-%H-%m-%S.mp4";
    const char *log_path = "/mnt/mmcblk0p1/ffmpeg_output.log";

    const char *cmd_template =
        "(/mnt/mmcblk0p1/bin/ffmpeg -loglevel error "
        "-rtsp_transport tcp "
        "-i %s "
        "-c copy "
        "-map 0 "
        "-f segment "
        "-segment_time 4 "
        "-reset_timestamps 1 "
        "-strftime 1 "
        "-segment_format mp4 "
        "-segment_format_options movflags=+frag_keyframe+empty_moov "
        "-segment_list /mnt/mmcblk0p1/segment_list.txt "
        "-segment_list_type flat "
        "%s/%s "
        "> %s 2>&1 & "
        "FFMPEG_PID=$! ; "
        "/mnt/mmcblk0p1/www/bin/encrypt /mnt/mmcblk0p1/segment_list.txt %s $FFMPEG_PID & "
        "echo $FFMPEG_PID > " PID_FILE " ) &";

    char FFMPEG_DAEMON_CMD[2048];

    snprintf(FFMPEG_DAEMON_CMD, sizeof(FFMPEG_DAEMON_CMD),
             cmd_template,
             rtsp_url,
             video_path,
             filename,
             log_path,
            HEX_KEY
            );

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