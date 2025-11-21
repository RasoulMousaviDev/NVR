#include <stdio.h>
#include <stdlib.h>
#include <sys/statvfs.h>

int main()
{
    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n\r\n");

    char *base_path = getenv("BASE_PAHT");
    struct statvfs stat;

    if (statvfs(base_path, &stat) != 0)
    {
        printf("{\"error\": \"Error reading disk info\"}\n");
        return 1;
    }

    double total = (double)(stat.f_blocks * stat.f_frsize) / (1024 * 1024 * 1024);
    double free = (double)(stat.f_bfree * stat.f_frsize) / (1024 * 1024 * 1024);
    double used = total - free;

    printf("{\"size\": %.1f,\"used\": %.1f}", total, used);

    return 0;
}
