/* util.h -- general utility functions
 * 
 * Header file for util.c
 *
 * Copyrights removed. The text contained herein is purely functional,
 * and does not constitute a unique work.
 *
 * All information is extrapolated from the associated .c file.
 *
 */

#ifndef INCLUDED_UTIL_H
#define INCLUDED_UTIL_H

const unsigned char convert_to_lowercase[256];

#define TOLOWER(c) (convert_to_lowercase[(unsigned char)(c)])

/* convert string to all lower case
 */
char *lcase (char *str);

#endif /* INCLUDED_UTIL_H */
