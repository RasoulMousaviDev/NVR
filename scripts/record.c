#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <dirent.h>
#include <signal.h>
#include <gst/gst.h>

#define ENV_FILE ".env"
#define KEY_VAR "AES_KEY"
#define IV_SIZE 16

static volatile sig_atomic_t stop_flag = 0;

void handle_sig(int sig)
{
    (void)sig;
    stop_flag = 1;
}

unsigned char *base64_decode(const char *b64, int *out_len)
{
    BIO *bio, *b64bio;
    int len = (int)strlen(b64);
    unsigned char *buffer = malloc(len);
    if (!buffer)
        return NULL;
    bio = BIO_new_mem_buf((void *)b64, len);
    b64bio = BIO_new(BIO_f_base64());
    BIO_set_flags(b64bio, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64bio, bio);
    *out_len = BIO_read(bio, buffer, len);
    BIO_free_all(bio);
    return buffer;
}

unsigned char *read_aes_key(const char *path, const char *var, int *key_len)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;
    char line[1024];
    char *key = NULL;
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, var, strlen(var)) == 0 && line[strlen(var)] == '=')
        {
            key = strdup(line + strlen(var) + 1);
            key[strcspn(key, "\r\n")] = 0;
            break;
        }
    }
    fclose(f);
    if (!key)
        return NULL;
    if (key[0] == '"' && key[strlen(key) - 1] == '"')
    {
        key[strlen(key) - 1] = 0;
        memmove(key, key + 1, strlen(key));
    }
    unsigned char *decoded = base64_decode(key, key_len);
    free(key);
    return decoded;
}

