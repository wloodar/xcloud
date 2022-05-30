/* xcloud client
   Copyright (c) 2022 bellrise */

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "../common/common.h"
#include <xcp.h>

#define SERVER_PORT     5050
#define DAEMON_PORT     7708
#define DATA_DIR        "./data/"
#define USERNAME        "test-laptop"


void die(const char *fmt, ...);
void dbytes(void *addr, size_t amount);
int send_example_raw(int sock);
xcp_userid acquire_userid(int sock);


int main(int argc, char **argv)
{
	struct sockaddr_in addr;
	xcp_userid userid;
	int sock;
    bool is_example_req = false;

	if (argc < 2)
		die("Missing argument: <host>");

    for (int i = 0; i < argc; i++) {
        if (!strcmp(argv[i], "-e")) {
            is_example_req = true;
            break;
        }
    }

	/* Setup connection info. */

	sock = socket(AF_INET, SOCK_STREAM, 0);

	addr.sin_addr.s_addr = inet_addr(argv[1]);
	addr.sin_family = AF_INET;

    if (is_example_req) {
        addr.sin_port = htons(DAEMON_PORT);
    } else {
        addr.sin_port = htons(SERVER_PORT);
    }

	if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		die("Failed to connect");

    if (is_example_req) {
        send_example_raw(sock);
        return 0;
    }

	/* If we don't have any saved ID, ask the server for one. */
	userid = get_userid();
	if (!userid.b[0])
		userid = acquire_userid(sock);
	if (!userid.b[0])
		die("Failed to acquire user ID");

	save_userid(userid);
	close(sock);
}

int send_example_raw(int sock)
{
    const char lorem[] = "lorem ipsum message";

    struct xcp_packet packet;
    packet.type = XCP_RAW;
    packet.size = htons(sizeof(lorem));
    packet.version = XCP_VERSION;

    send(sock, &packet, sizeof(packet), 0);
    send(sock, lorem, sizeof(lorem), 0);

    return 0;
}

xcp_userid acquire_userid(int sock)
{
	struct xcp_packet_new *newp;
	struct xcp_packet p;
	uint16_t payload_size;
	xcp_userid id = {0};

	/* Construct the payload */

	const char *username = USERNAME;
	int nusername = strlen(USERNAME);

	payload_size = sizeof(struct xcp_packet_new) + nusername + 1;

	newp = malloc(payload_size);
	newp->name = 0;
	newp->img = -1;
	strncpy((char *) newp->data, username, nusername);
	newp->data[nusername] = 0;

	/* Construct the header */

	p.type = XCP_NEW;
	p.version = XCP_VERSION;
	p.size = payload_size;
	memset(&p.userid, 0, sizeof(p.userid));

	dbytes(&p, sizeof(p));
	dbytes(newp, payload_size);
	write(sock, &p, sizeof(p));
	write(sock, newp, payload_size);

	/* Receive a user ID back. */
	struct xcp_packet res;
	read(sock, &res, sizeof(res));

	if (res.type == XCP_ERR) {
		/* We got an error! */
		uint8_t e;
		if (res.size != 1) {
			info_error("Invalid error packet format");
			return id;
		}

		read(sock, &e, 1);
		info_error("Error while acquiring user ID: %s", xcp_str_err(e));
		return id;
	}

	if (res.type != XCP_NEW)
		die("Invalid packet type received");

	read(sock, &id, sizeof(id));
	free(newp);

	return id;
}
