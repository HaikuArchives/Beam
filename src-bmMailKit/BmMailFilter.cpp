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
#define BM_LOGNAME "Filter"

/*------------------------------------------------------------------------------*\
	BmMailFilter()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailFilter::BmMailFilter( const BmString& name, BmFilter* filter)
	:	BmJobModel( name)
	,	mFilter( filter)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailFilter::~BmMailFilter() { 
}

/*------------------------------------------------------------------------------*\
	AddMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFilter::AddMailRef( BmMailRef* ref) {
	if (ref)
		mMailRefs.push_back( ref);
}

/*------------------------------------------------------------------------------*\
	AddMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFilter::AddMail( BmMail* mail) {
	if (mail)
		mMails.push_back( mail);
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
	SetMailFlags()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFilter::SetMailFlags( sieve_imapflags_t* flags, BmMail* mail) {
	if (mail && flags) {
		for( int i=0; i<flags->nflags; ++i) {
			BmString flag( flags->flag[i]);
			if (!flag.ICompare( "\\Seen"))
				mail->MarkAs( BM_MAIL_STATUS_READ);
		}
	}
}

/*------------------------------------------------------------------------------*\
	sieve_keep()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_keep( void* action_context, void* interp_context, 
			   				  		void* script_context, void* message_context, 
			   				  		const char** errmsg) {
	MsgContext* msgContext = static_cast< MsgContext*>( message_context);
	sieve_keep_context* keepContext 
		= static_cast< sieve_keep_context*>( action_context);
	if (msgContext && msgContext->mail && keepContext)
		SetMailFlags( keepContext->imapflags, msgContext->mail);
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_fileinto()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_fileinto( void* action_context, void* interp_context, 
			   				  			 void* script_context, void* message_context, 
			   				 			 const char** errmsg) {
	MsgContext* msgContext = static_cast< MsgContext*>( message_context);
	sieve_fileinto_context* fileintoContext 
		= static_cast< sieve_fileinto_context*>( action_context);
	if (msgContext && msgContext->mail && fileintoContext) {
		SetMailFlags( fileintoContext->imapflags, msgContext->mail);
		msgContext->mail->DestFoldername( fileintoContext->mailbox);
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_size()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_get_size( void* message_context, int* sizePtr) {
	MsgContext* msgContext = static_cast< MsgContext*>( message_context);
	if (msgContext && msgContext->mail && sizePtr) {
		*sizePtr = msgContext->mail->RawText().Length();
	}
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	sieve_get_header()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_get_header( void* message_context, const char* header,
			  									const char*** contents) {
	MsgContext* msgContext = static_cast< MsgContext*>( message_context);
	if (msgContext && msgContext->mail) {
		msgContext->mail->Header()->GetAllFieldValues( header, msgContext->headers);
		*contents = msgContext->headers;
	}	
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	execute_error()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFilter::sieve_execute_error( const char* msg, void* interp_context,
													void* script_context, void* message_context) {
	BmString filterName = "<unknown>";
	BmString mailName = "<unknown>";
	if (script_context) {
		BmFilter* filter = static_cast< BmFilter*>( script_context);
		if (filter)
			filterName = filter->Name();
	}
	MsgContext* msgContext = static_cast< MsgContext*>( message_context);
	if (msgContext && msgContext->mail)
			mailName = msgContext->mail->Name();
	BmString err("An error occurred during execution of a mail-filter.");
	err << "\nFilter: " << filterName;
	err << "\nMail-ID: " << mailName;
	err << "\n\nError: " << (msg ? msg : "");
	BM_SHOWERR( err);
	return SIEVE_OK;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the job, executes the filter on all given mail-refs
\*------------------------------------------------------------------------------*/
bool BmMailFilter::StartJob() {
	try {
		BmRef<BmMail> mail;
		for( uint32 i=0; i<mMailRefs.size(); ++i) {
			mail = BmMail::CreateInstance( mMailRefs[i].Get());
			if (mail) {
				MsgContext msgContext( this, mail.Get());
				if (!mFilter->Execute( &msgContext))
					break;
			}
		}
		mMailRefs.clear();
		for( uint32 i=0; i<mMails.size(); ++i) {
			MsgContext msgContext( this, mMails[i].Get());
			if (!mFilter->Execute( &msgContext))
				break;
		}
		mMails.clear();
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
