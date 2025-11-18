#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    FILE *pipe;
    unsigned char buffer[1024*512];
    size_t n;

    printf("Content-Type: multipart/x-mixed-replace; boundary=frame\r\n");
    printf("Cache-Control: no-cache\r\n");
    printf("Connection: close\r\n\r\n");
    fflush(stdout);

    pipe = popen("/mnt/mmcblk0p1/bin/ffmpeg -rtsp_transport tcp -i rtsp://admin:admin@192.168.1.239:554/stream -an -vf scale=320:-1 -q:v 10 -update 1 -f singlejpeg -", "r");
    if (!pipe) return 1;

    while ((n = fread(buffer, 1, sizeof(buffer), pipe)) > 0) {
        printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %zu\r\n\r\n", n);
        fwrite(buffer, 1, n, stdout);
        fflush(stdout);
    }

    pclose(pipe);
    return 0;
}
