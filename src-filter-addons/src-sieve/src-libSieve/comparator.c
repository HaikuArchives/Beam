/* comparator.c -- comparator functions
 * Larry Greenfield
 */
/***********************************************************
        Copyright 1999 by Carnegie Mellon University

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of Carnegie Mellon
University not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE FOR
ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
******************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "comparator.h"
#include "tree.h"
#include "sieve.h"

/* --- i;octet comparators --- */

/* we do a brute force attack */
static int octet_contains(const char *pat, const char *text)
{
    return (strstr(text, pat) != NULL);
}

static int octet_matches_(const char *pat, const char *text, int casemap)
{
    const char *p;
    const char *t;
    char c;

    t = text;
    p = pat;
    for (;;) {
	if (*p == '\0') {
	    /* ran out of pattern */
	    return (*t == '\0');
	}
	c = *p++;
	switch (c) {
	case '?':
	    if (*t == '\0') {
		return 0;
	    }
	    t++;
	    break;
	case '*':
	    while (*p == '*' || *p == '?') {
		if (*p == '?') {
		    /* eat the character now */
		    if (*t == '\0') {
			return 0;
		    }
		    t++;
		}
		/* coalesce into a single wildcard */
		p++;
	    }
	    if (*p == '\0') {
		/* wildcard at end of string, any remaining text is ok */
		return 1;
	    }

	    while (*t != '\0') {
		/* recurse */
		if (octet_matches_(p, t, casemap)) return 1;
		t++;
	    }
	case '\\':
	    p++;
	    /* falls through */
	default:
	    if (casemap && (toupper((int)(unsigned char)c) ==
			    toupper((int)(unsigned char)*t))) {
		t++;
	    } else if (!casemap && (c == *t)) {
		t++;
	    } else {
		/* literal char doesn't match */
		return 0;
	    }
	}
    }
    /* never reaches */
    abort();
}

static int octet_matches(const char *pat, const char *text)
{
    return octet_matches_(pat, text, 0);
}

#ifdef ENABLE_REGEX
static int octet_regex(const char *pat, const char *text)
{
    return (!regexec((regex_t *) pat, text, 0, NULL, 0));
}
#endif

/* these are relational wrappers for octet comparisons */
static int oct_eq(const char *pat, const char *text)
{
    return (strcmp(text, pat) == 0);
}

static int oct_ne(const char *pat, const char *text)
{
    return (strcmp(text, pat) != 0);
}

static int oct_gt(const char *pat, const char *text)
{
    return (strcmp(text, pat) > 0);
}

static int oct_ge(const char *pat, const char *text)
{
    return (strcmp(text, pat) >= 0);
}

static int oct_lt(const char *pat, const char *text)
{
    return (strcmp(text, pat) < 0);
}

static int oct_le(const char *pat, const char *text)
{
    return (strcmp(text, pat) <= 0);
}




/* --- i;ascii-casemap comparators --- */

/* sheer brute force */
static int ascii_casemap_contains(const char *pat, const char *text)
{
    int N = strlen(text);
    int M = strlen(pat);
    int i, j;

    i = 0, j = 0;
    while ((j < M) && (i < N)) {
	if (toupper((int)(unsigned char)text[i]) ==
	    toupper((int)(unsigned char)pat[j])) {
	    i++; j++;
	} else {
	    i = i - j + 1;
	    j = 0;
	}
    }
    return (j == M); /* we found a match! */
}

static int ascii_casemap_matches(const char *pat, const char *text)
{
    return octet_matches_(pat, text, 1);
}

/* these are relational wrappers for ascii-casemap comparisons */
static int acase_eq(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) == 0);
}

static int acase_ne(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) != 0);
}

static int acase_gt(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) > 0);
}

static int acase_ge(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) >= 0);
}

static int acase_lt(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) < 0);
}

static int acase_le(const char *pat, const char *text)
{
    return (strcasecmp(text, pat) <= 0);
}



/* i;ascii-numeric; only supports relational tests
 *
 *  A \ B    number   not-num 
 *  number   A ? B    B > A 
 *  not-num  A > B    A == B
 */
static int ascii_numeric_cmp(const char *text, const char *pat)
{
    if (isdigit((int)(unsigned char)*pat)) {
	if (isdigit((int)(unsigned char)*text)) {
	    return (atoi(text) - atoi(pat));
	} else
	    return -1;
    } else if (isdigit((int)(unsigned char)*text)) 
    	return 1;
    else 
    	return 0; /* both not digits */
}

/* these are relational wrappers for ascii-numeric comparisons */
static int num_eq(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) == 0);
}

static int num_ne(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) != 0);
}

static int num_gt(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) > 0);
}

static int num_ge(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) >= 0);
}

static int num_lt(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) < 0);
}

static int num_le(const char *pat, const char *text)
{
    return (ascii_numeric_cmp(text, pat) <= 0);
}

comparator_t *lookup_comp(const char *comp, int mode, int relation)
{
    comparator_t *ret;

    ret = NULL;
    if (!strcmp(comp, "i;octet")) {
	switch (mode) {
	case IS:
	    ret = &oct_eq;
	    break;
	case CONTAINS:
	    ret = &octet_contains;
	    break;
	case MATCHES:
	    ret = &octet_matches;
	    break;
#ifdef ENABLE_REGEX
	case REGEX:
	    ret = &octet_regex;
	    break;
#endif
	case VALUE:
	    switch (relation)
	    {
	      case EQ:
		ret = &oct_eq;
		break;
	      case NE:
		ret = &oct_ne; 
		break;
	      case GT: 
		ret = &oct_gt; 
		break;
	      case GE:
	         ret = &oct_ge; 
		 break;
	      case LT:
		ret = &oct_lt; 
		break;
	      case LE:
		ret = &oct_le; 
	    }
	    break;
	}
    } else if (!strcmp(comp, "i;ascii-casemap")) {
	switch (mode) {
	case IS:
	    ret = &acase_eq;
	    break;
	case CONTAINS:
	    ret = &ascii_casemap_contains;
	    break;
	case MATCHES:
	    ret = &ascii_casemap_matches;
	    break;
#ifdef ENABLE_REGEX
	case REGEX:
	    /* the ascii-casemap destinction is made during
	       the compilation of the regex in verify_regex() */
	    ret = &octet_regex;
	    break;
#endif
	case VALUE:
	    switch (relation)
	    {
	      case EQ:
		ret = &acase_eq;
		break;
	      case NE:
		ret = &acase_ne; 
		break;
	      case GT: 
		ret = &acase_gt; 
		break;
	      case GE:
	         ret = &acase_ge; 
		 break;
	      case LT:
		ret = &acase_lt; 
		break;
	      case LE:
		ret = &acase_le; 
	    }
	    break;
	}
    } else if (!strcmp(comp, "i;ascii-numeric")) {
	switch (mode) {
	case IS:
	    ret = &num_eq;
	    break;
	case VALUE:
	case COUNT:
	    switch (relation)
	    {
	      case EQ:
		ret = &num_eq;
		break;
	      case NE:
		ret = &num_ne; 
		break;
	      case GT: 
		ret = &num_gt; 
		break;
	      case GE:
	         ret = &num_ge; 
		 break;
	      case LT:
		ret = &num_lt; 
		break;
	      case LE:
		ret = &num_le; 
	    }
	    break;
	}
    }
    return ret;
}
