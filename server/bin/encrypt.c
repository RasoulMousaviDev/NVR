#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <limits.h>

pid_t ffmpeg_pid = 0;

int check_storage(const char *base_path)
{
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "df -k %s | tail -1 | awk '{print int($5)}'", base_path);

    FILE *fp = popen(cmd, "r");
    if (!fp)
        return -1;

    int usage = 0;
    fscanf(fp, "%d", &usage);
    pclose(fp);

    if (usage < 90)
        return 0;

    char videos_dir[64];
    snprintf(videos_dir, sizeof(videos_dir), "%s/videos", base_path);

    snprintf(cmd, sizeof(cmd),
             "find %s -type f -name '*.enc' -exec stat -c '%%Y %%n' {} \\; | sort -n | head -1 | cut -d' ' -f2- | xargs -r rm -f",
             videos_dir);

    return check_storage(base_path);
}

int encrypt_file(const char *base_path, char *filename)
{
    char *key = getenv("AES_KEY");
    const char *iv = "00000000000000000000000000000000";

    char input_file[512];
    snprintf(input_file, sizeof(input_file), "%s/tmp/%s", base_path, filename);

    int model, Y, M, D, h, m, s;
    if (sscanf(filename, "%s-%4d-%2d-%2d-%2d-%2d-%2d", &model, &Y, &M, &D, &h, &m, &s) != 6)
        return -1;

    char output_idr[512];
    snprintf(output_idr, sizeof(output_idr), "%s/videos/%s/%04d/%02d/%02d/%02d", base_path, model, Y, M, D, h);

    char mkdir[512];
    snprintf(mkdir, sizeof(mkdir), "mkdir -p %s", output_idr);
    system(mkdir);

    char output_file[512];
    snprintf(output_file, sizeof(output_file), "%s/%2d-%2d.mp4", output_idr, h);

    char openssl_cmd[2048];
    snprintf(openssl_cmd, sizeof(openssl_cmd),
             "%s/bin/openssl enc -aes-256-cbc -e -in \"%s\" -out \"%s\" -K %s -iv %s ",
             base_path, input_file, output_file, key, iv);

    int status = system(openssl_cmd);

    if (status == 0)
    {
        remove(input_file);
        check_storage(base_path);
        return 0;
    }
    else
    {
        remove(output_file);
        return -1;
    }
}

void stop(int sig)
{
    if (ffmpeg_pid > 0)
        kill(ffmpeg_pid, SIGTERM);

    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
        return 1;

    signal(SIGTERM, stop);
    signal(SIGINT, stop);

    char *base_path = getenv("BASE_PAHT");

    char *model = argv[1];

    char segments_path[64];
    snprintf(segments_path, sizeof(segments_path), "%s/tmp/%s-segments.txt", base_path, model);

    char pid_path[64];
    snprintf(pid_path, sizeof(pid_path), "%s/tmp/%s-record.pid", base_path, model);

    FILE *fp = fopen(pid_path, "r");
    if (fp)
        fscanf(fp, "%d", &ffmpeg_pid);

    // fprintf(log, "Monitor started for PID %d\n", ffmpeg_pid);

    long int last_pos = 0;
    while (1)
    {
        if (kill(ffmpeg_pid, 0) == -1 && errno == ESRCH)
        {
            char temp_dir[64];
            snprintf(temp_dir, sizeof(temp_dir), "%s/tmp", base_path);

            DIR *dir;
            struct dirent *entry;

            if (!(dir = opendir(temp_dir)))
                return -1;

            while ((entry = readdir(dir)) != NULL)
            {
                if (entry->d_type == DT_REG)
                {
                    // fprintf(log, "Incomplete: %s\n", path);
                    encrypt_file(base_path, entry->d_name);
                }
            }
            closedir(dir);

            // fprintf(log, "Killed: %d\n", result);
            return 0;
        }

        FILE *fp = fopen(segments_path, "r");
        if (fp)
        {
            char filename[256];
            char absolute_path[512];
            fseek(fp, last_pos, SEEK_SET);

            while (fgets(filename, sizeof(filename), fp) != NULL)
            {
                filename[strcspn(filename, "\n")] = 0;

                if (encrypt_file(base_path, filename) == 0)
                {

                    // fprintf(log, "Encrypted: %s\n", absolute_path);
                }
            }

            last_pos = ftell(fp);
            fclose(fp);
        }

        sleep(1);
    }
    return 0;
}