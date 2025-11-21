#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int camera_find(const char *id, char *camera)
{
    char file_path[1024];
    snprintf(file_path, sizeof(file_path), "%s/www/cameras/cameras.txt", getenv("BASE_PATH"));

    FILE *fp = fopen(file_path, "a+");
    char line[1024];

    while (fgets(line, sizeof(line), fp))
    {
        line[strcspn(line, "\n")] = 0;

        if (strncmp(line, "id=", 3) == 0)
        {
            const char *id_value = line + 3;

            if (strcmp(id_value, id) == 0)
            {
                strncpy(camera, line, sizeof(line));
                break;
            }
        }
    }

    fclose(fp);
    return 1;
}

int camera_get(const char *id, char *key, char *value)
{
    char camera[1024];

    if (camera_find(id, camera))
        return 1;

    char query[32];
    snprintf(query, sizeof(query), "%s=%127[^&]", key);
    
    sscanf(camera, query, value);

    return 0;
}