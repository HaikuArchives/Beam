/*
	BmUtil.h
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


#ifndef _BmUtil_h
#define _BmUtil_h

#include <ctime>

#include <Message.h>
#include <String.h>

extern BString BM_SPACES;
extern BString BM_DEFAULT_STRING;

/*------------------------------------------------------------------------------*\
	ShowAlert( text, logtext)
		-	pops up an Alert showing the passed text
		-	logs text unless logtext is specified, in which case that is 
			written to the logfile
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text);

/*------------------------------------------------------------------------------*\
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

/*------------------------------------------------------------------------------*\
	utility function to format a number of bytes into a string
\*------------------------------------------------------------------------------*/
BString BytesToString( int32 bytes, bool mini=false);

/*------------------------------------------------------------------------------*\
	utility function to format a time into a string
\*------------------------------------------------------------------------------*/
BString TimeToString( time_t t, const char* format="%Y-%m-%d %H:%M:%S");

/*------------------------------------------------------------------------------*\
	utility operator to easy concatenation of BStrings
\*------------------------------------------------------------------------------*/
BString operator+(const BString& s1, const BString& s2);
BString operator+(const char* s1, const BString& s2);
BString operator+(const BString& s1, const char* s2);
/*------------------------------------------------------------------------------*\
	utility function that removes a set of chars from a BString
	(needed since performance of BString::RemoveAll() is so pathetic...)
\*------------------------------------------------------------------------------*/
BString& RemoveSetFromString( BString& str, const char* chars);
/*------------------------------------------------------------------------------*\
	utility functions to convert between different linebreak-styles:
\*------------------------------------------------------------------------------*/
void ConvertLinebreaksToLF( const BString& in, BString& out);
void ConvertLinebreaksToCRLF( const BString& in, BString& out);
/*------------------------------------------------------------------------------*\
	utility function to wrap lines at word boundary:
\*------------------------------------------------------------------------------*/
void WordWrap( const BString& in, BString& out, int32 maxLineLen, BString nl);

#endif
