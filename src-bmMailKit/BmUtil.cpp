/*
	BmUtil.cpp
		$Id$
*/

#include <stdio.h>

#include <Alert.h>

#include "BmBasics.h"
#include "BmUtil.h"

BString BM_SPACES("                                                                                ");
BString BM_DEFAULT_STRING;

/*------------------------------------------------------------------------------*\
	FindMsgString( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
const char* FindMsgString( BMessage* archive, const char* name, int32 index) {
	const char* str;
	BM_assert(archive && name);
	if (archive->FindString( name, index, &str) == B_OK) {
		return str;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgBool( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
bool FindMsgBool( BMessage* archive, const char* name, int32 index) {
	bool b;
	BM_assert(archive && name);
	if (archive->FindBool( name, index, &b) == B_OK) {
		return b;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgInt64( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int64 FindMsgInt64( BMessage* archive, const char* name, int32 index) {
	int64 i;
	BM_assert(archive && name);
	if (archive->FindInt64( name, index, &i) == B_OK) {
		return i;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgInt32( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int32 FindMsgInt32( BMessage* archive, const char* name, int32 index) {
	int32 i;
	BM_assert(archive && name);
	if (archive->FindInt32( name, index, &i) == B_OK) {
		return i;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgInt16( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int16 FindMsgInt16( BMessage* archive, const char* name, int32 index) {
	int16 i;
	BM_assert(archive && name);
	if (archive->FindInt16( name, index, &i) == B_OK) {
		return i;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgMsg( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
BMessage* FindMsgMsg( BMessage* archive, const char* name, BMessage* msg, int32 index) {
	BM_assert(archive && name);
	if (!msg)
		msg = new BMessage;
	if (archive->FindMessage( name, index, msg) == B_OK) {
		return msg;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgFloat( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
float FindMsgFloat( BMessage* archive, const char* name, int32 index) {
	float f;
	BM_assert(archive && name);
	if (archive->FindFloat( name, index, &f) == B_OK) {
		return f;
	} else {
		BString s( "unknown message-field: ");
		s += name;
		throw BM_invalid_argument( s.String());
	}
}

/*------------------------------------------------------------------------------*\
	FindMsgPointer( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
void* FindMsgPointer( BMessage* archive, const char* name, int32 index) {
	void* ptr;
	BM_assert(archive && name);
	if (archive->FindPointer( name, index, &ptr) == B_OK) {
		return ptr;
	} else {
		throw BM_invalid_argument( BString( "unknown message-field: ") << name);
	}
}

/*------------------------------------------------------------------------------*\
	BytesToString( bytes)
		-	returns the given number of bytes as a short, descriptive string:
			* bytes < 1024 				-> "X bytes"
			* 1024 < bytes < 1024000	-> "X.xx KB"
			* 1048576 < bytes 			-> "X.xx MB"
		-	we define a Mega-Byte as 1024**2 bytes (as Tracker does)
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes, bool mini) {
	char buf[20];
	if (bytes >= 1048576) {
		sprintf( buf, "%6.2f MB", bytes/1048576.0);
	} else if (bytes >= 1024) {
		sprintf( buf, "%6.2f KB", bytes/1024.0);
	} else {
		sprintf( buf, "%ld %s", bytes, mini ? "b" : "bytes");
	}
	return BString(buf);
}

/*------------------------------------------------------------------------------*\
	TimeToString( time)
		-	converts the given time into a string
\*------------------------------------------------------------------------------*/
BString TimeToString( time_t t, const char* format) {
	BString s;
	const int32 bufsize=40;
	s.SetTo( '\0', bufsize);
	char* buf=s.LockBuffer( 0);
	strftime( buf, bufsize, format, localtime( &t));
	s.UnlockBuffer( -1);
	return s;
}


/*------------------------------------------------------------------------------*\*\
	ShowAlert( text)
		-	pops up an Alert showing the passed (error-)text
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}

/*------------------------------------------------------------------------------*\
	BString::operator+
		-	
\*------------------------------------------------------------------------------*/
BString operator+(const BString& s1, const BString& s2) 
{
	BString result(s1);
	result += s2;
	return result;
}

BString operator+(const char* s1, const BString& s2) 
{
	BString result(s1);
	result += s2;
	return result;
}

BString operator+(const BString& s1, const char* s2) 
{
	BString result(s1);
	result += s2;
	return result;
}

/*------------------------------------------------------------------------------*\
	RemoveSetFromString( string, charsToRemove)
		-	
\*------------------------------------------------------------------------------*/
BString& RemoveSetFromString( BString& str, const char* charsToRemove) {
	if (!charsToRemove) return str;
	char* buf = str.LockBuffer( 0);
	if (buf) {
		char* pos = buf;
		char* newPos = buf;
		while( *pos) {
			if (strchr( charsToRemove, *pos))
				pos++;
			else {
				if (pos != newPos)
					*newPos++ = *pos++;
				else {
					newPos++;
					pos++;
				}
			}
		}
		*newPos = 0;
		str.UnlockBuffer( newPos-buf);
	}
	return str;
}
