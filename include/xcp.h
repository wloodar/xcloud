#pragma once
#include <stdint.h>

#define XCP_VERSION     1
#define XCP_SEVER_PORT  5050
#define XCP_MAXSIZE     1500

#define XCP_PACK        __attribute__((packed))

typedef uint64_t xcp_userid;

enum xcp_packet_type
{
    XCP_ACK,
    XCP_RAW,
    XCP_NEW
};

struct XCP_PACK xcp_packet
{
    uint8_t type;
    uint8_t version;
    uint16_t size;
    xcp_userid user_id;
    uint8_t payload[];
};

struct XCP_PACK xcp_packet_new
{
    int name;
    int img;
    uint8_t data[];
};
