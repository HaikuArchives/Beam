/*
	BmUtil.cpp
		$Id$
*/

#include <stdio.h>

#include <Alert.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmUtil.h"

BString BM_SPACES("                                                                                                                                                                                    ");
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
	char* buf=s.LockBuffer( 40);
	strftime( buf, bufsize, format, localtime( &t));
	s.UnlockBuffer( strlen(buf));
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

/*------------------------------------------------------------------------------*\*\
	ShowAlertWithType( text, type)
		-	pops up an Alert of given type, showing the passed text
\*------------------------------------------------------------------------------*/
void ShowAlertWithType( const BString &text, alert_type type) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, type);
	alert->Go();
}

/*------------------------------------------------------------------------------*\
	BString::operator+
		-	utility operators for easier concatenation
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
		-	removes the given chars form the given string
		-	this function performs *much* better than BString::RemoveSet()
\*------------------------------------------------------------------------------*/
BString& RemoveSetFromString( BString& str, const char* charsToRemove) {
	if (!charsToRemove) return str;
	char* buf = str.LockBuffer( str.Length());
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

/*------------------------------------------------------------------------------*\
	BmToLower( string)
		-	calls ToLower() on given string in a DANO-compatible way:
\*------------------------------------------------------------------------------*/
BString& BmToLower( BString& str) {
	if (str.Length())
		// check introduced for Dano compatibility, otherwise "mysterious things"(TM) happen:
		str.ToLower();
	return str;
}

/*------------------------------------------------------------------------------*\
	BmToUpper( string)
		-	calls ToUpper() on given string in a DANO-compatible way:
\*------------------------------------------------------------------------------*/
BString& BmToUpper( BString& str) {
	if (str.Length())
		// check introduced for Dano compatibility, otherwise "mysterious things"(TM) happen:
		str.ToUpper();
	return str;
}

/*------------------------------------------------------------------------------*\
	ConvertLinebreaksToLF( in, out)
		-	converts linebreaks of given in-string from CRLF to LF
		-	result is stored in out
\*------------------------------------------------------------------------------*/
void ConvertLinebreaksToLF( const BString& in, BString& out) {
	int32 outSize = in.Length();
	if (!outSize) {
		out = "";
		return;
	}
	char* buf = out.LockBuffer( outSize);
	const char* pos = in.String();
	char* newPos = buf;
	while( *pos) {
		if (*pos == '\r' && *(pos+1) == '\n')
			pos++;
		else
			*newPos++ = *pos++;
	}
	*newPos = 0;
	out.UnlockBuffer( newPos-buf);
}

/*------------------------------------------------------------------------------*\
	ConvertLinebreaksToCRLF( string)
		-	converts linebreaks of given in-string from LF to CRLF
		-	result is stored in out
\*------------------------------------------------------------------------------*/
void ConvertLinebreaksToCRLF( const BString& in, BString& out) {
	int32 outSize = in.Length()*2;
	if (!outSize) {
		out = "";
		return;
	}
	char* buf = out.LockBuffer( outSize);
	const char* pos = in.String();
	char* newPos = buf;
	while( *pos) {
		if (*pos=='\n' && (pos==in.String() || *(pos-1)!='\r'))
			*newPos++ = '\r';
		*newPos++ = *pos++;
	}
	*newPos = 0;
	out.UnlockBuffer( newPos-buf);
	out.Truncate( newPos-buf);
}

/*------------------------------------------------------------------------------*\
	ConvertTabsToSpaces( in, out)
		-	converts all tabs of given in-string into three spaces
		-	result is stored in out
\*------------------------------------------------------------------------------*/
void ConvertTabsToSpaces( const BString& in, BString& out) {
	int32 outSize = in.Length()*2;
	if (!outSize) {
		out = "";
		return;
	}
	const int numSpaces = 3;
	char* buf = out.LockBuffer( outSize);
	const char* pos = in.String();
	char* newPos = buf;
	while( *pos) {
		if (*pos=='\t') {
			for( int i=0; i<numSpaces; ++i)
				*newPos++ = ' ';
			pos++;
		} else
			*newPos++ = *pos++;
	}
	*newPos = 0;
	out.UnlockBuffer( newPos-buf);
	out.Truncate( newPos-buf);
}

/*------------------------------------------------------------------------------*\
	WordWrap( in, out, maxLineLen)
		-	wraps given in-string along word-boundary
		-	param maxLineLen indicates right border for wrap
		-	resulting text is stored in param out
		-	the string in has to be UTF8-encoded for this function to work correctly!
\*------------------------------------------------------------------------------*/
void WordWrap( const BString& in, BString& out, int32 maxLineLen, BString nl) {
	if (!in.Length()) {
		out = "";
		return;
	}
	int32 lastPos = 0;
	const char *s = in.String();
	for(	int32 pos = 0; 
			(pos = in.FindFirst( nl, pos)) != B_ERROR; 
			pos += nl.Length(), lastPos = pos) {
		// determine length of line in UTF8-characters (not bytes):
		int32 lineLen = 0;
		for( int i=lastPos; i<pos; ++i) {
			while( i<pos && s[i]&0x80)
				i++;
			lineLen++;
		}
		while (lineLen > maxLineLen) {
			int32 lastSpcPos = in.FindLast( " ", lastPos+maxLineLen);
			if (lastSpcPos==B_ERROR || lastSpcPos<lastPos) {
				// line doesn't contain any space character, we simply break it
				// at right margin:
				out.Append( in.String()+lastPos, maxLineLen);
				out.Append( nl);
				lastPos += maxLineLen;
			} else {
				// wrap line after last space:
				out.Append( in.String()+lastPos, 1+lastSpcPos-lastPos);
				out.Append( nl);
				lastPos = lastSpcPos+1;
			}
			lineLen = 0;
			for( int i=lastPos; i<pos; ++i) {
				while( i<pos && s[i]&0x80)
					i++;
				lineLen++;
			}
		}
		out.Append( in.String()+lastPos, nl.Length()+pos-lastPos);
	}
	if (lastPos < in.Length())
		out.Append( in.String()+lastPos, in.Length()-lastPos);
}
