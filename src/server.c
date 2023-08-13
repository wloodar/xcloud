/* xcloud server
   Copyright (c) 2023 bellrise */

#include "xcp.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

struct client_data
{
    struct sockaddr_in addr;
    socklen_t len;
    int sock;
    pthread_t thread;
    int user_id;
};

static void info(const char *fmt, ...);
static void die(const char *fmt, ...);
static int open_server_port(const char *ip);
static void accept_client(int server_fd);
static void *serve_client(void *);
static pthread_mutex_t *get_console_lock();

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s <ipaddr>\n", argv[0]);
        exit(1);
    }

    srand(time(NULL));

    /* Open the server port & start listening. */

    int server;

    server = open_server_port(argv[1]);

    info("waiting for connections");

    while (1) {
        accept_client(server);
    }
}

void accept_client(int server_fd)
{
    struct client_data *client;

    client = calloc(sizeof(struct client_data), 1);
    client->sock =
        accept(server_fd, (struct sockaddr *) &client->addr, &client->len);

    pthread_create(&client->thread, NULL, serve_client, client);
}

struct user
{
    const char *name;
    int id;
};

struct userlist
{
    struct user *users;
    int nusers;
};

static struct userlist users = {NULL, 0};

int user_count()
{
    int n = 0;

    for (int i = 0; i < users.nusers; i++) {
        if (users.users[i].name)
            n++;
    }

    return n;
}

struct user *user_add(const char *name)
{
    struct user *user;

    users.users =
        realloc(users.users, sizeof(struct user) * (users.nusers + 1));
    user = &users.users[users.nusers++];

    user->name = strdup(name);
    do {
        user->id = rand();
    } while (!user->id);

    return user;
}

void user_del(int id)
{
    for (int i = 0; i < users.nusers; i++) {
        if (!users.users[i].name)
            continue;
        if (users.users[i].id == id)
            users.users[i].name = NULL;
    }
}

struct user *user_find(int id)
{
    for (int i = 0; i < users.nusers; i++) {
        if (!users.users[i].name)
            continue;
        if (users.users[i].id == id)
            return &users.users[i];
    }

    return NULL;
}

/* Returns user id or 0 if not found. */
int user_id(const char *name)
{
    for (int i = 0; i < users.nusers; i++) {
        if (!users.users[i].name)
            continue;
        if (!strcmp(users.users[i].name, name))
            return users.users[i].id;
    }

    return 0;
}

void handle_hello(struct client_data *client)
{
    xcp_packet_reply reply;
    xcp_packet_hello packet;
    size_t bytes;
    int user;

    /* Read the rest */
    bytes = sizeof(xcp_packet_hello);
    if (read(client->sock, &packet, bytes) != bytes)
        return;

    user = user_id(packet.username);

    /* Not logged in */
    if (user == 0) {
        client->user_id = user_add(packet.username)->id;
        info("[%d] client connected as `%s`", client->user_id, packet.username);
        reply.status = XS_OK;
    } else {
        info("client requested existing name `%s`", packet.username);
        reply.status = XS_NAMETAKEN;
    }

    write(client->sock, &reply, sizeof(reply));
}

void handle_listusers(struct client_data *client)
{
    info("[%d] LISTUSERS", client->user_id);

    char buf[256];
    int n = user_count();

    info("[%d] sending %d users", client->user_id, user_count());

    /* amount */
    write(client->sock, &n, sizeof(int));

    /* user names */
    for (int i = 0; i < users.nusers; i++) {
        if (!users.users[i].name)
            continue;

        memset(buf, 0, 256);
        strcpy(buf, users.users[i].name);
        write(client->sock, buf, 256);
    }
}

void *serve_client(void *_client_data)
{
    struct client_data *client = _client_data;

    info("client connected");

    while (1) {
        xcp_packet_header header;
        size_t bytes = sizeof(xcp_packet_header);

        /* Read header */
        if (read(client->sock, &header, bytes) != bytes)
            goto disconnect;

        switch (header.type) {
        case XCP_HELLO:
            handle_hello(client);
            break;
        case XCP_LISTUSERS:
            handle_listusers(client);
            break;
        default:
            goto disconnect;
        }
    }

disconnect:
    if (client->user_id) {
        info("[%d] disconnect `%s`", client->user_id,
             user_find(client->user_id)->name);
        user_del(client->user_id);
    } else {
        info("disconnect unnamed");
    }

    close(client->sock);
    return NULL;
}

int open_server_port(const char *ip)
{
    struct sockaddr_in addr;
    int server;

    addr.sin_addr.s_addr = inet_addr(ip);
    addr.sin_port = htons(XCP_SEVER_PORT);
    addr.sin_family = AF_INET;

    server = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(server, (struct sockaddr *) &addr, sizeof(addr)) == -1)
        die("failed to bind()");

    listen(server, 10);

    return server;
}

void info(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pthread_mutex_lock(get_console_lock());

    printf("[server] ");
    vprintf(fmt, args);
    fputc('\n', stdout);

    pthread_mutex_unlock(get_console_lock());

    va_end(args);
}

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    printf("\033[31m[error] ");
    vprintf(fmt, args);
    printf("\033[0m\n");

    va_end(args);
    exit(1);
}

pthread_mutex_t *get_console_lock()
{
    static pthread_mutex_t mut;
    return &mut;
}
