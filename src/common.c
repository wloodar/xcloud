#include "common.h"

#include "xcp.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void send_header(int sock, xcp_packet_type type)
{
    xcp_packet_header p_header;
    p_header.type = type;
    p_header.version = XCP_VERSION;

    write(sock, &p_header, sizeof(p_header));
}

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    fprintf(stderr, "xcloud: \033[91merror\033[0m: \033[1;98m");
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\033[0m\n");

    va_end(args);
    exit(1);
}

int rstrip(char *str)
{
    ssize_t strl, i;

    strl = strlen(str);
    i = strl - 1;
    while (i >= 0) {
        if (isspace(str[i]))
            str[i--] = 0;
        else
            break;
    }

    return 0;
}

void strlist_append(strlist *list, char *string)
{
    list->strings = realloc(list->strings, sizeof(char *) * (list->len + 1));
    list->strings[list->len] = strdup(string);
    list->len++;
}
