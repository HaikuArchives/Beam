%{
/*
 * addr.y -- RFC 822 address parser
 * Ken Murchison
 * $Id$
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

#include <stdlib.h>
#include <string.h>

#include "addr.h"
#include "xmalloc.h"

static int addrerror(char *msg);
extern int addrlex(void);

#define YYERROR_VERBOSE /* i want better error messages! */
%}

%token ATOM QTEXT DTEXT

%start sieve_address

%%
sieve_address: addrspec			/* simple address */
	| phrase '<' addrspec '>'	/* name & addr-spec */
	;

addrspec: localpart '@' domain		/* global-address */
	;

localpart: word				/* uninterpreted, case-preserved */
	| word '.' localpart
	;

domain: subdomain
	| subdomain '.' domain
	;

subdomain: domainref
	| domainlit
	;

domainref: ATOM				/* symbolic reference */
	;

domainlit: '[' DTEXT ']'
	;

phrase: word
	| word phrase
	;

word: ATOM
	| qstring
	;

qstring: '"' QTEXT '"'
	;

%%

/* copy address error message into buffer provided by sieve parser */
static int addrerror(char *s)
{
    extern char *addrerr;

    addrerr = xstrdup(s);
    return 0;
}
