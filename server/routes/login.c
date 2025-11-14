#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Content-type: application/json\r\n\r\n");
    printf("{\"status\": \"OK\", \"message\": \"API is running on OpenIPC\"}");
    return 0;
}