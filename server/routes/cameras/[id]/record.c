#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

#define PID_FILE "/mnt/mmcblk0p1/rtsp_recorder.pid"

int is_recording() {
    FILE *fp = fopen(PID_FILE, "r");
    pid_t pid;
    if (fp) {
        if (fscanf(fp, "%d", &pid) == 1) {
            fclose(fp);
            if (kill(pid, 0) == 0 || errno != ESRCH) {
                return pid;
            }
        } else {
            fclose(fp);
        }
    }
    return 0;
}

int start_recording() {
    if (is_recording()) {
        return 0;
    }

    char *cmd[] = {
        "ffmpeg", 
        "-loglevel", "error", 
        "-rtsp_transport", "tcp", 
        "-i", "rtsp://admin:admin@192.168.1.239:554/stream", 
        "-c", "copy", 
        "-map", "0", 
        "-f", "segment", 
        "-segment_time", "60", 
        "-movflags", "+faststart", 
        "-strftime", "1", 
        "/mnt/mmcblk0p1/videos/%Y%m%d_%H%M%S.mp4", 
        NULL 
    };
    
    pid_t pid = fork();

    if (pid < 0) {
        return -1;
    }

    if (pid == 0) {
        setsid(); 
        execv("/mnt/mmcblk0p1/bin/ffmpeg", cmd);
        exit(1);
    } else {
        FILE *fp = fopen(PID_FILE, "w");
        if (fp) {
            fprintf(fp, "%d", pid);
            fclose(fp);
        }
    }
    return 0;
}

int stop_recording() {
    pid_t pid = is_recording();
    
    if (pid > 0) {
        if (kill(pid, SIGTERM) == 0) {
            waitpid(pid, NULL, 0); 
            remove(PID_FILE);
            return 0;
        }
    }
    return -1;
}

int main(void) {
    char *request_method = getenv("REQUEST_METHOD");

    printf("Content-type: application/json\r\n\r\n");

    if (!request_method) {
        printf("{\"ok\":\"false\",\"message\":\"Missing REQUEST_METHOD\"}");
        return 1;
    }

    if (strcmp(request_method, "POST") == 0) {
        if (start_recording() == 0) {
            printf("{\"ok\":\"true\",\"action\":\"recording_started\"}");
        } else {
            printf("{\"ok\":\"false\",\"message\":\"Failed to start recording\"}");
        }
    } else if (strcmp(request_method, "DELETE") == 0) {
        if (stop_recording() == 0) {
            printf("{\"ok\":\"true\",\"action\":\"recording_stopped\"}");
        } else {
            printf("{\"ok\":\"false\",\"message\":\"Failed to stop recording or process not found\"}");
        }
    } else {
        printf("{\"ok\":\"false\",\"message\":\"Method %s not supported\"}", request_method);
    }
    return 0;
}