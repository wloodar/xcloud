/* xcloud client
   Copyright (c) 2023 wloodar */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

#include "common.h"
#include "xcp.h"
#include "client.h"

char config_path[64];
char config_path_username[64];

int main(int argc, char **argv)
{  
    // Setup crucial parts of client
    initialize();

    char* username = get_username();
    
    // Setup socket for client <-> server communication
    struct sockaddr_in addr;
    int sock;

    if (argc < 2) {
        die("Missing argument: <host>");
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(XCP_SEVER_PORT);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        die("Couldn't connect to the xcloud server.");
    }

    xcp_packet_reply hello_reply = send_hello(sock, username);
    if (hello_reply.status == XS_NAMETAKEN) {
        die("The username \"%s\" is already taken.", username);
    }

    strlist users = get_active_users(sock);
    for (int i = 0; i < users.len; i++) {
        printf("%s\n", users.strings[i]);
    }

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
    rstrip(name);

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

xcp_packet_reply send_hello(int sock, char *username) 
{
    send_header(sock, XCP_HELLO);
    
    xcp_packet_hello p_hello;
    strcpy(p_hello.username, username);

    write(sock, &p_hello, sizeof(p_hello));

    xcp_packet_reply reply;
    read(sock, &reply, sizeof(reply));

    return reply;
}

strlist get_active_users(int sock) {
    send_header(sock, XCP_LISTUSERS);

    int amount;
    strlist users;
    read(sock, &amount, sizeof(int));

    for (int i = 0; i < amount; i++) {
        char *buf = malloc(256);
        read(sock, buf, 256);
        strlist_append(&users, buf);
    }

    return users;
}
