/*
	BmPopAccount.cpp

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


#include <Application.h>
#include <ByteOrder.h>
#include <File.h>
#include <Message.h>
#include <MessageRunner.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPopAccount
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmPopAccount()
		-	
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( const char* name, BmPopAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mCheckMail( true)
	,	mDeleteMailFromServer( false)
	,	mPortNr( 110)
	,	mPortNrString( "110")
	,	mAuthMethod( "POP3")
	,	mMarkedAsDefault( false)
	,	mMarkedAsBitBucket( false)
	,	mPwdStoredOnDisk( false)
	,	mCheckInterval( 0)
	,	mCheckIntervalString( "")
	,	mIntervalRunner( NULL)
{
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	BmPopAccount( archive)
		-	constructs a BmPopAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( BMessage* archive, BmPopAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
	,	mIntervalRunner( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mPOPServer = FindMsgString( archive, MSG_POP_SERVER);
	mSMTPAccount = FindMsgString( archive, MSG_SMTP_ACCOUNT);
	mRealName = FindMsgString( archive, MSG_REAL_NAME);
	mMailAddr = FindMsgString( archive, MSG_MAIL_ADDR);
	mMailAliases = FindMsgString( archive, MSG_MAIL_ALIASES);
	mSignatureName = FindMsgString( archive, MSG_SIGNATURE_NAME);
	mCheckMail = FindMsgBool( archive, MSG_CHECK_MAIL);
	mDeleteMailFromServer = FindMsgBool( archive, MSG_DELETE_MAIL);
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << mPortNr;
	mAuthMethod = FindMsgString( archive, MSG_AUTH_METHOD);
	mAuthMethod.ToUpper();
	mMarkedAsDefault = FindMsgBool( archive, MSG_MARK_DEFAULT);
	mPwdStoredOnDisk = FindMsgBool( archive, MSG_STORE_PWD);
	mMarkedAsBitBucket = FindMsgBool( archive, MSG_MARK_BUCKET);
	const char* uid;
	for( int32 i=0; archive->FindString( MSG_UID, i, &uid) == B_OK; ++i) {
		mUIDs.push_back( uid);
	}
	if (version > 1) {
		mCheckInterval = FindMsgInt16( archive, MSG_CHECK_INTERVAL);
		if (mCheckInterval)
			mCheckIntervalString << mCheckInterval;
	} else {
		mCheckInterval = 0;
	}
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	~BmPopAccount()
		-	
\*------------------------------------------------------------------------------*/
BmPopAccount::~BmPopAccount() {
	delete mIntervalRunner;
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPopAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPopAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_POP_SERVER, mPOPServer.String())
		||	archive->AddString( MSG_SMTP_ACCOUNT, mSMTPAccount.String())
		||	archive->AddString( MSG_REAL_NAME, mRealName.String())
		||	archive->AddString( MSG_MAIL_ADDR, mMailAddr.String())
		||	archive->AddString( MSG_MAIL_ALIASES, mMailAliases.String())
		||	archive->AddString( MSG_SIGNATURE_NAME, mSignatureName.String())
		||	archive->AddBool( MSG_CHECK_MAIL, mCheckMail)
		||	archive->AddBool( MSG_DELETE_MAIL, mDeleteMailFromServer)
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddBool( MSG_MARK_DEFAULT, mMarkedAsDefault)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddBool( MSG_MARK_BUCKET, mMarkedAsBitBucket)
		||	archive->AddInt16( MSG_CHECK_INTERVAL, mCheckInterval));
	int32 count = mUIDs.size();
	for( int i=0; ret==B_OK && i<count; ++i) {
		ret = archive->AddString( MSG_UID, mUIDs[i].String());
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	GetPOPAddress()
		-	returns the POP3-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
bool BmPopAccount::GetPOPAddress( BNetAddress* addr) const {
	return addr->SetTo( mPOPServer.String(), mPortNr) == B_OK;
}

/*------------------------------------------------------------------------------*\
	GetDomainName()
		-	returns the domain of this POP-Account
\*------------------------------------------------------------------------------*/
BString BmPopAccount::GetDomainName() const {
	int32 dotPos = mPOPServer.FindFirst(".");
	if (dotPos != B_ERROR)
		return mPOPServer.String()+dotPos+1;
	else
		return "";
}

/*------------------------------------------------------------------------------*\
	GetFromAddress()
		-	returns the constructed from - address for this account
\*------------------------------------------------------------------------------*/
BString BmPopAccount::GetFromAddress() const {
	BString addr( mRealName);
	BString domainPart = GetDomainName();
	if (domainPart.Length())
		domainPart.Prepend( "@");
	if (addr.Length()) {
		if (mMailAddr.Length())
			addr << " <" << mMailAddr << ">";
		else
			addr << " <" << mUsername << domainPart << ">";
	} else {
		if (mMailAddr.Length())
			addr << mMailAddr;
		else
			addr << mUsername << domainPart;
	}
	return addr;
}

/*------------------------------------------------------------------------------*\
	HandlesAddress()
		-	determines if the given address belongs to this POP-account
\*------------------------------------------------------------------------------*/
bool BmPopAccount::HandlesAddress( BString addr, bool needExactMatch) const {
	Regexx rx;
	if (addr==GetFromAddress() || addr==mMailAddr || addr==mUsername)
		return true;
	int32 atPos = addr.FindFirst("@");
	if (atPos != B_ERROR) {
		BString addrDomain( addr.String()+atPos+1);
		if (addrDomain != GetDomainName())
			return false;						// address is from different domain
		if (addr == mUsername+"@"+addrDomain)
			return true;
		addr.Truncate( atPos);
	}
	if (!needExactMatch && mMarkedAsBitBucket)
		return true;
	BString regex = BString("\\b") + addr + "\\b";
	return rx.exec( mMailAliases, regex) > 0;
}

/*------------------------------------------------------------------------------*\
	IsUIDDownloaded()
		-	
\*------------------------------------------------------------------------------*/
bool BmPopAccount::IsUIDDownloaded( BString uid) {
	uid << " ";									// append a space to avoid matching only in parts
	int32 uidLen = uid.Length();
	int32 count = mUIDs.size();
	for( int32 i=count-1; i>=0; --i) {
		if (!mUIDs[i].Compare( uid, uidLen))
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	MarkUIDAsDownloaded()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccount::MarkUIDAsDownloaded( BString uid) {
	mUIDs.push_back( uid << " " << system_time());
}

/*------------------------------------------------------------------------------*\
	CheckInterval( interval)
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccount::CheckInterval( int16 i) { 
	mCheckInterval = i; 
	mCheckIntervalString = i ? BString()<<i : "";
	TellModelItemUpdated( UPD_ALL);
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	SetupIntervalRunner()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccount::SetupIntervalRunner() {
	delete mIntervalRunner;
	mIntervalRunner = NULL;
	if (mCheckInterval>0) {
		BMessage* msg = new BMessage( BMM_CHECK_MAIL);
		msg->AddString( BmPopAccountList::MSG_ITEMKEY, Key().String());
		msg->AddBool( BmPopAccountList::MSG_AUTOCHECK, true);
		mIntervalRunner = new BMessageRunner( be_app_messenger, msg, 
														  mCheckInterval*60*1000*1000, -1);
	}
}



/********************************************************************************\
	BmPopAccountList
\********************************************************************************/


BmRef< BmPopAccountList> BmPopAccountList::theInstance( NULL);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmPopAccountList* BmPopAccountList::CreateInstance( BLooper* jobMetaController) {
	if (!theInstance) {
		theInstance = new BmPopAccountList( jobMetaController);
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmPopAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmPopAccountList::BmPopAccountList( BLooper* jobMetaController)
	:	inherited( "PopAccountList") 
	,	mJobMetaController( jobMetaController)
{
}

/*------------------------------------------------------------------------------*\
	~BmPopAccountList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPopAccountList::~BmPopAccountList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BString BmPopAccountList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Pop Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BString("Start of InstantiateItems() for PopAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find pop-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmPopAccount* newAcc = new BmPopAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BString("PopAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InstantiateItems() for PopAccountList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	DefaultAccount()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmPopAccountList::DefaultAccount() {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (acc->MarkedAsDefault()) {
			return acc;
		}
	}
	if (size() >= 1)
		return dynamic_cast< BmPopAccount*>( begin()->second.Get());
	else
		return NULL;
}

/*------------------------------------------------------------------------------*\
	DefaultAccount()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::SetDefaultAccount( BString accName) {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (acc->Key() == accName) {
			acc->MarkedAsDefault( true);
		} else if (acc->MarkedAsDefault()) {
			acc->MarkedAsDefault( false);
		}
	}
}

/*------------------------------------------------------------------------------*\
	FindAccountForAddress()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmPopAccountList::FindAccountForAddress( const BString addr) {
	BmModelItemMap::const_iterator iter;
	// we first check whether any account handles the given address as alias:
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (acc->HandlesAddress( addr, true)) {
			return acc;
		}
	}
	// may we have a bit-bucket account (fallback for failed delivery):
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (acc->MarkedAsBitBucket()) {
			BString regex = acc->GetDomainName()<<"$";
			Regexx rx;
			if (rx.exec( addr, regex))
				return acc;
		}
	}
	// nothing found !?!
	return NULL;
}


/*------------------------------------------------------------------------------*\
	CheckMail()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMail( bool allAccounts) {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (allAccounts || acc->CheckMail()) {
			CheckMailFor( acc->Name());
		}
	}
}

/*------------------------------------------------------------------------------*\
	CheckMailFor()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMailFor( BString accName, bool isAutoCheck) {
	BMessage archive(BM_JOBWIN_POP);
	archive.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	archive.AddBool( MSG_AUTOCHECK, isAutoCheck);
	mJobMetaController->PostMessage( &archive);
}

/*------------------------------------------------------------------------------*\
	AuthOnlyFor()
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccountList::AuthOnlyFor( BString accName) {
	BMessage msg(BM_JOBWIN_POP);
	msg.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	msg.AddInt32( BmJobModel::MSG_JOB_SPEC, BmPopper::BM_AUTH_ONLY_JOB);
	mJobMetaController->PostMessage( &msg);
}
