#pragma once
#include <stdint.h>

enum xcp_packet_type
{
    XCP_ACK,
    XCP_RAW,
    XCP_NEW
};

struct xcp_packet
{
    int type;
    uint8_t *payload;
};
