#include <unistd.h>
#include <stdio.h>

#include "../common/common.h"
#include "server.h"
#include "xcp.h"


void send_packet(int client_sock, int type, void *buf, int size)
{
    struct xcp_packet packet;
    packet.type = type;
    packet.size = size;
    packet.version = XCP_VERSION;

	info("Sending %s of size %d to sock=%d", xcp_str_ptype(type), size,
			client_sock);

    write(client_sock, &packet, sizeof(packet));
    write(client_sock, buf, size);
}

void send_packet_err(int client_sock, int err)
{
	uint8_t errb = err;
	send_packet(client_sock, XCP_ERR, &errb, sizeof(errb));
}
