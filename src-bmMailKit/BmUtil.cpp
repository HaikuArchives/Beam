/*
	BmUtil.cpp
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <ctime>
#include <parsedate.h>

#include <Alert.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMemIO.h"
#include "BmPrefs.h"
#include "BmUtil.h"

BmString BM_SPACES("                                                                                                                                                                                    ");
BmString BM_DEFAULT_STRING;
BmString BM_DefaultItemLabel("<default>");

/*------------------------------------------------------------------------------*\
	FindMsgString( archive, name)
		-	extracts the msg-field with the specified name from the given archive and
			returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
const char* FindMsgString( BMessage* archive, const char* name, int32 index) {
	const char* str;
	BM_assert(archive && name);
	if (archive->FindString( name, index, &str) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return str;
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
	if (archive->FindBool( name, index, &b) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return b;
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
	if (archive->FindInt64( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
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
	if (archive->FindInt32( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
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
	if (archive->FindInt16( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
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
	if (archive->FindMessage( name, index, msg) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return msg;
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
	if (archive->FindFloat( name, index, &f) != B_OK) {
		BmString s( "unknown message-field: ");
		s += name;
		throw BM_invalid_argument( s.String());
	}
	return f;
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
	if (archive->FindPointer( name, index, &ptr) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return ptr;
}

/*------------------------------------------------------------------------------*\
	BytesToString( bytes)
		-	returns the given number of bytes as a short, descriptive string:
			* bytes < 1024 				-> "X bytes"
			* 1024 < bytes < 1024000	-> "X.xx KB"
			* 1048576 < bytes 			-> "X.xx MB"
		-	we define a Mega-Byte as 1024**2 bytes (as Tracker does)
\*------------------------------------------------------------------------------*/
BmString BytesToString( int32 bytes, bool mini) {
	char buf[20];
	if (bytes >= 1048576) {
		sprintf( buf, "%6.2f MB", bytes/1048576.0);
	} else if (bytes >= 1024) {
		sprintf( buf, "%6.2f KB", bytes/1024.0);
	} else {
		sprintf( buf, "%ld %s", bytes, mini ? "b" : "bytes");
	}
	return BmString(buf);
}

/*------------------------------------------------------------------------------*\
	TimeToString( time)
		-	converts the given time into a string
\*------------------------------------------------------------------------------*/
BmString TimeToString( time_t utc, const char* format) {
	BmString s;
	const int32 bufsize=100;
	s.SetTo( '\0', bufsize);
	char* buf=s.LockBuffer( bufsize);
	struct tm ltm;
	localtime_r( &utc, &ltm);
	strftime( buf, bufsize, format, &ltm);
	s.UnlockBuffer( strlen(buf));
	return s;
}

/*------------------------------------------------------------------------------*\
	TimeToSwatchString( time)
		-	converts the given (local) time into a string representing 
			swatch internet time
\*------------------------------------------------------------------------------*/
BmString TimeToSwatchString( time_t utc) {
	time_t swatch = utc+3600;	// add one hour
	int32 beats = ((swatch % 86400) * 1000) / 86400;
	return TimeToString( swatch, "%Y-%m-%d @") << beats;
}

/*------------------------------------------------------------------------------*\
	ParseDateTime()
		-	parses the given string for a legal date
\*------------------------------------------------------------------------------*/
bool ParseDateTime( const BmString& str, time_t& dateTime) {
	if (!str.Length()) return false;
	dateTime = parsedate( str.String(), -1);
	return dateTime != -1;
}

/*------------------------------------------------------------------------------*\*\
	ShowAlert( text)
		-	pops up an Alert showing the passed (error-)text
\*------------------------------------------------------------------------------*/
void ShowAlert( const BmString &text) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->Go();
}

/*------------------------------------------------------------------------------*\*\
	ShowAlertWithType( text, type)
		-	pops up an Alert of given type, showing the passed text
\*------------------------------------------------------------------------------*/
void ShowAlertWithType( const BmString &text, alert_type type) {
	BAlert* alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
										 B_WIDTH_AS_USUAL, type);
	alert->Go();
}

