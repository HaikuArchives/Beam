/*
	BmMailFilter.h

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


#ifndef _BmMailFilter_h
#define _BmMailFilter_h

#include <memory>

#include <Message.h>

#include "BmUtil.h"

class BmFilter;
class BmMail;

/*------------------------------------------------------------------------------*\
	BmMailFilter
		-	
\*------------------------------------------------------------------------------*/
class BmMailFilter : public BmJobModel {
	typedef BmJobModel inherited;
	
public:
	BmMailFilter( const BmString& name, const BmFilter* filter, BmMail* mail);
	virtual ~BmMailFilter();

	// SIEVE-callbacks:
	static int sieve_redirect( void* action_context, void* interp_context, 
			   						void* script_context, void* message_context, 
			   						const char** errmsg);
	static int sieve_keep( void* action_context, void* interp_context, 
			   				  void* script_context, void* message_context, 
			   				  const char** errmsg);
	static int sieve_fileinto( void* action_context, void* interp_context, 
			   				  		void* script_context, void* message_context, 
			   				  		const char** errmsg);
	static int sieve_get_size( void* message_context, int* size);
	static int sieve_get_header( void* message_context, const char* header,
			  							  const char*** contents);

	// overrides of BmJobModel base:
	bool StartJob();

	// getters:
	inline BmString Name() const			{ return ModelName(); }

private:
	const BmFilter* mFilter;
							// the actual SIEVE-filter
	BmMail* mMail;
							// the mail we are filtering

	// Hide copy-constructor and assignment:
	BmMailFilter( const BmMailFilter&);
	BmMailFilter operator=( const BmMailFilter&);
};

#endif
