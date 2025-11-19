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

#define CHUNK_SIZE 1024
#define ENC_SUFFIX ".enc"
#define LOG_FILE "/mnt/mmcblk0p1/monitor_log.txt"
#define VIDEO_BASE_PATH "/mnt/mmcblk0p1/videos"
#define AES_KEY_LEN 16
#define AES_IV_LEN 16

extern void AES128_CBC_encrypt_buffer(uint8_t *output, uint8_t *input, size_t length, const uint8_t *key, const uint8_t *iv);

void hex_to_bin(const char *hex_str, unsigned char *bin_buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        sscanf(hex_str + 2 * i, "%2hhx", &bin_buf[i]);
    }
}

void create_parent_dir(const char *path)
{
    char dir_path[256];
    char *last_slash = strrchr(path, '/');

    if (last_slash != NULL)
    {
        size_t len = last_slash - path;
        strncpy(dir_path, path, len);
        dir_path[len] = '\0';

        char mkdir_cmd[300];
        snprintf(mkdir_cmd, sizeof(mkdir_cmd), "/bin/mkdir -p %s", dir_path);
        system(mkdir_cmd);
    }
}

int aes_encrypt_file(const char *input_path, const char *key_hex)
{
    unsigned char key[AES_KEY_LEN];
    unsigned char iv[AES_IV_LEN] = {0};
    char output_path[512];
    char relative_filename[256];
    const char *filename_ptr;

    filename_ptr = strrchr(input_path, '/');
    if (filename_ptr == NULL)
        return -1;
    filename_ptr++;
    strncpy(relative_filename, filename_ptr, sizeof(relative_filename) - 4);
    relative_filename[sizeof(relative_filename) - 1] = '\0';

    int Y, M, D, h, m, s;

    if (sscanf(relative_filename, "%4d-%2d-%2d-%2d-%2d-%2d", &Y, &M, &D, &h, &m, &s) != 6)
        return -1;

    snprintf(output_path, sizeof(output_path),
             "%s/%04d/%02d/%02d/%02d/%02d-%02d%s",
             VIDEO_BASE_PATH, Y, M, D, h, m, s, ENC_SUFFIX);

    hex_to_bin(key_hex, key, AES_KEY_LEN);

    create_parent_dir(output_path);

    FILE *in = fopen(input_path, "rb");
    FILE *out = fopen(output_path, "wb");

    if (in && out)
    {
        unsigned char buffer[CHUNK_SIZE];
        unsigned char enc_buffer[CHUNK_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, in)) > 0)
        {

            memcpy(enc_buffer, buffer, bytes_read);

            fwrite(enc_buffer, 1, bytes_read, out);
        }

        fclose(in);
        fclose(out);
        remove(input_path);
        return 0;
    }
    if (in)
        fclose(in);
    if (out)
        fclose(out);
    return -1;
}

int process_directory(const char *dir_name, const char *key_hex)
{
    DIR *dir;
    struct dirent *entry;
    char path[1024];

    if (!(dir = opendir(dir_name)))
        return -1;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            snprintf(path, sizeof(path), "%s/%s", dir_name, entry->d_name);
            FILE *log = fopen(LOG_FILE, "a");
            fprintf(log, "Incomplete: %s\n", path);
            fclose(log);
            aes_encrypt_file(path, key_hex);
        }
    }
    closedir(dir);

    return 0;
}

pid_t ffmpeg_pid = 0;

void handle_signal(int sig)
{
    if (ffmpeg_pid > 0)
    {
        kill(ffmpeg_pid, SIGTERM);
    }
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        return 1;
    }

    const char *list_file = argv[1];
    const char *key_hex = argv[2];
    ffmpeg_pid = atoi(argv[3]);

    FILE *log = fopen(LOG_FILE, "a");
    if (log)
    {
        fprintf(log, "Monitor started for PID %d\n", ffmpeg_pid);
        fclose(log);
    }

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    long int last_pos = 0;
    while (1)
    {
        if (kill(ffmpeg_pid, 0) == -1 && errno == ESRCH)
        {
            int result = process_directory(VIDEO_BASE_PATH, key_hex);

            if (log)
            {
                log = fopen(LOG_FILE, "a");
                fprintf(log, "Killed: %d\n", result);
                fclose(log);
            }
            return 0;
        }

        FILE *fp = fopen(list_file, "r");
        if (fp)
        {
            char filename[256];
            char absolute_path[512];
            fseek(fp, last_pos, SEEK_SET);

            while (fgets(filename, sizeof(filename), fp) != NULL)
            {
                filename[strcspn(filename, "\n")] = 0;

                snprintf(absolute_path, sizeof(absolute_path), "%s/%s", VIDEO_BASE_PATH, filename);

                if (aes_encrypt_file(absolute_path, key_hex) == 0)
                {

                    if (log)
                    {
                        log = fopen(LOG_FILE, "a");
                        fprintf(log, "Encrypted: %s\n", absolute_path);
                        fclose(log);
                    }
                }
            }

            last_pos = ftell(fp);
            fclose(fp);
        }

        sleep(1);
    }
    return 0;
}