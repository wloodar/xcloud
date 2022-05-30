#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "common.h"

#define USERID_FILE     "data/userid"


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

void info_error(const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   fprintf(stderr, "xcloud: \033[91merror\033[0m: \033[1;98m");
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "\033[0m\n");

   va_end(args);
}

void info(const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   fprintf(stderr, "xcloud: \033[90m");
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "\033[0m\n");

   va_end(args);
}

void inform(const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);

   fprintf(stderr, "xcloud: \033[1;98m");
   vfprintf(stderr, fmt, args);
   fprintf(stderr, "\033[0m\n");

   va_end(args);
}

void userid_to_string(char *buf, xcp_userid userid)
{
	for (int i = 0; i < 8; i++)
		sprintf(buf + i * 2, "%02hhx", ((unsigned char *) &userid)[i]);
}

const char *xcp_str_err(int err)
{
	const char *const errs[] = {
		"Everything is fine",
		"Username is already taken",
		"Unsuspected payload size",
		"???"
	};
	const int nerrs = sizeof(errs) / sizeof(char *) - 1;

	err = err < 0 ? -err : err;
	if (err <= nerrs)
		return errs[err];
	return errs[nerrs];
}

const char *xcp_str_ptype(int ptype)
{
	const char *const ptypes[] = {
		"XCP_ACK",
		"XCP_RAW",
		"XCP_NEW",
		"XCP_ERR",
		"???"
	};
	const int nptypes = sizeof(ptypes) / sizeof(char *) - 1;

	ptype = ptype < 0 ? -ptype : ptype;
	if (ptype <= nptypes)
		return ptypes[ptype];
	return ptypes[ptype];
}

xcp_userid string_to_userid(const char *str)
{
	xcp_userid id = {0};
	int c;

	if (!strncmp(str, "0x", 2))
		str += 2;

	/* Walk from the start and turn the hex chars into bytes. */
	for (int i = 0; i < 16; i++) {
		c = str[i];

		if (c >= '0' && c <= '9')
			c -= '0';
		if (c >= 'A' && c <= 'F')
			c -= 'A' - 10;
		if (c >= 'a' && c <= 'f')
			c -= 'a' - 10;

		c <<= (i % 2 == 0 ? 4 : 0);
		id.b[i >> 1] |= c;
	}

	return id;
}

void dbytes(void *addr, size_t amount)
{
   size_t lines, rest;

   lines = amount >> 4;
   for (size_t i = 0; i < lines; i++) {
      for (size_t j = 0; j < 16; j++) {
         printf("%02hhx", ((char *) addr)[i*16+j]);
         if (j % 2 != 0)
            putc(' ', stdout);
      }
      printf("\n");
   }

   rest = (lines << 4) ^ amount;
   for (size_t i = 0; i < rest; i++) {
      printf("%02hhx", ((char *) addr)[(lines << 4) + i]);
      if (i % 2 != 0)
         putc(' ', stdout);
   }

   if (rest)
      putc('\n', stdout);
}

xcp_userid get_userid()
{
	xcp_userid userid = {0};
	FILE *f;

	if (!(f = fopen(USERID_FILE, "rb")))
		return userid;

	fread(&userid, 1, sizeof(userid), f);
	fclose(f);
	return userid;
}

int save_userid(xcp_userid userid)
{
	FILE *f;

	char tmpbuf[17] = {0};
	userid_to_string(tmpbuf, userid);
	inform("Saving userid %s", tmpbuf);

	if (!(f = fopen(USERID_FILE, "wb")))
		return 1;

	fwrite(&userid, 1, sizeof(userid), f);
	fclose(f);
	return 0;
}

int rstrip(char *str)
{
   size_t strl, i;

   strl = strlen(str);
   i = strl - 1;
   while (i > 0) {
      if (isspace(str[i]))
         str[i--] = 0;
      else
         break;
   }

   return 0;
}
