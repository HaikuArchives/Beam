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

const char* const BmSmtpAccount::MSG_NAME = 			"bm:name";
const char* const BmSmtpAccount::MSG_USERNAME = 	"bm:username";
const char* const BmSmtpAccount::MSG_PASSWORD = 	"bm:password";
const char* const BmSmtpAccount::MSG_SMTP_SERVER = "bm:smtpserver";
const char* const BmSmtpAccount::MSG_DOMAIN = 		"bm:domain";
const char* const BmSmtpAccount::MSG_AUTH_METHOD = "bm:authmethod";
const char* const BmSmtpAccount::MSG_PORT_NR = 		"bm:portnr";
const char* const BmSmtpAccount::MSG_ACC_FOR_SAP = "bm:accForSmtpAfterPop";
const char* const BmSmtpAccount::MSG_STORE_PWD = 	"bm:storepwd";
const int16 BmSmtpAccount::nArchiveVersion = 4;

const char* const BmSmtpAccount::AUTH_SMTP_AFTER_POP= "SMTP-AFTER-POP";
const char* const BmSmtpAccount::AUTH_PLAIN = 			"PLAIN";
const char* const BmSmtpAccount::AUTH_LOGIN = 			"LOGIN";

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
	mAuthMethod.ToUpper( );
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << mPortNr;
	mPwdStoredOnDisk = FindMsgBool( archive, MSG_STORE_PWD);
	if (version > 1) {
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
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_SMTP_SERVER, mSMTPServer.String())
		||	archive->AddString( MSG_DOMAIN, mDomainToAnnounce.String())
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddString( MSG_ACC_FOR_SAP, mAccForSmtpAfterPop.String());
	return ret;
}

/*------------------------------------------------------------------------------*\
	SMTPAddress()
		-	returns the SMTP-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::GetSMTPAddress( BNetAddress* addr) const {
	if (addr)
		return addr->SetTo( mSMTPServer.String(), mPortNr) == B_OK;
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	NeedsAuthViaPopServer()
		-	determines if this SMTP-account requires authentication through a
			corresponding POP-server
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::NeedsAuthViaPopServer() {
	return mAuthMethod.ICompare(AUTH_SMTP_AFTER_POP) == 0;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmSmtpAccount::SanityCheck( BmString& complaint, BmString& fieldName) const {
	if (!mSMTPServer.Length()) {
		complaint = "Please enter the address of this account's SMTP-Server.";
		fieldName = "smtpserver";
		return false;
	}
	if ((mAuthMethod==AUTH_PLAIN || mAuthMethod==AUTH_LOGIN) && !mUsername.Length()) {
		complaint = "Please enter a username to use during authentication.";
		fieldName = "username";
		return false;
	}
	if (mPortNr<=0) {
		complaint = "Please enter a valid port-nr (1-65535) for this account.";
		fieldName = "portnr";
		return false;
	}
	return true;
}



/********************************************************************************\
	BmSmtpAccountList
\********************************************************************************/

BmRef< BmSmtpAccountList> BmSmtpAccountList::theInstance( NULL);

const int16 BmSmtpAccountList::nArchiveVersion = 1;

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
	NeedControllersToContinue( false);
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
const BmString BmSmtpAccountList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/" << "Smtp Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	fetches SMTP-accounts from given message-archive
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BmString("Start of InstantiateItems() for SmtpAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find smtp-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmSmtpAccount* newAcc = new BmSmtpAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BmString("SmtpAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BmString("End of InstantiateMailRefs() for SmtpAccountList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	SendQueuedMailFor( accName)
		-	sends all queued mails for the account specified by accName
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::SendQueuedMailFor( const BmString accName) {
	BMessage archive(BM_JOBWIN_SMTP);
	archive.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	mJobMetaController->PostMessage( &archive);
}
