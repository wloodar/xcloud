/* XCloud Protocol
   Definitions for the XCloud Protocol, along with structs and default values.
   Copyright (c) 2023 wloodar, bellrise */

#pragma once

#include <stdint.h>

/* Packet format version. If you receive a value higher than this, it means that
   either you have outdated software or a real bad bug has happened. */
#define XCP_VERSION 1

/* Default port the XCP server listens on for incoming requests. */
#define XCP_SEVER_PORT 7070

typedef enum
{
    XCP_HELLO, /* hello to server */
    XCP_SENDMSG,
    XCP_LISTUSERS,
} xcp_packet_type;

typedef enum
{
    XS_OK,
    XS_ERR,
    XS_NAMETAKEN
} xcp_status;

typedef struct
{
    uint8_t type;    /* type of the packet */
    uint8_t version; /* version of the packet (XCP_VERSION) */
} xcp_packet_header;

typedef struct
{
    xcp_status status;
} xcp_packet_reply;

typedef struct
{
    char username[256];
} xcp_packet_hello;

typedef struct
{
    char dest[256];
    uint16_t message_len;
    /* message here */
} xcp_packet_sendmsg;
