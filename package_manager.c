#include <stdio.h>
#include <sys/socket.h>
#include "server.h"
#include "xcp.h"

void send_package(int client_sock, int type, void *buf, int size)
{
    struct xcp_packet packet;
    packet.type = type;
    packet.size = size;
    packet.version = XCP_VERSION;

    send(client_sock, &packet, sizeof(packet), 0);
    send(client_sock, buf, size, 0);
}
