#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "../common/common.h"

void listen_requests();
int proccess_socket_data(int server_sock);

int main(int argc, char **argv)
{
    if (argc < 2)
        die("Missing argument <ip>");

    listen_requests(argv[1]);
}

void listen_requests(char *ipaddr)
{
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(ipaddr);
    addr.sin_port = htons(7708);
    addr.sin_family = AF_INET;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (bind(sock, (struct sockaddr*) &addr, sizeof(addr)))
        die("Failed to bind daemon primary socket");

    if (listen(sock, 1))
        die("Failed to init listen with daemon primary sock");

    struct sockaddr_in server_addr;
    socklen_t server_socklen;

    int server_sock = accept(sock, (struct sockaddr*) &server_addr, &server_socklen);
    puts("Daemon connected");

    while (1) {
        if (proccess_socket_data(server_sock))
            break;
    }

    close(server_sock);
    close(sock);
}

int proccess_socket_data(int server_sock)
{
    struct xcp_packet buf;
    if (!read(server_sock, &buf, sizeof(buf)))
        return 1;

    buf.size = ntohs(buf.size);

    printf("%d", buf.size);
}
