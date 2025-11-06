#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <openssl/evp.h>
#include <sys/stat.h>

static int file_exists(const char *p)
{
    struct stat s;
    return stat(p, &s) == 0 && S_ISREG(s.st_mode);
}

static unsigned char *base64_decode_str(const char *in, size_t *out_len)
{
    size_t il = strlen(in);
    char *clean = malloc(il + 1);
    if (!clean)
        return NULL;
    size_t ci = 0;
    for (size_t i = 0; i < il; i++)
    {
        char c = in[i];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            continue;
        clean[ci++] = c;
    }
    clean[ci] = 0;
    int max_out = (int)(ci * 3 / 4 + 4);
    unsigned char *out = malloc(max_out);
    if (!out)
    {
        free(clean);
        return NULL;
    }
    int decoded = EVP_DecodeBlock(out, (const unsigned char *)clean, (int)ci);
    free(clean);
    if (decoded < 0)
    {
        free(out);
        return NULL;
    }
    /* EVP_DecodeBlock may include padding bytes; compute real length by removing '=' padding */
    int pad = 0;
    if (ci >= 1 && in[ci - 1] == '=')
        pad++;
    if (ci >= 2 && in[ci - 2] == '=')
        pad++;
    *out_len = (size_t)(decoded - pad);
    return out;
}

static int read_key(unsigned char **out_key, size_t *out_len)
{
    FILE *f = fopen(".env", "r");
    if (!f)
    {
        fprintf(stderr, "error: can't open .env\n");
        return 0;
    }
    char line[8192];
    char *val = NULL;
    while (fgets(line, sizeof(line), f))
    {
        if (strncmp(line, "AES_KEY=", 8) == 0)
        {
            val = strchr(line, '=') + 1;
            break;
        }
    }
    fclose(f);
    if (!val)
    {
        fprintf(stderr, "error: AES_KEY not found in .env\n");
        return 0;
    }
    while (*val == ' ' || *val == '\t')
        val++;
    size_t L = strcspn(val, "\r\n");
    val[L] = 0;
    if (val[0] == '"' && val[strlen(val) - 1] == '"')
    {
        val[strlen(val) - 1] = 0;
        val++;
    }
    size_t klen = 0;
    unsigned char *k = base64_decode_str(val, &klen);
    if (!k)
    {
        fprintf(stderr, "error: base64 decode failed\n");
        return 0;
    }
    if (klen != 32)
    {
        fprintf(stderr, "error: decoded key length %zu != 32\n", klen);
        free(k);
        return 0;
    }
    *out_key = k;
    *out_len = klen;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s /path/to/file.mp4\n", argv[0]);
        return 2;
    }
    char in[PATH_MAX];
    if (strlen(argv[1]) >= PATH_MAX - 8)
    {
        fprintf(stderr, "error: input path too long\n");
        return 3;
    }
    strncpy(in, argv[1], sizeof(in) - 1);
    in[sizeof(in) - 1] = 0;
    char base[PATH_MAX];
    char enc[PATH_MAX];
    char ivp[PATH_MAX];
    char *dot = strrchr(in, '.');
    if (!dot)
    {
        fprintf(stderr, "error: input has no extension\n");
        return 4;
    }
    size_t baselen = (size_t)(dot - in);
    if (baselen >= sizeof(base))
    {
        fprintf(stderr, "error: base path too long\n");
        return 5;
    }
    memcpy(base, in, baselen);
    base[baselen] = 0;
    if (snprintf(enc, sizeof(enc), "%s.enc", base) >= (int)sizeof(enc))
    {
        fprintf(stderr, "error: enc path overflow\n");
        return 6;
    }
    if (snprintf(ivp, sizeof(ivp), "%s.iv", base) >= (int)sizeof(ivp))
    {
        fprintf(stderr, "error: iv path overflow\n");
        return 7;
    }

    if (!file_exists(enc))
    {
        fprintf(stderr, "error: encrypted file not found: %s\n", enc);
        return 8;
    }
    if (!file_exists(ivp))
    {
        fprintf(stderr, "error: iv file not found: %s\n", ivp);
        return 9;
    }

    unsigned char *key = NULL;
    size_t keylen = 0;
    if (!read_key(&key, &keylen))
        return 10;

    FILE *fenc = fopen(enc, "rb");
    if (!fenc)
    {
        fprintf(stderr, "error: cannot open %s\n", enc);
        free(key);
        return 11;
    }
    FILE *fiv = fopen(ivp, "rb");
    if (!fiv)
    {
        fprintf(stderr, "error: cannot open %s\n", ivp);
        fclose(fenc);
        free(key);
        return 12;
    }
    unsigned char iv[16];
    size_t r = fread(iv, 1, 16, fiv);
    fclose(fiv);
    if (r != 16)
    {
        fprintf(stderr, "error: iv size %zu != 16\n", r);
        fclose(fenc);
        free(key);
        return 13;
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
    {
        fprintf(stderr, "error: EVP_CIPHER_CTX_new\n");
        fclose(fenc);
        free(key);
        return 14;
    }
    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
    {
        fprintf(stderr, "error: EVP_DecryptInit_ex\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fenc);
        free(key);
        return 15;
    }

    unsigned char inbuf[8192];
    unsigned char outbuf[8192 + EVP_MAX_BLOCK_LENGTH];
    int inl = 0, outl = 0;
    while ((inl = (int)fread(inbuf, 1, sizeof(inbuf), fenc)) > 0)
    {
        if (1 != EVP_DecryptUpdate(ctx, outbuf, &outl, inbuf, inl))
        {
            fprintf(stderr, "error: decrypt update\n");
            EVP_CIPHER_CTX_free(ctx);
            fclose(fenc);
            free(key);
            return 16;
        }
        if (outl > 0)
        {
            if (fwrite(outbuf, 1, outl, stdout) != (size_t)outl)
            {
                fprintf(stderr, "error: write stdout failed\n");
                EVP_CIPHER_CTX_free(ctx);
                fclose(fenc);
                free(key);
                return 17;
            }
        }
    }
    if (ferror(fenc))
    {
        fprintf(stderr, "error: read enc file\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fenc);
        free(key);
        return 18;
    }
    if (1 != EVP_DecryptFinal_ex(ctx, outbuf, &outl))
    {
        fprintf(stderr, "error: decrypt final - bad key or corrupted\n");
        EVP_CIPHER_CTX_free(ctx);
        fclose(fenc);
        free(key);
        return 19;
    }
    if (outl > 0)
        fwrite(outbuf, 1, outl, stdout);
    fflush(stdout);
    EVP_CIPHER_CTX_free(ctx);
    fclose(fenc);
    free(key);
    return 0;
}
