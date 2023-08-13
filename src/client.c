/* xcloud client
   Copyright (c) 2023 wloodar */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

#include "xcp.h"
#include "client.h"

char config_path[64];
char config_path_username[64];

int main(int argc, char **argv)
{  
    initialize();

    char* username = get_username();
    printf(username);

    return 0;
}

void initialize() {
    char *envpath = getenv("HOME");
    snprintf(config_path, 64, "%s/.config", envpath);
    snprintf(config_path_username, 64, "%s/%s", config_path, "username");
    
    // Create xcloud config directory
    mkdir(config_path, 0744);
}

char *get_username()
{
    FILE *f = fopen(config_path_username, "r");
    if (f == NULL) {
        return set_username();
    }

    char *name = malloc(256);
    fgets(name, 256, f);

    fclose(f);

    return name;
}

char *set_username()
{
    char *name = malloc(256);
    
    printf("Please set your username: ");
    fgets(name, 256, stdin);

    FILE *fr = fopen(config_path_username, "w");
    fputs(name, fr);

    fclose(fr);

    return name;
}