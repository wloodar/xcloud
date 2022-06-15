#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "../common/common.h"
#include "server.h"
#include "xcp.h"

#define USERS_FILE  "data/users"


void serve();
void assign_user_id(int client_sock, uint16_t size);
void recv_raw(int client_sock, uint16_t size);
void read_to_null(int client_sock, uint16_t size);

struct thread_data
{
	int sock;
};

void *thread_handle_client(void *client);

struct userlist *glob_userlist;
pthread_mutex_t glob_userlist_lock;


int main(int argc, char **argv)
{
	struct userlist userlist;

	pthread_mutex_init(&glob_userlist_lock, NULL);
	glob_userlist = &userlist;

	if (argc < 2)
		die("Missing argument: <host>");

	userlist_load(&userlist, USERS_FILE);
	serve(argv[1]);

	userlist_free(&userlist);
}

void serve(char *host)
{
	struct sockaddr_in addr;
	addr.sin_addr.s_addr = inet_addr(host);
	addr.sin_port = htons(XCP_SEVER_PORT);
	addr.sin_family = AF_INET;

	if (addr.sin_addr.s_addr == (in_addr_t) -1)
		die("Failed to parse address: %s", host);

	int server_sock = socket(AF_INET, SOCK_STREAM, 0);

	if (bind(server_sock, (struct sockaddr*) &addr, sizeof(addr)))
		die("Failed to bind primary socket");

	if (listen(server_sock, 64))
		die("Failed to init listen with sock");

	/* Start accepting connections. Because we want to handle connections
	   quickly, we move them to another thread. */
	struct sockaddr_in client_addr;
	socklen_t client_socklen;
	int client;

	while (1) {
		info("Waiting for client connection...");

		client_socklen = 0;
		client = accept(server_sock, (struct sockaddr*) &client_addr,
				&client_socklen);

		if (client == -1) {
			info_error("Failed to accept connection: errno=%d", errno);
			continue;
		}

		inform("Client (\033[92m%d\033[0m) connected", client);

		/* Make a new struct for the thread. */
		struct thread_data *data = malloc(sizeof(struct thread_data));
		data->sock = client;

		pthread_t thread;
		pthread_create(&thread, NULL, thread_handle_client, data);
	}

	close(server_sock);
}

void *thread_handle_client(void *client_)
{
	struct thread_data *client = client_;
	struct xcp_packet buf;

	while (1) {
		/* Read the first packet header from the user. */

		if (!read(client->sock, &buf, sizeof(buf))) {
			inform("Client (\033[91m%d\033[0m) disconnected", client->sock);
			goto end;
		}

		buf.size = ntohs(buf.size);

		info("Received packet: type=%s, version=%d, size=%d",
				xcp_str_ptype(buf.type), buf.version, buf.size);

		switch (buf.type) {
			case XCP_NEW:
				assign_user_id(client->sock, buf.size);
				break;
			case XCP_RAW:
				recv_raw(client->sock, buf.size);
				break;
			default:
				/* If the client sends an incorrect packet type, we just want to
				   send an error packet and close the connection. */
				send_packet_err(client->sock, XCP_ETYPE);
				close(client->sock);
				goto end;
		}
	}

end:
	free(client);
	return NULL;
}

void assign_user_id(int client_sock, uint16_t size)
{
	struct xcp_packet_new *payload;
	int r;

	if (!size) {
		send_packet_err(client_sock, XCP_ESIZE);
		return;
	}

	payload = malloc(size);
	read(client_sock, payload, size);

	char* name = (char *) &payload->data[payload->name];
	info("User provided name: %s", name);

	pthread_mutex_lock(&glob_userlist_lock);
	r = userlist_find_by_name(glob_userlist, name);
	pthread_mutex_unlock(&glob_userlist_lock);

	if (r != -1) {
		/* This means that the username is already taken, so we return a XCP_ERR
		   packet with the XCP_ETAKEN error. */
		send_packet_err(client_sock, XCP_ETAKEN);
		return;
	}

	/* Otherwise we can send a new user ID back. */
	xcp_userid userid;

	userid = generate_userid();

	send_packet(client_sock, XCP_NEW, &userid, sizeof(userid));

	pthread_mutex_lock(&glob_userlist_lock);
	userlist_add(glob_userlist, userid, name);
	userlist_save(glob_userlist, USERS_FILE);
	pthread_mutex_unlock(&glob_userlist_lock);

	free(payload);
}

void recv_raw(int client_sock, uint16_t size)
{
	uint8_t *buf;

	buf = malloc(size);

	read(client_sock, buf, size);
	dbytes(buf, size);

	for (int i = 0; i < size; i++) {
		if (buf[i] > 0x20 && buf[i] < 0x7f)
			fputc(buf[i], stdout);
		else
			fputc('.', stdout);
	}

	fputc('\n', stdout);

	free(buf);
}

xcp_userid generate_userid()
{
	FILE *f = fopen("/dev/urandom", "rb");
	xcp_userid id = {0};

	/* As per RSD 11/4, the user ID is a 8 byte struct with the first 4
	   bytes being the time of creation, and 4 random bytes. */
	if (!f)
		return id;

	* (int *) id.f.tm = time(NULL);
	fread(id.f.bytes, 1, 4, f);

	fclose(f);
	return id;
}
