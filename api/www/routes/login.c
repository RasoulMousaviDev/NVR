#include "/home/rasoul/Projects/NVR/api/include/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void generate_token(char *buf, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    for (size_t i = 0; i < size - 1; i++)
    {
        buf[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    buf[size - 1] = '\0';
}

int main(void)
{
    char *method = getenv("REQUEST_METHOD");
    if (!method || strcmp(method, "POST") != 0)
    {
        printf("Status: 405 Method Not Allowed\r\n\r\n");
        return 0;
    }

    int content_length = atoi(getenv("CONTENT_LENGTH"));
    char *body = malloc(content_length + 1);
    fread(body, 1, content_length, stdin);
    body[content_length] = '\0';

    char username[64] = {0};
    char password[64] = {0};
    sscanf(body, "username=%63[^&]&password=%63s", username, password);
    free(body);

    const char *env_user = getenv("LOGIN_USER");
    const char *env_pass = getenv("LOGIN_PASS");

    if (strcmp(username, env_user) != 0 || strcmp(password, env_pass) != 0)
    {
        printf("Status: 401 Unauthorized\r\n");
        printf("Content-Type: application/json\r\n\r\n");
        printf("{\"ok\":false,\"error\":\"invalid credentials\"}\n");

        logger("User login failed");

        return 0;
    }

    srand(time(NULL));
    char token[64];
    generate_token(token, sizeof(token));

    FILE *fp = fopen("/tmp/auth_token", "w");
    if (fp)
    {
        fprintf(fp, "%s", token);
        fclose(fp);
    }

    printf("Status: 200 OK\r\n");
    printf("Content-Type: application/json\r\n");
    printf("Set-Cookie: auth_token=%s; HttpOnly; Path=/; Max-Age=3600\r\n", token);
    printf("\r\n");
    printf("{\"ok\":true}\n");

    logger("User login success");

    return 0;
}
