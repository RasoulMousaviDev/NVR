#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define KEY_FILE "/mnt/mmcblk0p1/keys/enc.key"
#define VIDEO_BASE_PATH "/mnt/mmcblk0p1/videos"
#define MAX_PATH 1024
#define BUF_SIZE 4096

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "GET") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        return 0;
    }

    if (argc < 2)
    {
        printf("Status: 400 Bad Request\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        return 0;
    }

    char year[5], month[3], day[3], hour[3], minute[3], second[3];
    if (sscanf(argv[1], "%4[0-9]-%2[0-9]-%2[0-9]-%2[0-9]-%2[0-9]-%2[0-9]",
               year, month, day, hour, minute, second) != 6)
    {
        printf("Status: 400 Bad Request\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        return 0;
    }

    char key_hex[33];
    FILE *kf = fopen(KEY_FILE, "rb");
    if (!kf || fread(key_hex, 1, 16, kf) != 16)
    {
        printf("Status: 500 Internal Server Error\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        if (kf)
            fclose(kf);
        return 0;
    }
    fclose(kf);

    char key_str[64];
    for (int i = 0; i < 16; i++)
        sprintf(key_str + i * 2, "%02X", (unsigned char)key_hex[i]);
    key_str[32] = 0;

    char ts_path[MAX_PATH];
    snprintf(ts_path, MAX_PATH, "%s/%s/%s/%s/%s/%s-%s.ts",
             VIDEO_BASE_PATH, year, month, day, hour, minute, second);

    struct stat st;
    if (stat(ts_path, &st) != 0)
    {
        printf("Status: 404 Not Found\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        return 0;
    }

    printf("Status: 200 OK\r\n");
    printf("Content-Type: video/mp4\r\n");
    printf("Accept-Ranges: none\r\n");
    printf("\r\n");

    char cmd[MAX_PATH + 200];
    snprintf(
        cmd,
        sizeof(cmd),
        "/mnt/mmcblk0p1/bin/ffmpeg -loglevel quiet -decryption_key %s -i '%s' -movflags frag_keyframe+empty_moov -c copy -f mp4 pipe:1",
        key_str,
        ts_path);

    FILE *fp = popen(cmd, "r");
    if (!fp)
        return 0;

    unsigned char buf[BUF_SIZE];
    size_t n;

    while ((n = fread(buf, 1, BUF_SIZE, fp)) > 0)
        fwrite(buf, 1, n, stdout);

    pclose(fp);
    return 0;
}
