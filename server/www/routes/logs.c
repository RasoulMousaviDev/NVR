#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "GET") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }

    printf("Status: 200 OK\r\n");

    char *query = getenv("QUERY_STRING");
    int start_line = 1;

    if (query)
    {
        char *p = strstr(query, "line=");
        if (p)
        {
            start_line = atoi(p + 5);
            if (start_line < 1)
                start_line = 1;
        }
    }

    printf("Content-Type: text/plain\r\n\r\n");

    char *base_path = getenv("BASE_PATH");

    char log_file[64];
    snprintf(log_file, sizeof(log_file), "%s/tmp/log.json", base_path);

    FILE *fp = fopen(log_file, "r");
    if (!fp)
    {
        printf("Error: cannot open log file\n");
        return 1;
    }

    char buf[1024];
    int line_num = 0;
    int lines_sent = 0;

    while (fgets(buf, sizeof(buf), fp))
    {
        line_num++;
        if (line_num < start_line)
            continue;

        printf("%s", buf);
        lines_sent++;

        if (lines_sent >= 20)
            break;
    }

    fclose(fp);
    
    return 0;
}
