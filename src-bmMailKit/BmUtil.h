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
#include "BmString.h"

extern BmString BM_SPACES;
extern BmString BM_DEFAULT_STRING;
extern BmString BM_DefaultItemLabel;

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
BmString BytesToString( int32 bytes, bool mini=false);

/*------------------------------------------------------------------------------*\
	time-related utility functions
\*------------------------------------------------------------------------------*/
BmString TimeToSwatchString( time_t t);
bool ParseDateTime( const BmString& str, time_t& dateTime);

/*------------------------------------------------------------------------------*\
	utility function to generate the sortkey for a given name:
\*------------------------------------------------------------------------------*/
BmString GenerateSortkeyFor( const BmString& name);
/*------------------------------------------------------------------------------*\
	utility defines for UTF8-character parsing:
\*------------------------------------------------------------------------------*/
#define IS_PART_OF_UTF8_MULTICHAR(c) ((c)&0x80)
#define IS_UTF8_STARTCHAR(c) (((c)&0xc0)==0xc0)
#define IS_WITHIN_UTF8_MULTICHAR(c) (((c)&0xc0)==0x80)

#endif
