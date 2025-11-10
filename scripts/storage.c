#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_FILES 100000
#define MAX_PATH 1024
#define ENV_FILE ".env"
#define CHECK_INTERVAL 20

typedef struct
{
    char path[MAX_PATH];
    time_t mtime;
} FileInfo;

FileInfo files[MAX_FILES];
size_t file_count = 0;

char *read_env_var(const char *path, const char *var)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    char line[1024];
    char *value = NULL;

    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, var, strlen(var)) == 0 && line[strlen(var)] == '=')
        {
            value = strdup(line + strlen(var) + 1);
            value[strcspn(value, "\r\n")] = 0;
            break;
        }
    }

    fclose(f);
    if (!value)
        return NULL;

    size_t len = strlen(value);
    if (len >= 2 && value[0] == '"' && value[len - 1] == '"')
    {
        value[len - 1] = 0;
        memmove(value, value + 1, len - 1);
    }

    return value;
}

int collect_files(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    (void)ftwbuf;
    if (typeflag == FTW_F)
    {
        if (strstr(fpath, ".enc") || strstr(fpath, ".iv"))
        {
            if (file_count < MAX_FILES)
            {
                strncpy(files[file_count].path, fpath, MAX_PATH - 1);
                files[file_count].mtime = sb->st_mtime;
                file_count++;
            }
        }
    }
    return 0;
}

int compare_files(const void *a, const void *b)
{
    const FileInfo *fa = a;
    const FileInfo *fb = b;
    return (fa->mtime - fb->mtime);
}

double get_disk_usage(const char *path)
{
    struct statvfs buf;
    if (statvfs(path, &buf) != 0)
    {
        perror("statvfs");
        return -1;
    }
    unsigned long long total = buf.f_blocks * buf.f_frsize;
    unsigned long long used = (buf.f_blocks - buf.f_bfree) * buf.f_frsize;
    return (double)used / total;
}

int main()
{
    char *storage_path = read_env_var(ENV_FILE, "STORAGE_PATH");
    if (!storage_path)
    {
        printf("STORAGE_PATH not found in %s\n", ENV_FILE);
        return 1;
    }

    char *threshold_str = read_env_var(ENV_FILE, "THRESHOLD");
    double threshold = threshold_str ? atof(threshold_str) : 0.9;
    if (!threshold_str)
        printf("THRESHOLD not found in %s, using default 0.9\n", ENV_FILE);

    free(threshold_str);

    while (1)
    {
        double usage = get_disk_usage(storage_path);
        if (usage < 0)
        {
            sleep(CHECK_INTERVAL);
            continue;
        }

        printf("Current disk usage: %.2f%%\n", usage * 100);

        if (usage >= threshold)
        {
            file_count = 0;
            if (nftw(storage_path, collect_files, 20, 0) == -1)
            {
                perror("nftw");
                sleep(CHECK_INTERVAL);
                continue;
            }

            qsort(files, file_count, sizeof(FileInfo), compare_files);

            for (size_t i = 0; i < file_count; i++)
            {
                if (remove(files[i].path) == 0)
                {
                    printf("deleted: %s\n", files[i].path);
                }
                else
                {
                    perror("delete failed");
                }

                usage = get_disk_usage(storage_path);
                if (usage < threshold)
                {
                    printf("Disk usage now below threshold (%.2f%%)\n", usage * 100);
                    break;
                }
            }
        }

        sleep(CHECK_INTERVAL);
    }

    free(storage_path);
    return 0;
}
