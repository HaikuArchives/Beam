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
	const int32 bufsize=100;
	s.SetTo( '\0', bufsize);
	char* buf=s.LockBuffer( bufsize+1);
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
	char* buf = str.LockBuffer( str.Length()+1);
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
	ReplaceSubstringWith( str, findStr, replaceStr)
		-	replaces all occurrences of findStr in str with replaceStr
		-	this function performs *much* better than BString::ReplaceAll()
\*------------------------------------------------------------------------------*/
BString& ReplaceSubstringWith( BString& str, const BString findStr, 
										 const BString replaceStr) {
	if (!findStr.Length() || !str.Length())
		return str;
	int32 newbufLen = (int32)MAX( MAX( replaceStr.Length(), 128), str.Length()*1.5);
	char* newbuf = (char*)malloc( newbufLen+1);
	if (newbuf) {
		int32 destPos = 0;
		int32 lastSrcPos = 0;
		int32 len;
		for( int32 srcPos=0; (srcPos=str.FindFirst( findStr, lastSrcPos))!=B_ERROR; ) {
			len = srcPos-lastSrcPos;
			if (destPos+len+replaceStr.Length() >= newbufLen) {
				newbufLen = MAX( 2*newbufLen, destPos+len+replaceStr.Length());
				newbuf = (char*)realloc( newbuf, newbufLen+1);
			}
			if (len>0) {
				str.CopyInto( newbuf+destPos, lastSrcPos, len);
				destPos+=len;
			}
			replaceStr.CopyInto( newbuf+destPos, 0, replaceStr.Length());
			destPos += replaceStr.Length();
			lastSrcPos = srcPos+findStr.Length();
		}
		if ((len = str.Length()-lastSrcPos)) {
			if (destPos+len >= newbufLen) {
				newbufLen = MAX( 2*newbufLen, destPos+len);
				newbuf = (char*)realloc( newbuf, newbufLen+1);
			}
			str.CopyInto( newbuf+destPos, lastSrcPos, len);
			destPos += len;
		}
		*(newbuf+destPos) = '\0';
		str.SetTo( newbuf);
		free( newbuf);
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
	char* buf = out.LockBuffer( outSize+1);
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
	char* buf = out.LockBuffer( outSize+1);
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
	const int numSpaces = 3;
	int32 outSize = in.Length()*numSpaces;
	if (!outSize) {
		out = "";
		return;
	}
	char* buf = out.LockBuffer( outSize+1);
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
	DeUrlify( in, out)
		-	converts all %xx sequences into the corresponding chars
		-	result is stored in out
\*------------------------------------------------------------------------------*/
#define HEXDIGIT2CHAR(d) (((d)>='0'&&(d)<='9') ? (d)-'0' : ((d)>='A'&&(d)<='F') ? (d)-'A'+10 : ((d)>='a'&&(d)<='f') ? (d)-'a'+10 : 0)
//
void DeUrlify( const BString& in, BString& out) {
	int32 outSize = in.Length()*2;
	if (!outSize) {
		out = "";
		return;
	}
	char* buf = out.LockBuffer( outSize+1);
	const char* pos = in.String();
	char* newPos = buf;
	char c1, c2;
	while( *pos) {
		if (*pos=='%' && (c1=*(pos+1)) && c1!='%' && (c2=*(pos+2))) {
			c1 = toupper(c1);
			c2 = toupper(c2);
			if (c1<'0' || c1>'9' && c1<'A' || c1>'F'
			||  c2<'0' || c2>'9' && c2<'A' || c2>'F') {
				*newPos++ = *pos++;
				continue;
			}
			*newPos++ = HEXDIGIT2CHAR(c1)*16+HEXDIGIT2CHAR(c2);
			pos+=3;
		} else
			*newPos++ = *pos++;
	}
	*newPos = 0;
	out.UnlockBuffer( newPos-buf);
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
	bool needBreak = false;
	for(	int32 pos = 0;  !needBreak;  pos += nl.Length(), lastPos = pos) {
		pos = in.FindFirst( nl, pos);
		if (pos == B_ERROR) {
			// handle the characters between last newline and end of string:
			pos = in.Length();
			needBreak = true;
		}
		// determine length of line in UTF8-characters (not bytes):
		int32 lineLen = 0;
		for( int i=lastPos; i<pos; ++i) {
			while( i<pos && IS_WITHIN_UTF8_MULTICHAR(s[i]))
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
				while( i<pos && IS_WITHIN_UTF8_MULTICHAR(s[i]))
					i++;
				lineLen++;
			}
		}
		out.Append( in.String()+lastPos, nl.Length()+pos-lastPos);
	}
	if (lastPos < in.Length())
		out.Append( in.String()+lastPos, in.Length()-lastPos);
}
