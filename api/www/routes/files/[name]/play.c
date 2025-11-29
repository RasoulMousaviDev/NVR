#include "/home/rasoul/Projects/NVR/api/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>
#include <string.h>

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

    char *base_path = getenv("BASE_PATH");

    char model[64];
    int Y, M, D, h, m, s;
    if (sscanf(argv[1], "%63[^-]-%4d-%2d-%2d-%2d-%2d-%2d", &model, &Y, &M, &D, &h, &m, &s) != 7)
    {
        printf("Status: 400 Bad Request\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"status\":\"error %s\"}", argv[1]);
        return 0;
    }

    char *key = getenv("AES_KEY");
    char *iv = "00000000000000000000000000000000";

    char input_path[1024];
    snprintf(input_path, sizeof(input_path), "%s/videos/%s/%04d/%02d/%02d/%02d/%02d-%02d.enc",
             base_path, model, Y, M, D, h, m, s);
  
    logger("File decrypt(%s) starting ...", input_path);

    char cmd[2048];
    snprintf(cmd, sizeof(cmd),
             "%s/bin/openssl enc -aes-256-cbc -d -in \"%s\" -out %s/www/play.mp4 -K %s -iv %s 2>error_log",
             base_path, input_path, base_path, key, iv);

    if (system(cmd) == -1)
    {
        logger("File decrypt(%s) failed", input_path);
        return -1;
    }

    printf("Status: 302 Found\r\n");
    printf("Location: /play.mp4\r\n\r\n");

    logger("File decrypt(%s) success", input_path);

    return 0;
}
