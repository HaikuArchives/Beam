/*
	BmSmtpAccount.cpp

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


#include <ByteOrder.h>
#include <File.h>
#include <Message.h>

#include "BmBasics.h"
#include "BmSmtpAccount.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSmtpAccount
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmSmtpAccount()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmSmtpAccount::BmSmtpAccount( const char* name, BmSmtpAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mPortNr( 25)
	,	mPortNrString( "25")
	,	mPwdStoredOnDisk( false)
{
}

/*------------------------------------------------------------------------------*\
	BmSmtpAccount( archive)
		-	c'tor
		-	constructs a BmSmtpAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmSmtpAccount::BmSmtpAccount( BMessage* archive, BmSmtpAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
{
	int16 version = FindMsgInt16( archive, MSG_VERSION);
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mSMTPServer = FindMsgString( archive, MSG_SMTP_SERVER);
	mDomainToAnnounce = FindMsgString( archive, MSG_DOMAIN);
	mAuthMethod = FindMsgString( archive, MSG_AUTH_METHOD);
	BmToUpper( mAuthMethod);
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << mPortNr;
	mPwdStoredOnDisk = FindMsgBool( archive, MSG_STORE_PWD);
	if (version >= 2) {
		mAccForSmtpAfterPop = FindMsgString( archive, MSG_ACC_FOR_SAP);
	}
}

/*------------------------------------------------------------------------------*\
	~BmSmtpAccount()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmSmtpAccount::~BmSmtpAccount() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmSmtpAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmSmtpAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_SMTP_SERVER, mSMTPServer.String())
		||	archive->AddString( MSG_DOMAIN, mDomainToAnnounce.String())
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddString( MSG_ACC_FOR_SAP, mAccForSmtpAfterPop));
	return ret;
}

/*------------------------------------------------------------------------------*\
	SMTPAddress()
		-	returns the SMTP-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::GetSMTPAddress( BNetAddress* addr) const {
	return addr->SetTo( mSMTPServer.String(), mPortNr) == B_OK;
}

/*------------------------------------------------------------------------------*\
	NeedsAuthViaPopServer()
		-	determines if this SMTP-account requires authentication through a
			corresponding POP-server
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::NeedsAuthViaPopServer() {
	return mAuthMethod.ICompare(AUTH_SMTP_AFTER_POP) == 0;
}


/********************************************************************************\
	BmSmtpAccountList
\********************************************************************************/

BmRef< BmSmtpAccountList> BmSmtpAccountList::theInstance( NULL);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmSmtpAccountList* BmSmtpAccountList::CreateInstance( BLooper* jobMetaController) {
	if (!theInstance) {
		theInstance = new BmSmtpAccountList( jobMetaController);
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmSmtpAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmSmtpAccountList::BmSmtpAccountList( BLooper* jobMetaController)
	:	inherited( "SmtpAccountList")
	,	mJobMetaController( jobMetaController)
{
}

/*------------------------------------------------------------------------------*\
	~BmSmtpAccountList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmSmtpAccountList::~BmSmtpAccountList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns name of settings-file for list of SMTP-accounts
\*------------------------------------------------------------------------------*/
const BString BmSmtpAccountList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Smtp Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	fetches SMTP-accounts from given message-archive
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BString("Start of InstantiateItems() for SmtpAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find smtp-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmSmtpAccount* newAcc = new BmSmtpAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BString("SmtpAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InstantiateMailRefs() for SmtpAccountList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	SendQueuedMailFor( accName)
		-	sends all queued mails for the account specified by accName
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::SendQueuedMailFor( const BString accName) {
	BMessage archive(BM_JOBWIN_SMTP);
	archive.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	mJobMetaController->PostMessage( &archive);
}
