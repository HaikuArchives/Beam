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
#include "BmFilterChain.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmPopAccount.h"
#include "BmSmtpAccount.h"
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
const int32 BmMailFilter::BM_EXECUTE_FILTER_IN_MEM = 	1;

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
	ShouldContinue()
		-	determines whether or not the mail-filter should continue to run
		-	in addition to the inherited behaviour, the mail-filter should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmMailFilter::ShouldContinue() {
	return inherited::ShouldContinue() 
			 || CurrentJobSpecifier() == BM_EXECUTE_FILTER_IN_MEM;
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
				ExecuteFilter( mail.Get());
			}
			BmString currentCount = BmString()<<c++<<" of "<<count;
			UpdateStatus( delta, mail->Name().String(), currentCount.String());
		}
		mMailRefs.clear();
		for( uint32 i=0; ShouldContinue() && i<mMails.size(); ++i) {
			ExecuteFilter( mMails[i].Get());
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

/*------------------------------------------------------------------------------*\
	ExecuteFilter()
		-	applies mail-filtering to a single given mail
\*------------------------------------------------------------------------------*/
void BmMailFilter::ExecuteFilter( BmMail* mail) {
	BmMsgContext msgContext( mail->RawText(), mail->Name());
	mail->Header()->GetAllFieldValues( msgContext);
	if (!mFilter) {
		// first we find the correct filter-chain for this mail...
		BmRef< BmListModelItem> accItem;
		BmRef< BmListModelItem> chainItem;
		if (mail->Outbound()) {
			// we have only one outbound chain:
			chainItem = TheFilterChainList->FindItemByKey( BM_DefaultOutItemLabel);
		} else {
			// fetch the corresponding inbound-chain:
			accItem = ThePopAccountList->FindItemByKey( mail->AccountName());
			BmPopAccount* popAcc = dynamic_cast< BmPopAccount*>( accItem.Get());
			if (popAcc)
				chainItem = TheFilterChainList->FindItemByKey( popAcc->FilterChain());
			else
				chainItem = TheFilterChainList->FindItemByKey( BM_DefaultItemLabel);
		}
		BmFilterChain* chain = dynamic_cast< BmFilterChain*>( chainItem.Get());
		if (chain) {
			// execute all the chain's filters:
			BmAutolockCheckGlobal lock( chain->ModelLocker());
			lock.IsLocked() 					|| BM_THROW_RUNTIME( chain->ModelNameNC() << ": Unable to get lock");
			BmModelItemMap::const_iterator iter;
			for( iter = chain->BmListModel::begin(); iter != chain->BmListModel::end(); ++iter) {
				BmChainedFilter* chainedFilter = dynamic_cast< BmChainedFilter*>( iter->second.Get());
				BmRef< BmListModelItem> filterItem = TheFilterList->FindItemByKey( chainedFilter->FilterName());
				BmFilter* filter = dynamic_cast< BmFilter*>( filterItem.Get());
				if (filter && filter->Addon())
					filter->Addon()->Execute( &msgContext);
			}
		}
	} else {
		if (mFilter->Addon())
			mFilter->Addon()->Execute( &msgContext);
	}
	bool needToStore = false;
	if (msgContext.folderName.Length()) {
		if (mail->SetDestFoldername( msgContext.folderName))
			needToStore = true;
	}
	if (msgContext.identity.Length()) {
		mail->IdentityName( msgContext.identity);
		needToStore = true;
	}
	if (msgContext.status.Length() && mail->Status() != msgContext.status) {
		mail->MarkAs( msgContext.status.String());
		needToStore = true;
	}
	if (needToStore && CurrentJobSpecifier()!=BM_EXECUTE_FILTER_IN_MEM)
		mail->Store();
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
