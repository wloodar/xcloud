/* xcloud client
   Copyright (c) 2022 bellrise */

#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "common.h"
#include "xcp.h"

#define SERVER_PORT     5050
#define DATA_DIR        "./data/"
#define HOSTNAME        "test-laptop"


struct __attribute__((packed)) userdata
{
	uint8_t userid[8];
	int userid_ts;
};


void die(const char *fmt, ...);
void dbytes(void *addr, size_t amount);
void acquire_userid(int sock, struct userdata *userdata);
int load_userdata(struct userdata *data);
int save_userdata(struct userdata *data);



int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	struct userdata userdata;
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
	if (load_userdata(&userdata))
		acquire_userid(sock, &userdata);

	save_userdata(&userdata);

	close(sock);
}

int load_userdata(struct userdata *data)
{
	FILE *f;

	if (!(f = fopen(DATA_DIR "userid", "rb")))
		return 1;

	fread(data, sizeof(*data), 1, f);
	fclose(f);
	return 0;
}

int save_userdata(struct userdata *data)
{
	FILE *f;

	f = fopen(DATA_DIR "userid", "wb");
	fwrite(data, sizeof(*data), 1, f);

	fclose(f);
	return 0;
}

void acquire_userid(int sock, struct userdata *data)
{
	struct xcp_packet *packet;
	struct xcp_packet_new *newpacket;

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

	send(sock, packet, packet_size, 0);

	/* Receive a user ID back. */
	struct xcp_packet res;

	read(sock, &res, sizeof(res));
	if (res.type != XCP_NEW)
		die("Invalid packet type received");

	read(sock, data->userid, 8);
	dbytes(data, sizeof(*data));

	data->userid_ts = time(NULL);

	printf("User ID: ");
	for (int i = 0; i < 8; i++)
		printf("%02hhx", data->userid[i]);
	fputc('\n', stdout);

#if 1
	FILE *f = fopen("test-bin", "wb");
	fwrite(packet, 1, sizeof(struct xcp_packet) + sizeof(struct xcp_packet_new)
			+ nusername + 1, f);
	fclose(f);
#endif

	free(packet);
}
