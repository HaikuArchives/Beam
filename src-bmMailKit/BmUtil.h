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

#include <Alert.h>
#include <Message.h>
#include <String.h>

extern BString BM_SPACES;
extern BString BM_DEFAULT_STRING;

/*------------------------------------------------------------------------------*\
	ShowAlert( text)
		-	pops up an Alert showing the passed text
\*------------------------------------------------------------------------------*/
void ShowAlert( const BString &text);

/*------------------------------------------------------------------------------*\
	ShowAlertWithType( text, type)
		-	pops up an Alert of given type, showing the passed text
\*------------------------------------------------------------------------------*/
void ShowAlertWithType( const BString &text, alert_type type);

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
	utility function to lower/upper a string (needed for Dano):
\*------------------------------------------------------------------------------*/
BString& BmToLower( BString& str);
BString& BmToUpper( BString& str);
/*------------------------------------------------------------------------------*\
	utility functions to convert between different linebreak-styles:
\*------------------------------------------------------------------------------*/
void ConvertLinebreaksToLF( const BString& in, BString& out);
void ConvertLinebreaksToCRLF( const BString& in, BString& out);
/*------------------------------------------------------------------------------*\
	utility function to convert tabs to a fixed number of spaces:
\*------------------------------------------------------------------------------*/
void ConvertTabsToSpaces( const BString& in, BString& out);
/*------------------------------------------------------------------------------*\
	utility function to convert URL-encoded chars (%xx) to real chars
\*------------------------------------------------------------------------------*/
void DeUrlify( const BString& in, BString& out);
/*------------------------------------------------------------------------------*\
	utility function to wrap lines at word boundary:
\*------------------------------------------------------------------------------*/
void WordWrap( const BString& in, BString& out, int32 maxLineLen, BString nl);
/*------------------------------------------------------------------------------*\
	utility defines for UTF8-character parsing:
\*------------------------------------------------------------------------------*/
#define IS_PART_OF_UTF8_MULTICHAR(c) ((c)&0x80)
#define IS_UTF8_STARTCHAR(c) (((c)&0xc0)==0xc0)
#define IS_WITHIN_UTF8_MULTICHAR(c) (((c)&0xc0)==0x80)

#endif
