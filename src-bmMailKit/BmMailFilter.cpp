/*
	BmMailFilter.cpp
		- Implements the main POP3-client-class: BmMailFilter

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


#include <memory.h>
#include <memory>
#include <stdio.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmFilter.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

/*------------------------------------------------------------------------------*\
	BmMailFilter()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailFilter::BmMailFilter( const BmString& name, const BmFilter* filter, BmMail* mail)
	:	BmJobModel( name)
	,	mFilter( filter)
	,	mMail( mail)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailFilter::~BmMailFilter() { 
}

/*------------------------------------------------------------------------------*\
	sieve_redirect()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_redirect( void* action_context, void* interp_context, 
			   							 void* script_context, void* message_context, 
			   							 const char** errmsg) {
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_keep()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_keep( void* action_context, void* interp_context, 
			   				  		void* script_context, void* message_context, 
			   				  		const char** errmsg) {
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_fileinto()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_fileinto( void* action_context, void* interp_context, 
			   				  			 void* script_context, void* message_context, 
			   				 			 const char** errmsg) {
	BmMail* mail = static_cast< BmMail*>( message_context);
	sieve_fileinto_context* fileintoContext 
		= static_cast< sieve_fileinto_context*>( action_context);
	if (mail && fileintoContext) {
		mail->DestFoldername( fileintoContext->mailbox);
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_size()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_get_size( void* message_context, int* sizePtr) {
	BmMail* mail = static_cast< BmMail*>( message_context);
	if (mail && sizePtr) {
		*sizePtr = mail->RawText().Length();
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_header()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_get_header( void* message_context, const char* header,
			  									const char*** contents) {
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the job, executes the filter on the mail
\*------------------------------------------------------------------------------*/
bool BmMailFilter::StartJob() {
	try {
		return true;
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		BmString text = Name() << "\n\n" << errstr;
		BM_SHOWERR( BmString("BmMailFilter: ") << text);
	}
	return false;
}
