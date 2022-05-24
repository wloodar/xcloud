#pragma once

#include <stddef.h>
#include <xcp.h>

void die(const char *fmt, ...);
void userid_to_string(char *buf, xcp_userid userid);
void dbytes(void *addr, size_t amount);

/* Returns the user ID from the data/userid file. If the user ID does not exist,
   returns 0, meaning that you have to send a request to the server for a new
   user ID. */
xcp_userid get_userid();

/* Save the given user ID into the data/userid file. */
int save_userid(xcp_userid userid);


xcp_userid flip_bytes(xcp_userid id);
