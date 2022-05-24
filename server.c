#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "server.h"
#include "xcp.h"

void serve();

int main()
{
    serve();
}

void serve()
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr("192.168.111.201");
    addr.sin_port = htons(5050);
    addr.sin_family = AF_INET;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(server_sock, (struct sockaddr*) &addr, sizeof(addr))) {
        die("Failed to bind primary socket");
    }

    if (listen(server_sock, 64)) {
        die("Failed to init listen with sock");
    }

    struct sockaddr_in client_addr;
    socklen_t client_socklen;

    int client_sock = accept(server_sock, (struct sockaddr*) &client_addr, &client_socklen);
    puts("Client connected");

    struct xcp_packet buf;
    read(client_sock, &buf, sizeof(buf));

    printf("%d\n", buf.type);
}
