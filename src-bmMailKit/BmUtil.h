/*
	BmUtil.h
		$Id$
*/

#ifndef _BmUtil_h
#define _BmUtil_h

#include <ctime>

#include <Message.h>
#include <String.h>

extern BString BM_SPACES;
extern BString BM_DEFAULT_STRING;

/*------------------------------------------------------------------------------*\*\
	ShowAlert( text, logtext)
		-	pops up an Alert showing the passed text
		-	logs text unless logtext is specified, in which case that is 
			written to the logfile
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text);

/*------------------------------------------------------------------------------*\*\
	FindMsgXXX( archive, name)
		-	functions that extract the msg-field of a specified name from the given 
			archive and return it.
\*------------------------------------------------------------------------------*/
const char* FindMsgString( BMessage* archive, const char* name, int32 index=0);
bool FindMsgBool( BMessage* archive, const char* name, int32 index=0);
int64 FindMsgInt64( BMessage* archive, const char* name, int32 index=0);
int32 FindMsgInt32( BMessage* archive, const char* name, int32 index=0);
int16 FindMsgInt16( BMessage* archive, const char* name, int32 index=0);
float FindMsgFloat( BMessage* archive, const char* name, int32 index=0);
BMessage* FindMsgMsg( BMessage* archive, const char* name, BMessage* msg=NULL, int32 index=0);
void* FindMsgPointer( BMessage* archive, const char* name, int32 index=0);

/*------------------------------------------------------------------------------*\*\
	utility function to format a number of bytes into a string
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes, bool mini=false);

/*------------------------------------------------------------------------------*\*\
	utility function to format a time into a string
\*------------------------------------------------------------------------------*/
BString TimeToString( time_t t, const char* format="%Y-%m-%d %H:%M:%S");

/*------------------------------------------------------------------------------*\*\
	utility operator to easy concatenation of BStrings
\*------------------------------------------------------------------------------*/
BString operator+(const BString& s1, const BString& s2);
BString operator+(const char* s1, const BString& s2);
BString operator+(const BString& s1, const char* s2);
/*------------------------------------------------------------------------------*\*\
	utility function that removes a set of chars from a BString
	(needed since performance of BString::RemoveAll() is so pathetic...)
\*------------------------------------------------------------------------------*/
BString& RemoveSetFromString( BString& str, const char* chars);
/*------------------------------------------------------------------------------*\*\
	utility functions to convert between different linebreak-styles:
\*------------------------------------------------------------------------------*/
void ConvertLinebreaksToLF( const BString& in, BString& out);
void ConvertLinebreaksToCRLF( const BString& in, BString& out);

#endif
