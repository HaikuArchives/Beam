/*
	BmBasics.cpp

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

#ifdef __POWERPC__
#define BM_BUILDING_BMBASE 1
#endif

#include "BmBasics.h"

/********************************************************************************\
	BM_error
\********************************************************************************/

BM_error::BM_error( const BmString& what_arg)
	:	mWhat( what_arg)
{
}

BM_error::BM_error( const char* what_arg)
	:	mWhat( what_arg)
{
}

const char* const BM_error::what() const {
	return mWhat.String();
}

/********************************************************************************\
	BM_runtime_error
\********************************************************************************/

BM_runtime_error::BM_runtime_error( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_runtime_error::BM_runtime_error( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_invalid_argument
\********************************************************************************/

BM_invalid_argument::BM_invalid_argument( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_invalid_argument::BM_invalid_argument( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_network_error
\********************************************************************************/

BM_network_error::BM_network_error( const BmString& what_arg)
	:	inherited( what_arg.String())
{
}

BM_network_error::BM_network_error( const char* what_arg)
	:	inherited( what_arg)
{
}

/********************************************************************************\
	BM_text_error
\********************************************************************************/

BM_text_error::BM_text_error( const BmString& what_arg, int32 pos)
	:	inherited( what_arg.String())
	,	posInText( pos)
{
}

BM_text_error::BM_text_error( const char* what_arg, int32 pos)
	:	inherited( what_arg)
	,	posInText( pos)
{
}

/********************************************************************************\
	throwing-helpers...
\********************************************************************************/

bool BM_Throw_Runtime( const BmString &s, int line, const char* file) { 
	throw BM_runtime_error(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}

bool BM_Throw_Invalid( const BmString &s, int line, const char* file) { 
	throw BM_invalid_argument(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}

bool BM_Throw_Network( const BmString &s, int line, const char* file) { 
	throw BM_network_error(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}