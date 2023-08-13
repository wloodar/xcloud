#pragma once

void send_header(int sock, xcp_packet_type type);

/* Show information & error messages in the terminal. die() shows an error
   and immediately closes the program. info_error, inform & info are all
   receiding levels of importance. */
void die(const char *fmt, ...);

/* Strip all whitespace from the right side of the string. */
int rstrip(char *str);

typedef struct {
    char **strings;
    int len;
} strlist;