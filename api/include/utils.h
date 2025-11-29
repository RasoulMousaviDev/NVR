
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int camera_get(const char *id, char *key, char *value, size_t value_size)
{
    char *base_path = getenv("BASE_PATH");

    char file_path[64];
    snprintf(file_path, sizeof(file_path), "%s/www/routes/cameras/list.txt", base_path);

    char cmd[256];
    snprintf(cmd, sizeof(cmd), "sed -n \"/id=%s/ s/.*%s=\\([^&]*\\).*/\\1/p\" %s", id, key, file_path);

    char line[32];
    FILE *fp = popen(cmd, "r");

    fgets(line, sizeof(line), fp);
    line[strcspn(line, "\n")] = 0;
    strncpy(value, line, value_size - 1);
    value[value_size  - 1] = '\0';

    fclose(fp);

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