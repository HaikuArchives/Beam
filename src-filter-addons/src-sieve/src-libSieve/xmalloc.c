/* xmalloc.c -- Allocation package that calls fatal() when out of memory
 *
 * Copyright (c) 1998-2000 Carnegie Mellon University.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The name "Carnegie Mellon University" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For permission or any other legal
 *    details, please contact  
 *      Office of Technology Transfer
 *      Carnegie Mellon University
 *      5000 Forbes Avenue
 *      Pittsburgh, PA  15213-3890
 *      (412) 268-4387, fax: (412) 268-7395
 *      tech-transfer@andrew.cmu.edu
 *
 * 4. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by Computing Services
 *     at Carnegie Mellon University (http://www.cmu.edu/computing/)."
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
/*
 * $Id$
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "xmalloc.h"

void* xmalloc(size_t size)
{
    void *ret;

    ret = malloc(size);
    if (ret != NULL) return ret;

    fatal("Virtual memory exhausted", 0);
    return 0; /*NOTREACHED*/
}

void *xrealloc (void* ptr, size_t size)
{
    void *ret;

    /* xrealloc (NULL, size) behaves like xmalloc (size), as in ANSI C */
    ret = (!ptr ? malloc (size) : realloc (ptr, size));
    if (ret != NULL) return ret;

    fatal("Virtual memory exhausted", 0);
    return 0; /*NOTREACHED*/
}

char *xstrdup(const char* str)
{
    char *p = xmalloc(strlen(str)+1);
    strcpy(p, str);
    return p;
}

/* Thanks to Timo Sirainen, December 2002 */
char *xstrconcat (const char *str, ...)
{
    va_list va;
    char *buf;
    size_t len, pos, size;

    if (str == NULL)
	return NULL;

    va_start(va, str);

    buf = NULL; pos = size = 0;
    do {
	len = strlen(str);
	if (size+pos <= len) {
	    size += len + 256;
	    buf = xrealloc(buf, size);
	}
	memcpy(buf+pos, str, len);
	pos += len;

	str = va_arg(va, const char *);
    } while (str != NULL);
    buf[pos] = '\0';

    va_end(va);
    return buf;
}

void *fatal(const char *message, int rc)
{
    fprintf(stderr, "fatal error: %s\n", message);
    exit(rc);
}
