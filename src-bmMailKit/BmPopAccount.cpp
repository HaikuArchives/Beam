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
		-	c'tor
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
		-	c'tor
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
		-	d'tor
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
BmString BmPopAccount::GetDomainName() const {
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
BmString BmPopAccount::GetFromAddress() const {
	BmString addr( mRealName);
	BmString domainPart = GetDomainName();
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
bool BmPopAccount::HandlesAddress( BmString addr, bool needExactMatch) const {
	Regexx rx;
	if (addr==GetFromAddress() || addr==mMailAddr || addr==mUsername)
		return true;
	int32 atPos = addr.FindFirst("@");
	if (atPos != B_ERROR) {
		BmString addrDomain( addr.String()+atPos+1);
		if (addrDomain != GetDomainName())
			return false;						// address is from different domain
		if (addr == mUsername+"@"+addrDomain)
			return true;
		addr.Truncate( atPos);
	}
	if (!needExactMatch && mMarkedAsBitBucket)
		return true;
	BmString regex = BmString("\\b") + addr + "\\b";
	return rx.exec( mMailAliases, regex) > 0;
}

/*------------------------------------------------------------------------------*\
	IsUIDDownloaded( uid)
		-	checks if a mail with the given uid (unique-ID) has already been 
			downloaded
\*------------------------------------------------------------------------------*/
bool BmPopAccount::IsUIDDownloaded( BmString uid) {
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
	MarkUIDAsDownloaded( uid)
		-	marks the given uid as downloaded
		-	this method should be called directly after a message has succesfully
			been stored locally
\*------------------------------------------------------------------------------*/
void BmPopAccount::MarkUIDAsDownloaded( BmString uid) {
	mUIDs.push_back( uid << " " << system_time());
}

/*------------------------------------------------------------------------------*\
	CheckInterval( interval)
		-	sets the regular check interval to the given interval (in minutes)
		-	initializes the interval-runner accordingly
\*------------------------------------------------------------------------------*/
void BmPopAccount::CheckInterval( int16 i) { 
	mCheckInterval = i; 
	mCheckIntervalString = i ? BmString()<<i : "";
	TellModelItemUpdated( UPD_ALL);
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	SetupIntervalRunner()
		-	sets up a BMessageRunner in such a way that it triggers when this
			account should be checked next time (indefinitely)
\*------------------------------------------------------------------------------*/
void BmPopAccount::SetupIntervalRunner() {
	delete mIntervalRunner;
	mIntervalRunner = NULL;
	BM_LOG( BM_LogPop, BmString("PopAccount.") << Key() << " sets check interval to " << mCheckInterval);
	if (mCheckInterval>0) {
		BMessage* msg = new BMessage( BMM_CHECK_MAIL);
		msg->AddString( BmPopAccountList::MSG_ITEMKEY, Key().String());
		msg->AddBool( BmPopAccountList::MSG_AUTOCHECK, true);
		mIntervalRunner = new BMessageRunner( be_app_messenger, msg, 
														  mCheckInterval*60*1000*1000, -1);
		if (mIntervalRunner->InitCheck() != B_OK)
			ShowAlert( BmString("Could not initialize check-interval runner for PopAccount ")<<Key());
	}
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmPopAccount::SanityCheck( BmString& complaint, BmString& fieldName) const {
	if (!mUsername.Length()) {
		complaint = "Please enter a username for this account.";
		fieldName = "username";
		return false;
	}
	if (!mPOPServer.Length()) {
		complaint = "Please enter the address of this account's POP-Server.";
		fieldName = "popserver";
		return false;
	}
	if (mPortNr<=0) {
		complaint = "Please enter a valid port-nr (1-65535) for this account.";
		fieldName = "portnr";
		return false;
	}
	if (mCheckInterval<0) {
		complaint = "Please enter a positive checking interval.";
		fieldName = "checkinterval";
		return false;
	}
	if (!mAuthMethod.Length()) {
		complaint = "Please select an authentication method for this account.";
		fieldName = "authmethod";
		return false;
	}
	if (mCheckInterval>0 && !mPwdStoredOnDisk) {
		complaint = "In order to check this account for mail automatically,\nthe password needs to be stored on disk.";
		fieldName = "pwdstoredondisk";
		return false;
	}
	return true;
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
		-	returns the name of the settins-file for the POP3-accounts-list
\*------------------------------------------------------------------------------*/
const BmString BmPopAccountList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/" << "Pop Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the POP3-accounts info from the given message
\*------------------------------------------------------------------------------*/
void BmPopAccountList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BmString("Start of InstantiateItems() for PopAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find pop-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmPopAccount* newAcc = new BmPopAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, BmString("PopAccount <") << newAcc->Name() << "," << newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, BmString("End of InstantiateItems() for PopAccountList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets the POP3-accounts to last saved state
		-	the list of downloaded messages is *not* reset, since resetting it
			might cause Beam to download recent messages again.
\*------------------------------------------------------------------------------*/
void BmPopAccountList::ResetToSaved() {
	BM_LOG2( BM_LogMailTracking, BmString("Start of ResetToSaved() for PopAccountList"));
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
	// first we copy all uid-lists into a temp map...
	map<BmString, vector<BmString> > uidListMap;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		uidListMap[acc->Key()] = acc->mUIDs;
	}
	// ...now we reset to saved state...
	Cleanup();
	StartJobInThisThread();
	// ...finally we update the uid-list of each account:
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		vector< BmString>& uidList = uidListMap[acc->Key()];
		if (!uidList.empty())
			acc->mUIDs = uidList;
	}
	BM_LOG2( BM_LogMailTracking, BmString("End of ResetToSaved() for PopAccountList"));
}

/*------------------------------------------------------------------------------*\
	DefaultAccount()
		-	returns the POP3-account that has been marked as default account
		-	if no account has been marked as default, the first account is returned
		-	if no POP3-account has been defined yet, NULL is returned
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmPopAccountList::DefaultAccount() {
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
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
	SetDefaultAccount( accName)
		-	marks the account with the given name as the default account
		-	any prior default-account is being reset
\*------------------------------------------------------------------------------*/
void BmPopAccountList::SetDefaultAccount( BmString accName) {
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
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
	FindAccountForAddress( addr)
		-	determines to which account the given address belongs (if any)
\*------------------------------------------------------------------------------*/
BmRef<BmPopAccount> BmPopAccountList::FindAccountForAddress( const BmString addr) {
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	// we first check whether any account handles the given address (as primary
	// address or as alias):
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
			// check if the bit-bucket is for the given address's domain
			BmString regex = acc->GetDomainName()<<"$";
			Regexx rx;
			if (rx.exec( addr, regex))
				return acc;
		}
	}
	// nothing found !?!
	return NULL;
}

/*------------------------------------------------------------------------------*\
	CheckMail( allAccounts)
		-	checks mail for the accounts that have been marked to be part of the
			primary account-set
		-	if param allAccounts is set, all accounts will be checked
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMail( bool allAccounts) {
	BmAutolock lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (allAccounts || acc->CheckMail()) {
			CheckMailFor( acc->Name());
		}
	}
}

/*------------------------------------------------------------------------------*\
	CheckMailFor( accName, isAutoCheck)
		-	checks mail for the given account
		-	param isAutoCheck indicates whether or not this check has been triggered
			by regular interval checking (in that case we do not want to show a GUI
			for the check).
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMailFor( BmString accName, bool isAutoCheck) {
	BMessage archive(BM_JOBWIN_POP);
	archive.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	archive.AddBool( MSG_AUTOCHECK, isAutoCheck);
	mJobMetaController->PostMessage( &archive);
}
