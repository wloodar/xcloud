#pragma once

#include <stddef.h>
#include <xcp.h>

/* Show information & error messages in the terminal. die() shows an error
   and immediately closes the program. info_error, inform & info are all
   receiding levels of importance. */
void die(const char *fmt, ...);
void info_error(const char *fmt, ...);
void inform(const char *fmt, ...);
void info(const char *fmt, ...);

/* Convert the 16 (or 18 if str starts with 0x) characters into a user ID. */
xcp_userid string_to_userid(const char *str);

/* The minimum size in `buf` must be 17 bytes. 16 for the user ID, and 1 for the
   null byte at the end. This function _does not_ add the null byte, and ope-
   -rates only on the first 16 chars. */
void userid_to_string(char *buf, xcp_userid userid);

void dbytes(void *addr, size_t amount);

/* Convert XCP integers to human-readable names. */
const char *xcp_str_ptype(int ptype);
const char *xcp_str_err(int err);

/* Returns the user ID from the data/userid file. If the user ID does not exist,
   returns 0, meaning that you have to send a request to the server for a new
   user ID. */
xcp_userid get_userid();

/* Save the given user ID into the data/userid file. */
int save_userid(xcp_userid userid);

/* Strip all whitespace from the right side of the string. */
int rstrip(char *str);
