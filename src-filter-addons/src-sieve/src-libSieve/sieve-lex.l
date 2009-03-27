%{
/* sieve.l -- sieve lexer
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

#include "xmalloc.h"

#include "tree.h"
#include "sieve.h"

int sievelex(void );

static int tonum(char *c);
static char *chkBuf( void);
static char *mlbuf = NULL;
static size_t mlbufsz, mlcur = 0;
int sieveerror(const char *);
void reset_sieve_lexer( FILE* sievein);
%}

%option prefix="sieve"
%option yylineno
%option noyywrap
%option nounput

ws		[ \t]+
ident		[a-zA-Z_][a-zA-Z_0-9]*
CRLF		(\r\n|\r|\n)

%x MULTILINE QSTRING

%%
%{
	BEGIN INITIAL;
%}
<MULTILINE>^\.{CRLF} { 
		    BEGIN INITIAL; 
                    if (mlbuf) 
                        mlbuf[mlcur] = '\0';
                    sievelval.sval = chkBuf(); 
                    return STRING; 
                }
<MULTILINE>^\.\.  { /* dot stuffing! we want one . */ 
		    yyless(1);
		}
<MULTILINE>(.|\n) { 
		    if (mlcur == mlbufsz) 
			mlbuf = xrealloc(mlbuf, 1 + (mlbufsz+=1024));
		    mlbuf[mlcur++] = yytext[0]; 
		}
<MULTILINE><<EOF>> { 
		    sieveerror("unexpected end of file in string"); 
		    yyterminate(); 
		}
<QSTRING>\"     { 
		    BEGIN INITIAL;
                    if (mlbuf) 
                        mlbuf[mlcur] = '\0';
		    sievelval.sval = chkBuf(); 
		    return STRING; 
		}
<QSTRING>(.|\n) { 
		    if (mlcur == mlbufsz) 
			mlbuf = xrealloc(mlbuf, 1 + (mlbufsz+=1024));
		    mlbuf[mlcur++] = yytext[0]; 
		}
text:{ws}?(#.*)?{CRLF}	{ 
		    BEGIN MULTILINE;
		    mlcur = 0; 
		    mlbufsz = 0; 
		    mlbuf = NULL; 
		}
\"        	{ 
		    BEGIN QSTRING;
                    mlcur = 0; 
                    mlbufsz = 0; 
                    mlbuf = NULL; 
                }
[0-9]+[KMG]?	{ 
		    sievelval.nval = tonum(yytext); 
		    return NUMBER; 
		}
if		return IF;
elsif		return ELSIF;
else		return ELSE;
anyof		return ANYOF;
allof		return ALLOF;
exists		return EXISTS;
false		return SFALSE;
true		return STRUE;
address		return ADDRESS;
envelope	return ENVELOPE;
header		return HEADER;
not		return NOT;
size		return SIZE;
reject		return REJCT;
fileinto	return FILEINTO;
redirect	return REDIRECT;
keep		return KEEP;
require		return REQUIRE;
stop		return STOP;
discard		return DISCARD;
setflag		return SETFLAG;
addflag		return ADDFLAG;
removeflag	return REMOVEFLAG;
mark		return MARK;
unmark		return UNMARK;
notify		return NOTIFY;
denotify	return DENOTIFY;
:id		return ID;
:method		return METHOD;
:options	return OPTIONS;
:low		return LOW;
:normal		return NORMAL;
:high		return HIGH;
:message	return MESSAGE;
vacation	return VACATION;
:days		return DAYS;
:addresses	return ADDRESSES;
:subject	return SUBJECT;
:mime		return MIME;
:comparator	return COMPARATOR;
:is		return IS;
:contains	return CONTAINS;
:matches	return MATCHES;
:regex		return REGEX;
:count		return COUNT;
:value		return VALUE;
:over		return OVER;
:under		return UNDER;
:all		return ALL;
:localpart	return LOCALPART;
:domain		return DOMAIN;
:user		return USER;
:detail		return DETAIL;
[ \t\n\r]+ 	; /* ignore whitespace */
#.* 		; /* ignore comments */
.		return yytext[0];

%%
/*  */
static int tonum(char *c)
{
  int val = atoi(c);
  switch (c[strlen(c)-1]) {
  case 'K': val *= (1 << 10); break;
  case 'M': val *= (1 << 20); break;
  case 'G': val *= (1 << 30); break;
  default: break;
  }
  return val;
}

/* convert NULL strings to "" */
static char *chkBuf()
{
    char* ret = mlbuf ? mlbuf : xstrdup("");
    mlbuf = NULL;
    return ret;
}

void reset_sieve_lexer( FILE* sievein) {
    if (mlbuf) {
    	free( mlbuf);
        mlbuf = NULL;
    }
    mlcur = mlbufsz = 0;

    sieverestart( sievein);

    yy_init = 1;		/* cause flex-code to reinitialize */
    yy_start = 0;		/* reset start state number */
}
