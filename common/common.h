#pragma once

#include <stddef.h>
#include <xcp.h>

void die(const char *fmt, ...);
void userid_to_string(char *buf, xcp_userid userid);
void dbytes(void *addr, size_t amount);

xcp_userid flip_bytes(xcp_userid id);
