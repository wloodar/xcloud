#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

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