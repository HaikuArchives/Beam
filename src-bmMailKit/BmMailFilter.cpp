/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <memory>
#include <stdio.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmMailHeader.h"
#include "BmRecvAccount.h"
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

/*------------------------------------------------------------------------------*\
	BmMailFilter()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailFilter::BmMailFilter( const BmString& name, BmFilter* filter, 
									 bool executeInMem, bool needControllers)
	:	BmJobModel( name)
	,	mFilter( filter)
	,	mMailRefs( NULL)
	,	mExecuteInMem( executeInMem)
{
	NeedControllersToContinue( needControllers);
}

/*------------------------------------------------------------------------------*\
	~BmMailFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailFilter::~BmMailFilter() { 
	delete mMailRefs;
}

/*------------------------------------------------------------------------------*\
	SetMailRefVect()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFilter::SetMailRefVect( BmMailRefVect* refVect) {
	if (refVect != mMailRefs) {
		delete mMailRefs;
		mMailRefs = refVect;
	}
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
		-	in addition to the inherited behaviour, the mail-filter should 
			continue when it executes special jobs (not BM_DEFAULT_JOB), since 
			in that case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmMailFilter::ShouldContinue() {
	return inherited::ShouldContinue() 
			 || !NeedControllersToContinue();
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the job, executes the filter on all given mail-refs
\*------------------------------------------------------------------------------*/
bool BmMailFilter::StartJob() {
	try {
		int32 count = mMails.size();
		int32 c=0;
		if (mMailRefs)
			count += mMailRefs->size();
		BM_LOG2( BM_LogFilter, 
					BmString("Starting filter-job for ") << count << " mails.");
		const float delta =  100.0 / (count / GRAIN);
		if (mMailRefs) {
			BmRef<BmMail> mail;
			for( uint32 i=0; ShouldContinue() && i<mMailRefs->size(); ++i) {
				mail = BmMail::CreateInstance( (*mMailRefs)[i].Get());
				if (mail) {
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
					if (mail->InitCheck() == B_OK)
					Execute( mail.Get());
				}
				BmString currentCount = BmString()<<++c<<" of "<<count;
				UpdateStatus( delta, mail->Name().String(), currentCount.String());
			}
			mMailRefs->clear();
		}
		for( uint32 i=0; ShouldContinue() && i<mMails.size(); ++i) {
			if (mMails[i]->InitCheck() != B_OK)
				continue;
			Execute( mMails[i].Get());
			BmString currentCount = BmString()<<++c<<" of "<<count;
			UpdateStatus( delta, mMails[i]->Name().String(), 
							  currentCount.String());
		}
		mMails.clear();
		BmString currentCount = BmString()<<c<<" of "<<count;
		UpdateStatus( delta, "", currentCount.String());
		if (ShouldContinue())
			BM_LOG2( BM_LogFilter, "Filter-job has finished.");
		else
			BM_LOG2( BM_LogFilter, "Filter-job has been stopped.");
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
	Execute()
		-	applies mail-filtering to a single given mail
\*------------------------------------------------------------------------------*/
void BmMailFilter::Execute( BmMail* mail) {
	BmMsgContext msgContext;
	msgContext.mail = mail;

	BmRef< BmListModelItem> accItem 
		= TheRecvAccountList->FindItemByKey( mail->AccountName());
	BmRecvAccount* recvAcc = dynamic_cast< BmRecvAccount*>( accItem.Get());
	if (!mail->Outbound() && recvAcc) {
		// set default folder for this mail according to its receiving account
		mail->SetDestFolderName(recvAcc->HomeFolder());
	}

	if (!mFilter) {
		BM_LOG2( BM_LogFilter, 
					BmString("Searching filter-chain for mail with Id <") 
						<< mail->Name() << ">...");
		// first we find the correct filter-chain for this mail...
		BmRef< BmListModelItem> chainItem;
		if (mail->Outbound()) {
			// use the outbound chain:
			chainItem = TheFilterChainList->FindItemByKey(BM_OutboundLabel);
		} else {
			// fetch the corresponding inbound-chain:
			if (recvAcc)
				chainItem 
					= TheFilterChainList->FindItemByKey( recvAcc->FilterChain());
			else
				chainItem 
					= TheFilterChainList->FindItemByKey( BM_DefaultItemLabel);
		}
		BmFilterChain* chain = dynamic_cast< BmFilterChain*>( chainItem.Get());
		if (chain) {
			BM_LOG2( BM_LogFilter, 
						BmString("...found chain ") << chain->DisplayKey() 
							<< ", applying all its filters...");
			// execute all the chain's filters:
			BmAutolockCheckGlobal lock( chain->ModelLocker());
			if (!lock.IsLocked())
				BM_THROW_RUNTIME( chain->ModelNameNC() << ": Unable to get lock");
			BmFilterPosVect::const_iterator iter;
			for( iter = chain->posBegin(); iter != chain->posEnd(); ++iter) {
				BmChainedFilter* chainedFilter = *iter;
				BmRef< BmListModelItem> filterItem 
					= TheFilterList->FindItemByKey( chainedFilter->Key());
				BmFilter* filter = dynamic_cast< BmFilter*>( filterItem.Get());
				if (filter) {
					if (!ExecuteFilter( mail, filter, &msgContext))
						break;
				}
			}
		} else {
			BM_LOG2( BM_LogFilter, "...no chain found -> nothing to do.");
			return;
		}
	} else
		ExecuteFilter( mail, mFilter.Get(), &msgContext);
	bool needToStore = false;
	bool learnAsSpam = msgContext.GetBool("LearnAsSpam");
	if (learnAsSpam) {
		BmRef<BmFilter> learnAsSpamFilter = TheFilterList->LearnAsSpamFilter();
		if (learnAsSpamFilter)
			learnAsSpamFilter->Execute( &msgContext);
		needToStore = true;
	}
	bool learnAsTofu = msgContext.GetBool("LearnAsTofu");
	if (learnAsTofu) {
		BmRef<BmFilter> learnAsTofuFilter = TheFilterList->LearnAsTofuFilter();
		if (learnAsTofuFilter)
			learnAsTofuFilter->Execute( &msgContext);
		needToStore = true;
	}
	BmString newIdentity = msgContext.GetString("Identity");
	if (newIdentity.Length() && newIdentity != mail->IdentityName()) {
		mail->IdentityName( newIdentity);
		needToStore = true;
	}
	BmString newListId = msgContext.GetString("ListId");
	if (newListId.Length() && newListId != mail->GetFieldVal(BM_FIELD_LIST_ID)) {
		mail->SetFieldVal(BM_FIELD_LIST_ID, newListId);
		mail->ReconstructRawText();
		needToStore = true;
	}
	BmString newStatus = msgContext.GetString("Status");
	if (newStatus.Length() && newStatus != mail->Status()) {
		mail->MarkAs( newStatus.String());
		needToStore = true;
	}
	BmString rejectMsg = msgContext.GetString("RejectMsg");
	if (rejectMsg.Length()) {
		// ToDo (maybe): implement sending of MDN
	}
	bool moveToTrash = msgContext.GetBool("MoveToTrash");
	if (moveToTrash) {
		mail->MoveToTrash( true);
		needToStore = true;
	}
	if (msgContext.HasField("RatioSpam")) {
		double ratioSpam = msgContext.GetDouble("RatioSpam");
		if (ratioSpam != mail->RatioSpam()) {
			mail->RatioSpam(ratioSpam);
			needToStore = true;
		}
	}
	bool isSpam = msgContext.GetBool("IsSpam");
	if (isSpam && !mail->IsMarkedAsSpam()) {
		mail->MarkAsSpam();
		needToStore = true;
	}
	bool isTofu = msgContext.GetBool("IsTofu");
	if (isTofu && !mail->IsMarkedAsTofu()) {
		mail->MarkAsTofu();
		needToStore = true;
	}
	BmString newFolderName = msgContext.GetString("FolderName");
	if (newFolderName.Length()) {
		if (mail->SetDestFolderName( newFolderName)) {
			if (!needToStore && !mExecuteInMem) {
				// optimize the (usual) case where folder-change is the only thing
				// that has happened, if so, we just move the mail (but do not
				// rewrite it completely);
				if (!mail->MoveToDestFolder())
					needToStore = true;
			} else
				needToStore = true;
		}
	}
	if (needToStore && !mExecuteInMem) {
		BM_LOG3( BM_LogFilter, 
					"Filtering has changed something, so mail will be stored now.");
		mail->Store();
	}
}

/*------------------------------------------------------------------------------*\
	ExecuteFilter()
		-	applies a specific filter to a single given mail
		-	return true if processing should continue, false if not
\*------------------------------------------------------------------------------*/
bool BmMailFilter::ExecuteFilter( BmMail* mail, BmFilter* filter,
											 BmMsgContext* msgContext) {
	if (filter->IsDisabled()) {
		BM_LOG2( BM_LogFilter, 
					BmString("Addon for Filter ") << filter->Name() 
						<< " (type=" << filter->Kind() 
						<< ") has not been loaded, we skip this filter.");
	} else {
		BM_LOG2( BM_LogFilter, 
					BmString("Executing Filter ") << filter->Name() 
						<< " (type=" << filter->Kind() << ")...");
		msgContext->ResetChanges();
		filter->Execute( msgContext);
		if (msgContext->FieldHasChanged("Identity")) {
			BmString newIdentity = msgContext->GetString("Identity");
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  		<< ": setting identity to " 
					  		<< newIdentity);
		}
		if (msgContext->FieldHasChanged("Status")) {
			BmString newStatus = msgContext->GetString("Status");
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  		<< ": setting status to " 
					  		<< newStatus);
		}
		if (msgContext->FieldHasChanged("MoveToTrash")
		&& msgContext->GetBool("MoveToTrash")) {
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  		<< ": moving mail to trash.");
		}
		if (msgContext->FieldHasChanged("RejectMsg")) {
			BmString rejectMsg = msgContext->GetString("RejectMsg");
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  		<< ": rejecting mail with msg " 
					  		<< rejectMsg);
		}
		if (msgContext->FieldHasChanged("ListId")) {
			BmString newListId = msgContext->GetString("ListId");
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  		<< ": setting ListId to " 
					  		<< newListId);
		}
		bool stopProcessing = msgContext->GetBool("StopProcessing");
		if (stopProcessing) {
			BM_LOG( BM_LogFilter, 
					  BmString("Filter ") << filter->Name() 
					  	<< ": wants to stop processing this chain.");
			return false;
		}
	}
	return true;
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
	if (!ShouldContinue())
		msg->AddString( MSG_TRAILING, 
							(BmString(currentCount) << ", Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, currentCount);
	TellControllers( msg.get());
}
