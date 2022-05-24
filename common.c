#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "common.h"

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

void userid_to_string(char *buf, xcp_userid userid)
{
	for (int i = 0; i < 8; i++)
		sprintf(buf + i, "%02hhx", ((unsigned char *) &userid)[i]);
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
