/* xcloud client
   Copyright (c) 2023 wloodar */

#include "client.h"

#include "common.h"
#include "gui.h"
#include "xcp.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

char config_path[64];
char config_path_username[64];

int main(int argc, char **argv)
{
    if (argc < 2) {
        die("Missing argument: <host>");
    }

    // Setup crucial parts of client
    initialize();

    char *username = get_username();

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
        char *command = gui_input();

        if (command[0] == 0) {
            free(command);
            continue;
        }

        if (!strcmp(command, "/users")) {
            list_active_users(sock, username);
        } else if (!strcmp(command, "/commands")) {
            show_commands_list(username);
        } else {
            char empty[256];
            xcp_packet_reply reply = send_message(sock, empty, command);
            if (reply.status != XS_OK) {
                printf("Message not sent.\n");
            }
        }

        free(command);
    }

    return 0;
}

void show_commands_list(char *username)
{
    gui_write_line("|----------------------------------------");
    gui_write_line("|");
    gui_write_line("|   Hello, %s! Here's a list of available "
                   "commands:",
                   username);
    gui_write_line("|");
    gui_write_line("|----------------------------------------");
    gui_write_line("|");
    gui_write_line("|   users       -   Show active users");
    gui_write_line("|   commands    -   List of all available commands");
    gui_write_line("|");
    gui_write_line("|----------------------------------------");
    gui_write_line("");
}

void initialize()
{
    char *envpath = getenv("HOME");
    snprintf(config_path, 64, "%s/.config", envpath);
    snprintf(config_path_username, 64, "%s/%s", config_path, "username");

    // Create xcloud config directory
    mkdir(config_path, 0744);

    gui_open();
}

struct l_thread
{
    char *username;
    char *host;
};

void listening_thread(char *host, char *username)
{
    struct l_thread *args = malloc(sizeof(*args));
    args->host = host;
    args->username = username;

    pthread_t listen_thread_id;
    pthread_create(&listen_thread_id, NULL, listening, args);
}

void *listening(void *_args)
{
    struct l_thread *args = _args;
    struct sockaddr_in addr;
    int l_sock;

    l_sock = socket(AF_INET, SOCK_STREAM, 0);

    addr.sin_addr.s_addr = inet_addr(args->host);
    addr.sin_family = AF_INET;
    addr.sin_port = htons(XCP_SEVER_PORT);

    if (connect(l_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        die("Couldn't connect to the xcloud server.");
    }

    xcp_packet_reply hello_reply =
        send_hello(l_sock, args->username, XCP_CONVERT);
    if (hello_reply.status != XS_OK) {
        die("Couldn't connect to the xcloud server.");
    }

    while (1) {
        xcp_packet_header l_header;
        if (read(l_sock, &l_header, sizeof(l_header)) == 0) {
            break;
        }

        xcp_packet_sendmsg p_message_info;
        read(l_sock, &p_message_info, sizeof(p_message_info));

        char *message = malloc(p_message_info.message_len + 1);
        read(l_sock, message, p_message_info.message_len);

        message[p_message_info.message_len] = 0;

        pthread_mutex_lock(get_console_lock());
        if (!strcmp(p_message_info.dest, args->username)) {
            gui_write_line("[\033[93m%s\033[0m]: %s", p_message_info.dest,
                           message);
        } else {
            gui_write_line("[\033[32m%s\033[0m]: %s", p_message_info.dest,
                           message);
        }
        pthread_mutex_unlock(get_console_lock());
    }

    close(l_sock);
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
    printf("Please set your username: ");
    char *name = gui_input();
    rstrip(name);

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

void list_active_users(int sock, char *username)
{
    strlist users = get_active_users(sock, username);

    if (users.len == 0) {
        gui_write_line("There's no active users.");
        return;
    }

    gui_write_line("Active users:");
    for (int i = 0; i < users.len; i++) {
        gui_write_line("%s", users.strings[i]);
    }
}

strlist get_active_users(int sock, char *username)
{
    send_header(sock, XCP_LISTUSERS);

    int amount;
    strlist users = {.strings = NULL, .len = 0};

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

pthread_mutex_t *get_console_lock()
{
    static pthread_mutex_t mut;
    return &mut;
}
