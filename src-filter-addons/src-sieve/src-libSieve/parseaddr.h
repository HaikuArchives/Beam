/* parseaddr.h -- RFC 822 address parser
 * 
 * Header file for parseaddr.c
 *
 * Copyrights removed. The text contained herein is purely functional,
 * and does not constitute a unique work.
 *
 * All information is extrapolated from the associated .c file.
 *
 */

#ifndef INCLUDED_PARSEADDR_H
#define INCLUDED_PARSEADDR_H

struct address {
    char *name;
    char *route;
    char *mailbox;
    char *domain;
    struct address *next;
    char *freeme;		/* If non-nil, free */
};

void parseaddr_list (const char *s, struct address **addrp);
void parseaddr_free (struct address *addr);

static char parseaddr_unspecified_domain[] = "unspecified-domain";
static void parseaddr_append (struct address ***addrpp, char *name,
				char *route, char *mailbox, char *domain,
				char **freemep);
static int parseaddr_phrase (char **inp, char **phrasep, char *specials);
static int parseaddr_domain (char **inp, char **domainp, char **commmentp);
static int parseaddr_route (char **inp, char **routep);


#endif /* INCLUDED_PARSEADDR_H */
