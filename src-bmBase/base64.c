/*
	base64.c
		$Id$
*/

/* [zooey]: this code is straight from MDR.
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <size_t.h>
#include <sys/types.h>


#define BASE64_LINELENGTH 76
typedef unsigned char uchar;

char base64_alphabet[64] = { //----Fast lookup table
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '+',
  '/'
 };
 
_EXPORT ssize_t	encode64(char *out, const char *in, register off_t length) {
	register unsigned long concat;
	register int i = 0;
	register int k = 0;
	register int curr_linelength = 4; //--4 is a safety extension, designed to cause retirement *before* it actually gets too long
	
	while ( i < length ) {
		concat = ((in[i] & 0xff) << 16);
		
		if ((i+1) < length)
			concat |= ((in[i+1] & 0xff) << 8);
		if ((i+2) < length)
			concat |= (in[i+2] & 0xff);
			
		i += 3;
				
		out[k++] = base64_alphabet[(concat >> 18) & 63];
		out[k++] = base64_alphabet[(concat >> 12) & 63];
		out[k++] = base64_alphabet[(concat >> 6) & 63];
		out[k++] = base64_alphabet[concat & 63];

		if (i >= length) {
			int v;
			for (v = 0; v <= (i - length); v++)
				out[k-v] = '=';
		}

		curr_linelength += 4;
		
		if (curr_linelength > BASE64_LINELENGTH) {
			out[k++] = '\r';
			out[k++] = '\n';
			
			curr_linelength = 4;
		}
	}
	
	return k;
}

_EXPORT  ssize_t	decode64(char *out, const char *in, register off_t length) {
		
		register unsigned long concat, value;
		register int i,j;
		register int k = 0;
		
		for (i = 0; i < length; i += 4) {
			concat = 0;
			
			for (j = 0; (j < 4) && ((i + j) < length); j++) {
				value = in[i+j];
				
				if (( value >= 'A' ) && ( value <= 'Z'))
					value -= 'A';
				else if (( value >= 'a' ) && ( value <= 'z'))
					value = value - 'a' + 26;
				else if (( value >= '0' ) && ( value <= '9'))
					value = value - '0' + 52;
				else if ( value == '+' )
					value = 62;
				else if ( value == '/' )
					value = 63;
				else if ( value == '=' )
					break;
				else {
					i += 2;
					j--;
					continue;
				}
				
				value = value << ((3-j)*6);
				
				concat |= value;
			}
			
			if (j > 1)
				out[k++] = (concat & 0x00ff0000) >> 16;
			if (j > 2)
				out[k++] = (concat & 0x0000ff00) >> 8;
			if (j > 3)
				out[k++] = (concat & 0x000000ff);
		}
		
		return k;
}

