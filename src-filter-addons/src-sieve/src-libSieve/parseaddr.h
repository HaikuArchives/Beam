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

#endif /* INCLUDED_PARSEADDR_H */
