/*
 * Copyright (c) 2000, Red Hat, Inc.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation; either version 2 of the License, or
 *     (at your option) any later version.
 *
 *     A copy of the GNU General Public License can be found at
 *     http://www.gnu.org/
 *
 * Written by DJ Delorie <dj@cygnus.com>
 *
 */

#include <stdarg.h>

/* Pass a null-terminated list of strings, and it concatenates them
   into a single string.  Warning - it assumes the result is a file
   name or URL, and will canonicalize the result accordingly
   (i.e. replace \ with /, collapse multiple /// to a single /, etc.)  */

char * concat (const char *s, ...);
char * vconcat (const char *, va_list);

/* convert slashes to backslashes */

char * backslash (char *s);

/* Used when treating / and \ as equivalent. */
#define SLASH_P(ch) \
    ({ \
        char __c = (ch); \
        ((__c) == '/' || (__c) == '\\'); \
    })
