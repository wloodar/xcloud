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
#include <pthread.h>

#include "common.h"
#include "xcp.h"
#include "client.h"

char config_path[64];
char config_path_username[64];

int main(int argc, char **argv)
{  
    if (argc < 2) {
        die("Missing argument: <host>");
    }

    // Setup crucial parts of client
    initialize();

    char* username = get_username();

    listening_thread(argv[1], username);

    // Setup socket for client <-> server communication
    struct sockaddr_in addr;
    int sock;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(XCP_SEVER_PORT);

    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        die("Couldn't connect to the xcloud server.");
    }

    xcp_packet_reply hello_reply = send_hello(sock, username, XCP_HELLO);
    if (hello_reply.status == XS_NAMETAKEN) {
        die("The username \"%s\" is already taken.", username);
    }

    show_commands_list(username);
    while (1) {
        printf("\n> ");
        char command[256];
        fgets(command, 256, stdin);
        rstrip(command);

        printf("\n");

        if (!strcmp(command, "users")) {
            list_active_users(sock, username);
        }

        if (!strncmp(command, "send", 4)) {
            xcp_packet_reply reply = send_message(sock, command, command + 5);
            if (reply.status == XS_OK) {
                printf("Message successfully sent.\n");
            }
        }

        if (!strcmp(command, "commands")) {
            show_commands_list(username);
        }
    }

    return 0;
}

void show_commands_list(char *username) 
{
    printf("|----------------------------------------\n|\n");
    printf("|   Hello, %s! Here's a list of available commands:\n|\n|----------------------------------------\n|\n", username);
    printf("|   users       -   Show active users\n");
    printf("|   send <msg>  -   Send message\n");
    printf("|   commands    -   List of all available commands\n");
    printf("|\n|----------------------------------------\n\n");
}

void initialize()
{
    char *envpath = getenv("HOME");
    snprintf(config_path, 64, "%s/.config", envpath);
    snprintf(config_path_username, 64, "%s/%s", config_path, "username");
    
    // Create xcloud config directory
    mkdir(config_path, 0744);
}

struct l_thread {
    char *username;
    char *host;
};

void listening_thread(char *host, char *username)
{
    struct l_thread * args = malloc(sizeof(*args));
    args->host = host;
    args->username = username;

    pthread_t listen_thread_id;
    pthread_create(&listen_thread_id, NULL, listening, args);
}

void *listening(void *_args)
{
    struct l_thread * args = _args;
    struct sockaddr_in addr;
    int l_sock;

    l_sock = socket(AF_INET, SOCK_STREAM, 0);
    
    addr.sin_addr.s_addr = inet_addr(args->host);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(XCP_SEVER_PORT);

    if (connect(l_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        die("Couldn't connect to the xcloud server.");
    }

    xcp_packet_reply hello_reply = send_hello(l_sock, args->username, XCP_CONVERT);
    if (hello_reply.status != XS_OK) {
        die("Couldn't connect to the xcloud server.");
    }

    while(1) {
        xcp_packet_header l_header;
        read(l_sock, &l_header, sizeof(l_header));

        xcp_packet_sendmsg p_message_info;
        read(l_sock, &p_message_info, sizeof(p_message_info));

        char *message = malloc(p_message_info.message_len + 1);
        read(l_sock, message, p_message_info.message_len);

        message[p_message_info.message_len] = 0;
        printf("[%s]: %s\n\n", p_message_info.dest, message);
    }
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

xcp_packet_reply send_hello(int sock, char *username, xcp_packet_type type) 
{
    send_header(sock, type);
    
    xcp_packet_hello p_hello;
    strcpy(p_hello.username, username);

    write(sock, &p_hello, sizeof(p_hello));

    xcp_packet_reply reply;
    read(sock, &reply, sizeof(reply));

    return reply;
}

void list_active_users(int sock, char *username) {
    strlist users = get_active_users(sock, username);

    if (users.len == 0) {
        printf("There's no active users.\n");
        return;
    }

    printf("Active users:\n");
    for (int i = 0; i < users.len; i++) {
        printf("%s\n", users.strings[i]);
    }
}

strlist get_active_users(int sock, char *username)
{
    send_header(sock, XCP_LISTUSERS);

    int amount;
    strlist users = {
        .strings = NULL,
        .len = 0
    };

    read(sock, &amount, sizeof(int));

    for (int i = 0; i < amount; i++) {
        char *buf = malloc(256);
        read(sock, buf, 256);

        if (!strcmp(buf, username)) {
            continue;
        }

        strlist_append(&users, buf);
    }

    return users;
}

xcp_packet_reply send_message(int sock, char dest[256], char *message)
{
    send_header(sock, XCP_SENDMSG);

    xcp_packet_sendmsg p_sendmsg;
    strcpy(p_sendmsg.dest, dest);
    p_sendmsg.message_len = strlen(message);

    write(sock, &p_sendmsg, sizeof(p_sendmsg));
    write(sock, message, strlen(message));

    xcp_packet_reply reply;
    read(sock, &reply, sizeof(reply));

    return reply;
}