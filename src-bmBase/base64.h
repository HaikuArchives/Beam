/*
	base64.h
		$Id$
*/

/* [zooey]: this code is straight from MDR.
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/

#ifndef __BASE64_H__
#define __BASE64_H__
#ifdef __cplusplus
extern "C"{
#endif

ssize_t decode64(char *out, const char *in, register off_t length);
ssize_t encode64(char *out, const char *in, register off_t length);

#ifdef __cplusplus
};
#endif
#endif /* __BASE64_H__ */
