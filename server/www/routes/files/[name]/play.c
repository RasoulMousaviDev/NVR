#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#define KEY_HEX "2cc4c6c797ead8baf874a4bf14ab04d0c9c2a43d1b629011375fc98f0c1e4ae3"
#define IV_HEX "00000000000000000000000000000000"
#define VIDEO_BASE_PATH "/mnt/mmcblk0p1/videos"

int main(int argc, char *argv[])
{
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

    char *base_path = getenv("BASE_PAHT");

    char model[32], year[5], month[3], day[3], hour[3], minute[3], second[3];
    if (sscanf(argv[1], "%4[0-9]-%2[0-9]-%2[0-9]-%2[0-9]-%2[0-9]-%2[0-9]",
               year, month, day, hour, minute, second) != 6)
    {
        printf("Status: 400 Bad Request\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error\"}");
        return 0;
    }

    char *key = getenv("AES_KEY");
    char *iv = "00000000000000000000000000000000";

    char input_path[1024];
    snprintf(input_path, sizeof(input_path), "%s/%s/%s/%s/%s/%s/%s-%s.enc",
             base_path, model, year, month, day, hour, minute, second);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s/bin/openssl enc -aes-256-cbc -d -in \"%s\" -out %s/www/play.mp4 -K %s -iv %s",
             base_path, input_path, base_path, key, iv);

    system(cmd);

    printf("Status: 302 Found\r\n");
    printf("Location: /play.mp4\r\n\r\n");
    
    return 0;
}
