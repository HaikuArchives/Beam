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

#include <vector>
//#include <memory>

#include <Message.h>

#include "BmMailRef.h"
#include "BmUtil.h"

class BmFilter;

/*------------------------------------------------------------------------------*\
	BmMailFilter
		-	
\*------------------------------------------------------------------------------*/
class BmMailFilter : public BmJobModel {
	typedef BmJobModel inherited;

	typedef vector< BmRef< BmMailRef> > BmMailRefVect;
	typedef vector< BmRef< BmMail> > BmMailVect;
	typedef vector< const char**> BmHeaderVect;
	
	struct MsgContext {
		MsgContext( BmMailFilter* mf, BmMail* m)
			: mailFilter( mf), mail( m), headers( NULL), changed( false) 	{}
		~MsgContext() 							{ if (headers) delete [] headers; }
		BmMailFilter* mailFilter;
		BmMail* mail;
		const char** headers;
		bool changed;
	};

public:
	//	message component definitions for status-msgs:
	static const char* const MSG_FILTER;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;
	static const char* const MSG_REFS;

	// alternate job-specifiers:
	static const int32 BM_EXECUTE_FILTER;
							// for filter-execution without any controllers being present

	BmMailFilter( const BmString& name, BmFilter* filter);
	virtual ~BmMailFilter();

	// native methods:
	void AddMailRef( BmMailRef* ref);
	void AddMail( BmMail* mail);
	void ManageHeaderVect( const char**header);

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
	// SIEVE-helpers:
	static void SetMailFlags( sieve_imapflags_t* flags, MsgContext* msgContext);

	static int sieve_execute_error( const char* msg, void* interp_context,
											  void* script_context, void* message_context);

	// overrides of BmJobModel base:
	bool StartJob();
	bool ShouldContinue();

	// getters:
	inline BmString Name() const			{ return ModelName(); }

private:
	void UpdateStatus( const float delta, const char* filename, const char* currentCount);

	BmRef<BmFilter> mFilter;
							// the actual SIEVE-filter
	BmMailRefVect mMailRefs;
							// the mail-refs we shall be filtering
	BmMailVect mMails;
							// the mails we shall be filtering

	// Hide copy-constructor and assignment:
	BmMailFilter( const BmMailFilter&);
	BmMailFilter operator=( const BmMailFilter&);
};

#endif