/*------------------------------------------------------------------------------*\
	WordWrap( in, out, maxLineLen)
		-	wraps given in-string along word-boundary
		-	param maxLineLen indicates right border for wrap
		-	resulting text is stored in param out
		-	the string in has to be UTF8-encoded for this function to work correctly!
		-	if keepLongWords is set, single words whose length exceeds maxLineLen 
			(like URLs) will be preserved (i.e. not be wrapped).
\*------------------------------------------------------------------------------*/
void WordWrap( const BmString& in, BmString& out, int32 maxLineLen, BmString nl, 
					bool keepLongWords) {
	if (!in.Length()) {
		out.Truncate( 0, false);
		return;
	}
	Regexx rx;
	int32 lastPos = 0;
	const char *s = in.String();
	bool needBreak = false;
	BmStringOBuf tempIO( in.Length()*1.1, 1.1);
	for( int32 pos = 0;  !needBreak;  pos += nl.Length(), lastPos = pos) {
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
		int32 lastSpcPos;
		while (lineLen > maxLineLen) {
			lastSpcPos = in.FindLast( " ", lastPos+maxLineLen);
			if (keepLongWords && lastSpcPos>lastPos) {
				// special-case lines containing only quotes and a long word,
				// since in this case we want to avoid wrapping between quotes
				// and the word:
				BmString lineBeforeSpace;
				in.CopyInto( lineBeforeSpace, lastPos, 1+lastSpcPos-lastPos);
				if (rx.exec( lineBeforeSpace, ThePrefs->GetString( "QuotingLevelRX"))) {
					BmString text=rx.match[0].atom[1];
					if (!text.Length()) {
						// the subpart before last space consists only of the quote,
						// we avoid wrapping this line:
						lastSpcPos=B_ERROR;
					}
				}
			}
			if (lastSpcPos==B_ERROR || lastSpcPos<lastPos) {
				// line doesn't contain any space character (before maxline-length), 
				// we simply break it at right margin (unless keepLongWords is set):
				if (keepLongWords) {
					// find next space or end of line and break line there:
					int32 nextSpcPos = in.FindFirst( " ", lastPos+maxLineLen);
					int32 nlPos = in.FindFirst( nl, lastPos+maxLineLen);
					if (nextSpcPos==B_ERROR || nlPos<nextSpcPos) {
						// have no space in line, we keep whole line:
						if (nlPos == B_ERROR) {
							tempIO.Write( in.String()+lastPos, in.Length()-lastPos);
							tempIO.Write( nl);
							lastPos = in.Length();
						} else {
							tempIO.Write( in.String()+lastPos, nl.Length()+nlPos-lastPos);
							lastPos = nlPos + nl.Length();
						}
					} else {
						// break long line at a space behind right margin:
						tempIO.Write( in.String()+lastPos, 1+nextSpcPos-lastPos);
						tempIO.Write( nl);
						lastPos = nextSpcPos+1;
					}
				} else {
					// break line at right margin:
					tempIO.Write( in.String()+lastPos, maxLineLen);
					tempIO.Write( nl);
					lastPos += maxLineLen;
				}
			} else {
				// wrap line after last space:
				tempIO.Write( in.String()+lastPos, 1+lastSpcPos-lastPos);
				tempIO.Write( nl);
				lastPos = lastSpcPos+1;
			}
			lineLen = 0;
			for( int i=lastPos; i<pos; ++i) {
				while( i<pos && IS_WITHIN_UTF8_MULTICHAR(s[i]))
					i++;
				lineLen++;
			}
		}
		if (needBreak)
			break;
		tempIO.Write( in.String()+lastPos, nl.Length()+pos-lastPos);
	}
	if (lastPos < in.Length())
		tempIO.Write( in.String()+lastPos, in.Length()-lastPos);
	out.Adopt( tempIO.TheString());
}

/*------------------------------------------------------------------------------*\
	GenerateSortkeyFor( name))
		-	returns the sortkey for the given name
\*------------------------------------------------------------------------------*/
BmString GenerateSortkeyFor( const BmString& name) {
	BmString skey( name);
	if (!skey.Length()) 
		return skey;
	skey.ToLower();
	skey.IReplaceAll( "ä", "ae");
	skey.IReplaceAll( "ö", "oe");
	skey.IReplaceAll( "ü", "ue");
	skey.ReplaceAll( "ß", "ss");
	// that's it for now, decomposition for other chars later...
	return skey;
}

