#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define ENDPOINT_BASE_DIR "/mnt/mmcblk0p1/www/routes/"
#define MAX_PATH_LEN 128
#define MAX_ARGS 10

void print_json_error(int http_code, const char *message, const char *detail);
void setup_cgi_headers();

int main() {
    char *path_info = getenv("PATH_INFO");
    char full_endpoint_path[MAX_PATH_LEN];
    char path_copy[MAX_PATH_LEN];
    char *token;
    char *saveptr;
    char *argv[MAX_ARGS];
    int argc = 0;
    
    setup_cgi_headers();

    if (!path_info || strlen(path_info) < 2) {
        print_json_error(400, "Error", "API path not specified.");
        return 0;
    }

    strncpy(path_copy, path_info, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    token = strtok_r(path_copy, "/", &saveptr); 

    if (!token) {
        print_json_error(404, "Error", "Invalid API path format.");
        return 0;
    }

    snprintf(full_endpoint_path, sizeof(full_endpoint_path), "%s%s", ENDPOINT_BASE_DIR, token);
    
    argv[argc++] = full_endpoint_path; 
    
    while ((token = strtok_r(NULL, "/", &saveptr)) != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
    }
    argv[argc] = NULL;
    execv(full_endpoint_path, argv);

    if (errno == ENOENT) {
        print_json_error(404, "Error", "Endpoint handler not found or is not executable.");
    } else {
        print_json_error(500, "Internal Server Error", strerror(errno));
    }
    
    return 1;
}

void setup_cgi_headers() {
    printf("Content-type: application/json\r\n");
    printf("Status: 200 OK\r\n\r\n");
}

void print_json_error(int http_code, const char *message, const char *detail) {
    printf("{\"status\": \"%s\", \"code\": %d, \"message\": \"%s\"}", message, http_code, detail);
}