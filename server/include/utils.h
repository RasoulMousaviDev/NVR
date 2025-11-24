
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int camera_find(const char *id, char *camera)
{
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/www/cameras/cameras.txt", getenv("BASE_PATH"));

    FILE *fp = fopen(file_path, "a+");
    char line[1024];

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "id=", 3) == 0)
        {
            const char *id_value = line + 3;

            if (strcmp(id_value, id) == 0)
            {
                strncpy(camera, line, sizeof(line));
                break;
            }
        }
    }

    fclose(fp);
    return 1;
}

int camera_get(const char *id, char *key, char *value)
{
    char camera[1024];

    if (camera_find(id, camera))
        return 1;

    char query[32];
    snprintf(query, sizeof(query), "%s=%127[^&]", key);

    sscanf(camera, query, value);

    return 0;
}

static void logger(const char *fmt, ...)
{
    char *base_path = getenv("BASE_PATH");

    char log_file[64];
    snprintf(log_file, sizeof(log_file), "%s/tmp/log.json", base_path);

    FILE *fp = fopen(log_file, "a");
    if (!fp)
        return;

    time_t t = time(NULL);
    struct tm tm;
    localtime_r(&t, &tm);

    char date[64];
    strftime(date, sizeof(date), "%Y-%m-%d - %H:%M:%S", &tm);

    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    fprintf(fp, "{\"date\":\"%s\",\"message\":\"%s\"}\n", date, msg);

    fclose(fp);
}