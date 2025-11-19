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

int main()
{
    signal(SIGPIPE, stop);
    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    printf("Content-Type: text/plain\r\n\r\n");

    FILE *pipe = popen(
        "/mnt/mmcblk0p1/bin/ffmpeg -rtsp_transport tcp -i rtsp://admin:admin@192.168.1.239:554/stream \
-c:v copy -an -r 3 -f hls -hls_time 0 -hls_list_size 1 -hls_wrap 1 \
/mnt/mmcblk0p1/www/stream.m3u8",
        "r");

    if (!pipe)
    {
        printf("Failed to start ffmpeg\n");
        return 1;
    }

    while (run)
    {
        if (feof(stdout))
        {
            break;
        }

        usleep(500000);
    }

    system("kill -9 $STREAM_PID");

    unlink("/www/stream.jpg");

    return 0;
}
