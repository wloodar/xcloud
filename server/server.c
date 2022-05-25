#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "../common/common.h"
#include "server.h"
#include "xcp.h"

void serve();
void pass_package(int client_sock, struct xcp_packet buf);
void assign_user_id(int client_sock, uint16_t size);

void *thread_handle_client(void *client);


struct userlist *glob_userlist;
pthread_mutex_t glob_lock;


int main(int argc, char **argv)
{
	struct userlist userlist;

	pthread_mutex_init(&glob_lock, NULL);
	glob_userlist = &userlist;

	if (argc < 2)
		die("Missing argument: <host>");

	userlist_load(&userlist, "data/users");
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
		puts("Waiting for client connection...");
		client = accept(server_sock, (struct sockaddr*) &client_addr,
				&client_socklen);

		printf("Client connected, delegating to thread. (%s)\n",
				inet_ntoa(client_addr.sin_addr));

		pthread_t thread;
		pthread_create(&thread, NULL, thread_handle_client, &client);
	}

    close(server_sock);
}

void *thread_handle_client(void *client_)
{
	struct xcp_packet buf;
	int client = * (int *) client_;

	/* Read the first packet header from the user. */
	read(client, &buf, sizeof(buf));
	buf.size = ntohs(buf.size);

	printf("Received packet: type=%d, version=%d, size=%d\n", buf.type,
			buf.version, buf.size);

	switch (buf.type) {
		case XCP_NEW:
			assign_user_id(client, buf.size);
	}

	return NULL;
}

void assign_user_id(int client_sock, uint16_t size)
{
    struct xcp_packet_new *payload;
	int r;

	payload = malloc(size);
    read(client_sock, payload, size);

    char* name = (char *) &payload->data[payload->name];
    printf("User provided name: %s\n", name);

	pthread_mutex_lock(&glob_lock);
	r = userlist_find_by_name(glob_userlist, name);
	pthread_mutex_unlock(&glob_lock);

	if (r != -1) {
		/* This means that the username is already taken, so we return a XCP_ERR
		   packet with the XCP_ETAKEN error. */
		struct xcp_packet p;
		p.type = XCP_ERR;
		p.version = XCP_VERSION;
		p.size = 1;

		uint8_t payload = XCP_ETAKEN;

		send(client_sock, &p, sizeof(p), 0);
		send(client_sock, &payload, 1, 0);

		return;
	}

	/* Otherwise we can send a new user ID back. */

    FILE *f = fopen("/dev/random", "rb");
    xcp_userid user_id;

    fread(&user_id, 1, sizeof(user_id), f);

	struct xcp_packet p;
	p.type = XCP_NEW;
	p.size = sizeof(xcp_userid);
	p.version = XCP_VERSION;

	send(client_sock, &p, sizeof(p), 0);
	send(client_sock, &user_id, sizeof(user_id), 0);

	pthread_mutex_lock(&glob_lock);
	userlist_add(glob_userlist, user_id, name);
	pthread_mutex_unlock(&glob_lock);

    fclose(f);
    free(payload);
}
