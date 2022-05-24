#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "common.h"
#include "server.h"
#include "xcp.h"

void serve();
void pass_package(int client_sock, struct xcp_packet buf);
void assign_user_id(int client_sock, uint16_t size);
void save_user_id(xcp_userid user_id, char* name);

int main()
{
    serve();
}

void serve()
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.111.201");
    addr.sin_port = htons(XCP_SEVER_PORT);
    addr.sin_family = AF_INET;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(server_sock, (struct sockaddr*) &addr, sizeof(addr)))
        die("Failed to bind primary socket");

    if (listen(server_sock, 64))
        die("Failed to init listen with sock");

    struct sockaddr_in client_addr;
    socklen_t client_socklen;

    int client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &client_socklen);
    puts("Client connected");

    struct xcp_packet buf;
    read(client_sock, &buf, sizeof(buf));

    buf.size = ntohs(buf.size);
    printf("type: %d, version: %d, size: %d\n", buf.type, buf.version, buf.size);

    pass_package(client_sock, buf);

    close(server_sock);
    close(client_sock);
}

void pass_package(int client_sock, struct xcp_packet buf)
{
    switch (buf.type) {
        case XCP_ACK:
            break;
        case XCP_NEW:
            assign_user_id(client_sock, buf.size);
            break;
    }
}

void assign_user_id(int client_sock, uint16_t size)
{
    struct xcp_packet_new *payload = malloc(size);

    read(client_sock, payload, size);

    char* name = (char*) &payload->data[payload->name];
    printf("User provided name: %s\n", name);

    FILE *f = fopen("/dev/random", "rb");

    xcp_userid user_id;
    fread(&user_id, 1, sizeof(user_id), f);

    send_package(client_sock, XCP_NEW, &user_id, sizeof(user_id));

    save_user_id(user_id, name);

    fclose(f);
    free(payload);
}

void save_user_id(xcp_userid user_id, char* name)
{
    FILE *f = fopen("data/users", "a");

    fprintf(f, "%016llx:%s\n", user_id, name);
    fclose(f);
}
