/* xcloud client
   Copyright (c) 2022 bellrise */

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../common/common.h"
#include <xcp.h>

#define SERVER_PORT     5050
#define DATA_DIR        "./data/"
#define HOSTNAME        "test-laptop"


void die(const char *fmt, ...);
void dbytes(void *addr, size_t amount);
xcp_userid acquire_userid(int sock);


int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	xcp_userid userid;
	int sock;

	if (argc < 2)
		die("Missing argument: <host>");

	sock = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		die("Failed to connect");

	/* If we don't have any saved ID, ask the server for one. */
	if (!(userid = get_userid()))
		userid = acquire_userid(sock);

	save_userid(userid);
	close(sock);
}

xcp_userid acquire_userid(int sock)
{
	struct xcp_packet *packet;
	struct xcp_packet_new *newpacket;
	xcp_userid userid;

	const char *username = HOSTNAME;
	int nusername = strlen(username);
	int packet_size = sizeof(struct xcp_packet) + sizeof(struct xcp_packet_new)
			+ nusername + 1;

	packet = malloc(packet_size);
	memset(packet, 0, packet_size);

	newpacket = (struct xcp_packet_new *) packet->payload;
	memcpy(newpacket->data, username, nusername);

	newpacket->data[nusername] = 0;
	newpacket->name = 0;
	newpacket->img = -1;

	packet->size = htons(sizeof(struct xcp_packet_new) + nusername + 1);
	packet->type = XCP_NEW;
	packet->version = XCP_VERSION;

	dbytes(packet, packet_size);
	send(sock, packet, packet_size, 0);

	/* Receive a user ID back. */
	struct xcp_packet res;

	read(sock, &res, sizeof(res));
	if (res.type != XCP_NEW)
		die("Invalid packet type received");

	read(sock, &userid, sizeof(userid));
	userid = flip_bytes(userid);

	free(packet);

	return userid;
}
