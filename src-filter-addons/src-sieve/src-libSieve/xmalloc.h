/* xmalloc.h -- Allocation package that calls fatal() when out of memory
 * 
 * Header file for xmalloc.c
 *
 * Copyrights removed. The text contained herein is purely functional,
 * and does not constitute a unique work.
 *
 * All information is extrapolated from the associated .c file.
 *
 */

#ifndef INCLUDED_XMALLOC_H
#define INCLUDED_XMALLOC_H

/* for size_t */
#include <stdio.h>

void *xmalloc (size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstrdup (const char *str);
char *xstrconcat (const char *str, ...);
void *fatal(const char *fatal_message, int fatal_code);

#endif /* INCLUDED_XMALLOC_H */
