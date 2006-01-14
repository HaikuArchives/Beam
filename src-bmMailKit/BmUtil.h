/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmUtil_h
#define _BmUtil_h

#include "BmMailKit.h"

#include <ctime>

#include <Alert.h>
#include <Message.h>
#include "BmString.h"

extern IMPEXPBMMAILKIT BmString BM_SPACES;
extern IMPEXPBMMAILKIT BmString BM_WHITESPACE;

/*------------------------------------------------------------------------------*\
	FindMsgXXX( archive, name)
		-	functions that extract the msg-field of a specified name from the 
			given archive and return it.
\*------------------------------------------------------------------------------*/
IMPEXPBMMAILKIT const char* FindMsgString( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT bool FindMsgBool( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT int64 FindMsgInt64( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT int32 FindMsgInt32( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT int16 FindMsgInt16( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT float FindMsgFloat( BMessage* archive, const char* name, int32 index=0);
IMPEXPBMMAILKIT BMessage* FindMsgMsg( BMessage* archive, const char* name, BMessage* msg=NULL, 
							 int32 index=0);
IMPEXPBMMAILKIT void* FindMsgPointer( BMessage* archive, const char* name, int32 index=0);

/*------------------------------------------------------------------------------*\
	utility function to format a number of bytes into a string
\*------------------------------------------------------------------------------*/
IMPEXPBMMAILKIT BmString BytesToString( int32 bytes, bool mini=false);

/*------------------------------------------------------------------------------*\
	time-related utility functions
\*------------------------------------------------------------------------------*/
IMPEXPBMMAILKIT BmString TimeToSwatchString( time_t t, const char* format);
IMPEXPBMMAILKIT bool ParseDateTime( const BmString& str, time_t& dateTime);

/*------------------------------------------------------------------------------*\
	utility function to generate the sortkey for a given name:
\*------------------------------------------------------------------------------*/
IMPEXPBMMAILKIT BmString GenerateSortkeyFor( const BmString& name);

/*------------------------------------------------------------------------------*\
	utility functions for UTF8-character parsing:
\*------------------------------------------------------------------------------*/
IMPEXPBMMAILKIT inline bool IS_PART_OF_UTF8_MULTICHAR( unsigned char c) {
	return c & 0x80;
}
IMPEXPBMMAILKIT inline bool IS_UTF8_STARTCHAR( unsigned char c) {
	return (c & 0xc0) == 0xc0;
}
IMPEXPBMMAILKIT inline bool IS_WITHIN_UTF8_MULTICHAR( unsigned char c) {
	return (c & 0xc0) == 0x80;
}

#endif
