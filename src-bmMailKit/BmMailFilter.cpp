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

static const float GRAIN = 1.0;

const char* const BmMailFilter::MSG_FILTER = 	"bm:filter";
const char* const BmMailFilter::MSG_DELTA = 		"bm:delta";
const char* const BmMailFilter::MSG_TRAILING = 	"bm:trailing";
const char* const BmMailFilter::MSG_LEADING = 	"bm:leading";
const char* const BmMailFilter::MSG_REFS = 		"refs";

// alternate job-specifiers:
const int32 BmMailFilter::BM_EXECUTE_FILTER = 	1;

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
void BmMailFilter::SetMailFlags( sieve_imapflags_t* flags, MsgContext* msgContext) {
	if (msgContext && flags) {
		for( int i=0; i<flags->nflags; ++i) {
			BmString flag( flags->flag[i]);
			if (!flag.ICompare( "\\Seen")
			&& msgContext->mail->Status() != BM_MAIL_STATUS_READ) {
				msgContext->mail->MarkAs( BM_MAIL_STATUS_READ);
				msgContext->changed = true;
			}
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
		SetMailFlags( keepContext->imapflags, msgContext);
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
		SetMailFlags( fileintoContext->imapflags, msgContext);
		if (msgContext->mail->SetDestFoldername( fileintoContext->mailbox))
			msgContext->changed = true;
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
	ShouldContinue()
		-	determines whether or not the mail-filter should continue to run
		-	in addition to the inherited behaviour, the mail-filter should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmMailFilter::ShouldContinue() {
	return inherited::ShouldContinue() 
			 || CurrentJobSpecifier() == BM_EXECUTE_FILTER;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the job, executes the filter on all given mail-refs
\*------------------------------------------------------------------------------*/
bool BmMailFilter::StartJob() {
	try {
		int32 count = mMailRefs.size() + mMails.size();
		int32 c=0;
		const float delta =  100.0 / (count / GRAIN);
		BmRef<BmMail> mail;
		for( uint32 i=0; ShouldContinue() && i<mMailRefs.size(); ++i) {
			mail = BmMail::CreateInstance( mMailRefs[i].Get());
			if (mail) {
				mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				MsgContext msgContext( this, mail.Get());
				ExecuteFilter( msgContext);
			}
			BmString currentCount = BmString()<<c++<<" of "<<count;
			UpdateStatus( delta, mail->Name().String(), currentCount.String());
		}
		mMailRefs.clear();
		for( uint32 i=0; ShouldContinue() && i<mMails.size(); ++i) {
			MsgContext msgContext( this, mMails[i].Get());
			ExecuteFilter( msgContext);
			BmString currentCount = BmString()<<c++<<" of "<<count;
			UpdateStatus( delta, mMails[i]->Name().String(), 
							  currentCount.String());
		}
		mMails.clear();
		BmString currentCount = BmString()<<count<<" of "<<count;
		UpdateStatus( delta, "", currentCount.String());
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

typedef map< int32, BmRef<BmFilter> > BmOrderedFilterMap;
/*------------------------------------------------------------------------------*\
	ExecuteFilter()
		-	applies mail-filtering to a single given mail
\*------------------------------------------------------------------------------*/
void BmMailFilter::ExecuteFilter( MsgContext& msgContext) {
	if (!mFilter) {
		// sort all appropriate active filters by their position and then execute each:
		BmRef<BmFilterList> filterList = msgContext.mail->Outbound()
														? TheOutboundFilterList 
														: TheInboundFilterList;
		BmOrderedFilterMap orderedFilterMap;
		{
			BmAutolock lock( filterList->ModelLocker());
			lock.IsLocked() 							|| BM_THROW_RUNTIME( filterList->ModelNameNC() << ": Unable to get lock");
			BmModelItemMap::const_iterator iter;
			for( iter = filterList->begin(); iter != filterList->end(); ++iter) {
				BmFilter* filter = dynamic_cast< BmFilter*>( iter->second.Get());
				if (filter->Active())
					orderedFilterMap[filter->Position()] = filter;
			}
		}
		bool needToStore = false;
		BmOrderedFilterMap::const_iterator ordIter;
		for( ordIter = orderedFilterMap.begin(); ordIter != orderedFilterMap.end(); ++ordIter) {
			needToStore |= (ordIter->second->Execute( &msgContext) 
								&& msgContext.changed);
		}
		if (needToStore)
			msgContext.mail->Store();
	} else {
		if (mFilter->Execute( &msgContext) && msgContext.changed)
			msgContext.mail->Store();
	}
}

/*------------------------------------------------------------------------------*\
	UpdateStatus()
		-	informs the interested party about a change in the current state
\*------------------------------------------------------------------------------*/
void BmMailFilter::UpdateStatus( const float delta, const char* filename, 
										   const char* currentCount) {
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_FILTER, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, filename);
	msg->AddString( MSG_TRAILING, currentCount);
	TellControllers( msg.get());
}
