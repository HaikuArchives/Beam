/*
	BmBasics.h
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

#ifndef _BmBasics_h
#define _BmBasics_h

#include <stdexcept>
#include <string>

#include "BmString.h"

#define BM_assert(expr) if (!expr) { throw BM_invalid_argument(BmString("BM_assert says no at ")<<__FILE__<<":"<<__LINE__); }

void ShowAlert( const BmString &text);	// forward declaration

/*------------------------------------------------------------------------------*\*\
	BM_runtime_error
		-	exception to indicate a general runtime error
\*------------------------------------------------------------------------------*/
class BM_runtime_error : public runtime_error {
	typedef runtime_error inherited;
public:
	BM_runtime_error (const BmString& what_arg): inherited (what_arg.String()) { }
	BM_runtime_error (const char* const what_arg): inherited (string(what_arg)) { }
};

/*------------------------------------------------------------------------------*\*\
	BM_invalid_argument
		-	exception to indicate an invalid-argument error
\*------------------------------------------------------------------------------*/
class BM_invalid_argument : public invalid_argument {
	typedef invalid_argument inherited;
public:
	BM_invalid_argument (const BmString& what_arg): inherited (what_arg.String()) { }
	BM_invalid_argument (const char* const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BM_network_error
		-	exception to indicate an error during network communication
\*------------------------------------------------------------------------------*/
class BM_network_error : public BM_runtime_error {
	typedef BM_runtime_error inherited;
public:
	BM_network_error (const BmString& what_arg): inherited (what_arg.String()) { }
	BM_network_error (const char* const what_arg): inherited (what_arg) { }
};

/*------------------------------------------------------------------------------*\*\
	BM_text_error
		-	exception to indicate a problem with a given text
		-	additionally, a character position may be given that indicates
			the position of the problem within the text
\*------------------------------------------------------------------------------*/
class BM_text_error : public runtime_error {
	typedef runtime_error inherited;
public:
	BM_text_error (const BmString& what_arg, int32 pos=-1)
		: inherited (what_arg.String())
		, posInText( pos)						{ }
	BM_text_error (const char* const what_arg, int32 pos=-1)
		: inherited (string(what_arg)) 
		, posInText( pos)						{ }
	int32 posInText;
};

/*------------------------------------------------------------------------------*\*\
	BM_THROW_...
		-	throws exception of specific type
\*------------------------------------------------------------------------------*/
#define BM_THROW_RUNTIME(s) BM_Throw_Runtime(s,__LINE__,__FILE__)
inline bool BM_Throw_Runtime( const BmString &s, int line, const char* file) { 
	throw BM_runtime_error(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}
#define BM_THROW_INVALID(s) BM_Throw_Invalid(s,__LINE__,__FILE__)
inline bool BM_Throw_Invalid( const BmString &s, int line, const char* file) { 
	throw BM_invalid_argument(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}
#define BM_THROW_NETWORK(s) BM_Throw_Network(s,__LINE__,__FILE__)
inline bool BM_Throw_Network( const BmString &s, int line, const char* file) { 
	throw BM_network_error(BmString("*** Exception at ")<<file<<":"<<line<<" ***\n"<<s); 
}

/*------------------------------------------------------------------------------*\*\
	utility defines to shorten the use of auto_ptrs
\*------------------------------------------------------------------------------*/
#define BmPtr const auto_ptr
#define BmNcPtr auto_ptr

/*------------------------------------------------------------------------------*\*\
	wrapper around BAutolock that enhances profiling output
\*------------------------------------------------------------------------------*/
// during profiling we use this:
/*
#include <Autolock.h>
class BmAutolock : public BAutolock {
public:
	BmAutolock( BLooper* l) : BAutolock( l) {};
	BmAutolock( BLocker* l) : BAutolock( l) {};
	BmAutolock( BLocker& l) : BAutolock( l) {};
};
*/
//otherwise, we use this:
#define BmAutolock BAutolock

#endif