void make_storage_path(const char *storage, const char *name, char *out_path, size_t size)
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    snprintf(out_path, size, "%s/%s/%04d/%02d/%02d/%02d",
             storage, name,
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour);
    char tmp[PATH_MAX];
    strncpy(tmp, out_path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = 0;
    for (char *p = tmp + strlen(storage) + 1; *p; p++)
    {
        if (*p == '/')
        {
            *p = 0;
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

int generate_iv(const char *path, unsigned char *iv, size_t size)
{
    if (RAND_bytes(iv, size) != 1)
        return 0;
    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    fwrite(iv, 1, size, f);
    fclose(f);
    return 1;
}

int encrypt_file(const char *infile, const char *outfile, unsigned char *key, unsigned char *iv)
{
    FILE *fin = fopen(infile, "rb");
    FILE *fout = fopen(outfile, "wb");
    if (!fin || !fout)
    {
        if (fin)
            fclose(fin);
        if (fout)
            fclose(fout);
        return 0;
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fclose(fin);
        fclose(fout);
        return 0;
    }
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        fclose(fin);
        fclose(fout);
        return 0;
    }
    unsigned char inbuf[8192], outbuf[8192 + EVP_MAX_BLOCK_LENGTH];
    int inlen, outlen;
    while ((inlen = (int)fread(inbuf, 1, sizeof(inbuf), fin)) > 0)
    {
        if (EVP_EncryptUpdate(ctx, outbuf, &outlen, inbuf, inlen) != 1)
        {
            EVP_CIPHER_CTX_free(ctx);
            fclose(fin);
            fclose(fout);
            return 0;
        }
        fwrite(outbuf, 1, outlen, fout);
    }
    if (EVP_EncryptFinal_ex(ctx, outbuf, &outlen) != 1)
    {
        EVP_CIPHER_CTX_free(ctx);
        fclose(fin);
        fclose(fout);
        return 0;
    }
    fwrite(outbuf, 1, outlen, fout);
    EVP_CIPHER_CTX_free(ctx);
    fclose(fin);
    fclose(fout);
    return 1;
}

int file_stable(const char *path)
{
    struct stat s1, s2;
    if (stat(path, &s1) != 0)
        return 0;
    sleep(1);
    if (stat(path, &s2) != 0)
        return 0;
    return s1.st_size == s2.st_size;
}

int process_one_tmp(const char *dirpath, const char *fname, unsigned char *key, int duration)
{
    char tmpfile[PATH_MAX], encfile[PATH_MAX], ivfile[PATH_MAX];
    snprintf(tmpfile, sizeof(tmpfile), "%s/%s", dirpath, fname);

    if (!file_stable(tmpfile))
        return 0;

    time_t t = time(NULL) - duration;
    struct tm tm = *localtime(&t);
    int mm = tm.tm_min;
    int ss = tm.tm_sec;

    snprintf(encfile, sizeof(encfile), "%s/%02d-%02d.enc", dirpath, mm, ss);
    snprintf(ivfile, sizeof(ivfile), "%s/%02d-%02d.iv", dirpath, mm, ss);

    unsigned char iv[IV_SIZE];
    if (!generate_iv(ivfile, iv, IV_SIZE))
        return 0;

    if (!encrypt_file(tmpfile, encfile, key, iv))
    {
        remove(ivfile);
        return 0;
    }

    remove(tmpfile);
    return 1;
}

int monitor_and_encrypt_once(const char *path, unsigned char *key, int duration)
{
    DIR *d = opendir(path);
    if (!d)
        return 0;
    struct dirent *dir;
    int processed = 0;
    while ((dir = readdir(d)) != NULL)
    {
        if (dir->d_type == DT_REG && strstr(dir->d_name, ".tmp"))
        {
            if (process_one_tmp(path, dir->d_name, key, duration))
                processed++;
        }
    }
    closedir(d);
    return processed;
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        printf("Usage: %s <name> <rtsp> <duration_sec> <storage>\n", argv[0]);
        return 1;
    }
    const char *name = argv[1];
    const char *rtsp = argv[2];
    int duration = atoi(argv[3]);
    const char *storage = argv[4];

    int key_len;
    unsigned char *key = read_aes_key(ENV_FILE, KEY_VAR, &key_len);
    if (!key || key_len != 32)
    {
        fprintf(stderr, "AES-256 key read failed\n");
        return 1;
    }

    char path[PATH_MAX];
    make_storage_path(storage, name, path, sizeof(path));

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sig;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    gst_init(NULL, NULL);

    char *pipeline = g_strdup_printf(
        "rtspsrc location=%s ! rtph264depay ! h264parse ! "
        "splitmuxsink location=%s/%%2d.tmp max-size-time=%ld muxer=mp4mux",
        rtsp, path, (long)duration * 1000000000L);

    GError *error = NULL;
    GstElement *pipeline_el = gst_parse_launch(pipeline, &error);
    if (!pipeline_el)
    {
        fprintf(stderr, "gst_parse_launch failed: %s\n", error ? error->message : "unknown");
        if (error)
            g_error_free(error);
        free(key);
        return 1;
    }

    GstBus *bus = gst_element_get_bus(pipeline_el);
    gst_element_set_state(pipeline_el, GST_STATE_PLAYING);

    while (!stop_flag)
    {
        GstMessage *msg = gst_bus_timed_pop_filtered(bus, 1000000000LL, GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_STATE_CHANGED);
        if (msg)
        {
            if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR)
            {
                GError *err = NULL;
                gst_message_parse_error(msg, &err, NULL);
                fprintf(stderr, "GStreamer error: %s\n", err ? err->message : "unknown");
                if (err)
                    g_error_free(err);
                gst_message_unref(msg);
                break;
            }
            else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS)
            {
                gst_message_unref(msg);
                break;
            }
            else
            {
                gst_message_unref(msg);
            }
        }
        monitor_and_encrypt_once(path, key, duration);
    }

    gst_element_set_state(pipeline_el, GST_STATE_NULL);
    gst_object_unref(bus);
    gst_object_unref(pipeline_el);

    for (int i = 0; i < 30; ++i)
    {
        int n = monitor_and_encrypt_once(path, key, duration);
        if (n == 0)
            break;
        sleep(1);
    }

    free(key);
    return 0;
}
