/* XCloud Protocol
   Definitions for the XCloud Protocol, along with structs and default values.
   Copyright (c) 2022 wloodar, bellrise */

#ifndef XCP_H
#define XCP_H

#include <stdint.h>

/* Packet format version. If you receive a value higher than this, it means that
   either you have outdated software or a real bad bug has happened. */
#define XCP_VERSION     1

/* Default port the XCP server listens on for incoming requests. */
#define XCP_SEVER_PORT  5050

/* Max payload size. */
#define XCP_MAXSIZE     1500

/* All packets need to be properly packed so the proper amount of bytes is sent
   and received. */
#define __xcp_packed __attribute__((packed))

struct __xcp_packed xcp_userid_fields
{
	uint8_t tm[3];          /* time created */
	uint8_t bytes[4];       /* random bytes */
};

union __xcp_packed xcp_userid
{
	uint8_t b[8];
	struct xcp_userid_fields f;
};

typedef union xcp_userid xcp_userid;


enum xcp_packet_type
{
	XCP_ACK,                /* simple acknowledgement */
	XCP_RAW,                /* raw bytes */
	XCP_NEW,                /* register a new user */
	XCP_ERR,                /* something went wrong */
};

/* Type of the error that is sent along with */
enum xcp_packet_err_type
{
	XCP_EOK,                /* everything is fine */
	XCP_ETAKEN,             /* username is already taken */
	XCP_ESIZE,              /* unsuspected payload size */
	XCP_ETYPE,              /* invalid packet type */
};

struct __xcp_packed xcp_packet
{
	uint8_t     type;       /* type of the packet */
	uint8_t     version;    /* version of the packet (XCP_VERSION) */
	uint16_t    size;       /* size of the payload */
	xcp_userid  userid;     /* user ID for some transactions */
	uint8_t     payload[];  /* the rest is the packet payload */
};

/* XCP_NEW packet payload. The `name` & `img` are indexes into the data array,
   pointing at the selected data. If the index is -1, it means that the data
   does not exist. */
struct __xcp_packed xcp_packet_new
{
    int name;
    int img;
    uint8_t data[];
};


#endif /* XCP_H */
