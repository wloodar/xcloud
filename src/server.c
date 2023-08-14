/* xcloud server
   Copyright (c) 2023 bellrise */

#include "xcp.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
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
static void chatinfo(const char *fmt, ...);
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

    /* Stop the sockets from signalling. */
    signal(SIGPIPE, SIG_IGN);

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

struct endpoint
{
    char username[256];
    int sock;
};

struct endpoints
{
    struct endpoint **clients;
    int n_clients;
    pthread_mutex_t lock;
};

static struct endpoints endpoints = {NULL, 0, PTHREAD_MUTEX_INITIALIZER};

void add_client_endpoint(int sock, const char *username)
{
    struct endpoint *p;

    pthread_mutex_lock(&endpoints.lock);

    endpoints.clients = realloc(
        endpoints.clients, sizeof(struct endpoint) * (endpoints.n_clients + 1));
    endpoints.clients[endpoints.n_clients] = calloc(1, sizeof(struct endpoint));
    p = endpoints.clients[endpoints.n_clients++];

    p->sock = sock;
    strcpy(p->username, username);

    pthread_mutex_unlock(&endpoints.lock);
}

int send_to(int sock, const char *from, int len, const char *content)
{
    xcp_packet_header header;
    xcp_packet_sendmsg msg;
    ssize_t bytes;

    header.version = XCP_VERSION;
    header.type = XCP_SENDMSG;

    msg.message_len = len;
    strncpy(msg.dest, from, 256);

    bytes = write(sock, &header, sizeof(header));
    if (bytes == -1)
        return 1;
    if (write(sock, &msg, sizeof(msg)) < sizeof(msg))
        return 1;
    if (write(sock, content, len) < len)
        return 1;

    return 0;
}

void send_to_all_endpoints(const char *from, int len, const char *content)
{
    struct endpoint *p;

    for (int i = 0; i < endpoints.n_clients; i++) {
        p = endpoints.clients[i];
        if (!p)
            continue;
        if (send_to(p->sock, from, len, content) == 1) {
            close(p->sock);
            endpoints.clients[i] = NULL;

            free(endpoints.clients[i]);
        }
    }
}

void convert_thread(struct client_data *client)
{
    /* Add this client to the endpoint list, and remove this thread+socket from
       the connected user list */

    xcp_packet_convert payload;
    read(client->sock, &payload, sizeof(payload));

    add_client_endpoint(client->sock, payload.username);

    if (client->user_id)
        user_del(client->user_id);

    /* Send confirmation */

    xcp_packet_reply reply;
    reply.status = XS_OK;

    write(client->sock, &reply, sizeof(reply));

    pthread_exit(NULL);
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
        chatinfo("%s joined", packet.username);

        /* TODO: think about the server sending joins */
        char buf[256];
        snprintf(buf, 256, "%s joined", packet.username);
        send_to_all_endpoints("<server>", strlen(buf), buf);

        reply.status = XS_OK;
    } else {
        info("client requested existing name `%s`", packet.username);
        reply.status = XS_NAMETAKEN;
    }

    write(client->sock, &reply, sizeof(reply));
}

void handle_sendmsg(struct client_data *client)
{
    xcp_packet_sendmsg msg;
    char *content;

    read(client->sock, &msg, sizeof(msg));
    content = calloc(msg.message_len + 1, 1);
    read(client->sock, content, msg.message_len);

    chatinfo("%s wrote: %.*s", user_find(client->user_id)->name,
             msg.message_len, content);

    xcp_packet_reply reply;
    reply.status = XS_OK;

    write(client->sock, &reply, sizeof(reply));

    /* TEMP: send message to all */
    send_to_all_endpoints(user_find(client->user_id)->name, msg.message_len,
                          content);
}

void handle_listusers(struct client_data *client)
{
    char buf[256];
    int n = user_count();

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
        case XCP_SENDMSG:
            handle_sendmsg(client);
            break;
        case XCP_LISTUSERS:
            handle_listusers(client);
            break;
        case XCP_CONVERT:
            convert_thread(client);
            break;
        default:
            goto disconnect;
        }
    }

disconnect:
    if (client->user_id) {
        const char *username = user_find(client->user_id)->name;

        chatinfo("%s disconnected", username);
        char buf[256];
        snprintf(buf, 256, "%s disconnected", username);
        send_to_all_endpoints("<server>", strlen(buf), buf);

        user_del(client->user_id);

    } else {
        info("[?] disconnect");
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

    int optval = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

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

    printf("[server] \033[90m");
    vprintf(fmt, args);
    fputs("\033[0m\n", stdout);

    pthread_mutex_unlock(get_console_lock());

    va_end(args);
}

void chatinfo(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    pthread_mutex_lock(get_console_lock());

    printf("[ chat ] ");
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
