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
#include "BmUtil.h"

BmString BM_SPACES("                                                                                                                                                                                    ");
BmString BM_DEFAULT_STRING;
BmString BM_DefaultItemLabel("<default>");
BmString BM_NoItemLabel("<none>");

/*------------------------------------------------------------------------------*\
	FindMsgString( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
const char* FindMsgString( BMessage* archive, const char* name, int32 index) {
	const char* str;
	BM_ASSERT(archive && name);
	if (archive->FindString( name, index, &str) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return str;
}

/*------------------------------------------------------------------------------*\
	FindMsgBool( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
bool FindMsgBool( BMessage* archive, const char* name, int32 index) {
	bool b;
	BM_ASSERT(archive && name);
	if (archive->FindBool( name, index, &b) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return b;
}

/*------------------------------------------------------------------------------*\
	FindMsgInt64( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int64 FindMsgInt64( BMessage* archive, const char* name, int32 index) {
	int64 i;
	BM_ASSERT(archive && name);
	if (archive->FindInt64( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
}

/*------------------------------------------------------------------------------*\
	FindMsgInt32( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int32 FindMsgInt32( BMessage* archive, const char* name, int32 index) {
	int32 i;
	BM_ASSERT(archive && name);
	if (archive->FindInt32( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
}

/*------------------------------------------------------------------------------*\
	FindMsgInt16( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
int16 FindMsgInt16( BMessage* archive, const char* name, int32 index) {
	int16 i;
	BM_ASSERT(archive && name);
	if (archive->FindInt16( name, index, &i) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return i;
}

/*------------------------------------------------------------------------------*\
	FindMsgMsg( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
BMessage* FindMsgMsg( BMessage* archive, const char* name, BMessage* msg, 
							 int32 index) {
	BM_ASSERT(archive && name);
	if (!msg)
		msg = new BMessage;
	if (archive->FindMessage( name, index, msg) != B_OK)
		throw BM_invalid_argument( BmString( "unknown message-field: ") << name);
	return msg;
}

/*------------------------------------------------------------------------------*\
	FindMsgFloat( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
float FindMsgFloat( BMessage* archive, const char* name, int32 index) {
	float f;
	BM_ASSERT(archive && name);
	if (archive->FindFloat( name, index, &f) != B_OK) {
		BmString s( "unknown message-field: ");
		s += name;
		throw BM_invalid_argument( s.String());
	}
	return f;
}

/*------------------------------------------------------------------------------*\
	FindMsgPointer( archive, name)
		-	extracts the msg-field with the specified name from the given archive
			and returns it.
		-	throws BM_invalid_argument if field is not contained withing archive
\*------------------------------------------------------------------------------*/
void* FindMsgPointer( BMessage* archive, const char* name, int32 index) {
	void* ptr;
	BM_ASSERT(archive && name);
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
	TimeToSwatchString( time)
		-	converts the given (utc) time into a string representing 
			swatch internet time
\*------------------------------------------------------------------------------*/
BmString TimeToSwatchString( time_t utc) {
	time_t swatch = utc+3600;	// add one hour
	int32 beats = ((swatch % 86400) * 1000) / 86400;
	return TimeToString( utc, "%Y-%m-%d @") << beats;
}

/*------------------------------------------------------------------------------*\
	ParseDateTime()
		-	parses the given string for a legal date
\*------------------------------------------------------------------------------*/
bool ParseDateTime( const BmString& str, time_t& dateTime) {
	if (!str.Length()) return false;
	// some mail-clients (notably BeMail!) generate date-formats with doubled
	// time-zone information which confuses parsedatetime(). 
	// N.B. Some other mailers enclose the same textual representation in
	// comments (parantheses), which is perfectly legal and will be handled
	// by the mail-header parsing code (the comments will not be present
	// in the string handled here).
	Regexx rx;
	// remove superfluous timezone information, i.e. convert something
	// like '12:11:10 +0200 CEST' to '12:11:10 +0200':
	const BmString s = rx.replace( str, "([+\\-]\\d+)[\\D]+$", "$1");
	// convert datetime-string into time_t:
	dateTime = parsedate( s.String(), -1);
	return dateTime != -1;
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

