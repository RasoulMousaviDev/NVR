/* ------------------------- INCLUDES ---------------------------- */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <stdint.h>

/* ------------------------- CONSTANTS ---------------------------- */
#define BUFFER_SIZE 4096

/* ------------------------- HEX TO BYTES ---------------------------- */
static int hex_to_bytes(const char *hex, unsigned char **out, size_t *out_len)
{
    size_t len = strlen(hex);
    if (len % 2 != 0)
        return -1;
    *out_len = len / 2;
    *out = (unsigned char *)malloc(*out_len);
    if (!*out)
        return -1;
    for (size_t i = 0; i < *out_len; ++i)
    {
        unsigned int byte;
        if (sscanf(hex + 2 * i, "%2x", &byte) != 1)
        {
            free(*out);
            return -1;
        }
        (*out)[i] = (unsigned char)byte;
    }
    return 0;
}

/* ------------------------- PRINT_OPENSSL_ERRORS ---------------------------- */
static void print_openssl_errors()
{
    unsigned long e;
    while ((e = ERR_get_error()) != 0)
    {
        char buf[256];
        ERR_error_string_n(e, buf, sizeof(buf));
        fprintf(stderr, "OpenSSL error: %s\n", buf);
    }
}

/* ------------------------- DECRYPT_FILE_TO_STDOUT ---------------------------- */
static int decrypt_file_to_stdout(const char *infile,
                                  const unsigned char *key, size_t key_len,
                                  const unsigned char *iv, size_t iv_len)
{
    FILE *f = fopen(infile, "rb");
    if (!f)
    {
        perror("fopen");
        return -1;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fprintf(stderr, "EVP_CIPHER_CTX_new failed\n");
        fclose(f);
        return -1;
    }

    if (key_len != 32 || iv_len != 16)
    {
        fprintf(stderr, "Invalid key/iv length: key_len=%zu, iv_len=%zu\n", key_len, iv_len);
        EVP_CIPHER_CTX_free(ctx);
        fclose(f);
        return -1;
    }

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
    {
        fprintf(stderr, "EVP_DecryptInit_ex failed\n");
        print_openssl_errors();
        EVP_CIPHER_CTX_free(ctx);
        fclose(f);
        return -1;
    }

    unsigned char inbuf[BUFFER_SIZE];
    unsigned char outbuf[BUFFER_SIZE + EVP_CIPHER_block_size(EVP_aes_256_cbc())];
    int outlen = 0;

    while (1)
    {
        size_t read_bytes = fread(inbuf, 1, BUFFER_SIZE, f);
        if (read_bytes > 0)
        {
            if (EVP_DecryptUpdate(ctx, outbuf, &outlen, inbuf, (int)read_bytes) != 1)
            {
                fprintf(stderr, "EVP_DecryptUpdate failed\n");
                print_openssl_errors();
                EVP_CIPHER_CTX_free(ctx);
                fclose(f);
                return -1;
            }
            if (outlen > 0)
            {
                size_t written = fwrite(outbuf, 1, (size_t)outlen, stdout);
                if (written != (size_t)outlen)
                {
                    perror("fwrite");
                    EVP_CIPHER_CTX_free(ctx);
                    fclose(f);
                    return -1;
                }
            }
        }
        if (read_bytes < BUFFER_SIZE)
        {
            if (feof(f))
                break;
            if (ferror(f))
            {
                perror("fread");
                EVP_CIPHER_CTX_free(ctx);
                fclose(f);
                return -1;
            }
        }
    }

    if (EVP_DecryptFinal_ex(ctx, outbuf, &outlen) != 1)
    {
        fprintf(stderr, "EVP_DecryptFinal_ex failed (bad key/iv or corrupted data)\n");
        print_openssl_errors();
        EVP_CIPHER_CTX_free(ctx);
        fclose(f);
        return -1;
    }
    if (outlen > 0)
    {
        size_t written = fwrite(outbuf, 1, (size_t)outlen, stdout);
        if (written != (size_t)outlen)
        {
            perror("fwrite final");
            EVP_CIPHER_CTX_free(ctx);
            fclose(f);
            return -1;
        }
    }

    fflush(stdout);
    EVP_CIPHER_CTX_free(ctx);
    fclose(f);
    return 0;
}

/* ------------------------- USAGE ---------------------------- */
static void print_usage(const char *prog)
{
    fprintf(stderr,
            "Usage: %s <encrypted-file> <key-hex (64 chars)> <iv-hex (32 chars)>\n"
            "Example: %s secret.mp4 0123... (64 hex) abcdef... (32 hex)\n",
            prog, prog);
}

/* ------------------------- MAIN ---------------------------- */
int main(int argc, char **argv)
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (argc != 4)
    {
        print_usage(argv[0]);
        return 2;
    }
    const char *infile = argv[1];
    const char *keyhex = argv[2];
    const char *ivhex = argv[3];

    unsigned char *key = NULL, *iv = NULL;
    size_t key_len = 0, iv_len = 0;

    if (hex_to_bytes(keyhex, &key, &key_len) != 0)
    {
        fprintf(stderr, "Invalid key hex\n");
        return 3;
    }
    if (hex_to_bytes(ivhex, &iv, &iv_len) != 0)
    {
        fprintf(stderr, "Invalid iv hex\n");
        free(key);
        return 4;
    }

    int rc = decrypt_file_to_stdout(infile, key, key_len, iv, iv_len);

    free(key);
    free(iv);

    EVP_cleanup();
    ERR_free_strings();

    return (rc == 0) ? 0 : 5;
}
