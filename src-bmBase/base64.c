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
		
	unsigned int concat = 0;
	unsigned int mIndex = 0;
	int value;
	
	static int base64_alphabet[256] = { //----Fast lookup table
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
		 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1,  0, -1, -1,
		 -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
		 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
		 -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
		 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
		 -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
	};

	const char* src = in;
	const char* srcEnd = in+length;
	char* dest = out;
	char* destEnd = out+length*5/3;
		
	while( src+(3-mIndex)<srcEnd && dest<=destEnd-3) {
		if ((value = base64_alphabet[(unsigned char)*src++])==-1)
			continue;
			
		concat |= (((unsigned int)value) << ((3-mIndex)*6));
		
		if (++mIndex == 4) {
			*dest++ = (concat & 0x00ff0000) >> 16;
			*dest++ = (concat & 0x0000ff00) >> 8;
			*dest++ = (concat & 0x000000ff);
			concat = mIndex = 0;
		}
	}
	
	return dest-out;
}

