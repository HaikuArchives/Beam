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

#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif
#include <NetAddress.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmPopAccount
\********************************************************************************/

const char* const BmPopAccount::MSG_NAME = 			"bm:name";
const char* const BmPopAccount::MSG_USERNAME = 		"bm:username";
const char* const BmPopAccount::MSG_PASSWORD = 		"bm:password";
const char* const BmPopAccount::MSG_POP_SERVER = 	"bm:popserver";
const char* const BmPopAccount::MSG_CHECK_MAIL = 	"bm:checkmail";
const char* const BmPopAccount::MSG_DELETE_MAIL = 	"bm:deletemail";
const char* const BmPopAccount::MSG_DELETE_DELAY = "bm:deletedelay";
const char* const BmPopAccount::MSG_PORT_NR = 		"bm:portnr";
const char* const BmPopAccount::MSG_UID = 			"bm:uid";
const char* const BmPopAccount::MSG_UID_TIME = 		"bm:uidtm";
const char* const BmPopAccount::MSG_AUTH_METHOD = 	"bm:authmethod";
const char* const BmPopAccount::MSG_MARK_DEFAULT = "bm:markdefault";
const char* const BmPopAccount::MSG_STORE_PWD = 	"bm:storepwd";
const char* const BmPopAccount::MSG_CHECK_INTERVAL = "bm:checkinterval";
const char* const BmPopAccount::MSG_FILTER_CHAIN = "bm:filterch";
const char* const BmPopAccount::MSG_HOME_FOLDER =  "bm:homefold";
const int16 BmPopAccount::nArchiveVersion = 7;

const char* const BmPopAccount::AUTH_POP3 = "POP3";
const char* const BmPopAccount::AUTH_APOP = "APOP";

enum {
	BM_APPENDED_UID	= 'bmez'
							// a uid that has been appended to archive
};

/*------------------------------------------------------------------------------*\
	BmPopAccount()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( const char* name, BmPopAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mCheckMail( true)
	,	mDeleteMailFromServer( false)
	,	mDeleteMailDelay( 0)
	,	mPortNr( 110)
	,	mPortNrString( "110")
	,	mAuthMethod( "POP3")
	,	mMarkedAsDefault( false)
	,	mPwdStoredOnDisk( false)
	,	mCheckInterval( 0)
	,	mCheckIntervalString( "")
	,	mIntervalRunner( NULL)
	,	mFilterChain( BM_DefaultItemLabel)
	,	mHomeFolder( "in")
{
	SetupIntervalRunner();
}

/*------------------------------------------------------------------------------*\
	BmPopAccount( archive)
		-	c'tor
		-	constructs a BmPopAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmPopAccount::BmPopAccount( BMessage* archive, BmPopAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, 
										  (BmListModelItem*)NULL)
	,	mIntervalRunner( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mPOPServer = FindMsgString( archive, MSG_POP_SERVER);
	mCheckMail = FindMsgBool( archive, MSG_CHECK_MAIL);
	mDeleteMailFromServer = FindMsgBool( archive, MSG_DELETE_MAIL);
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << (uint32)mPortNr;
	mAuthMethod = FindMsgString( archive, MSG_AUTH_METHOD);
	mAuthMethod.ToUpper();
	mMarkedAsDefault = FindMsgBool( archive, MSG_MARK_DEFAULT);
	mPwdStoredOnDisk = FindMsgBool( archive, MSG_STORE_PWD);
	if (version > 1) {
		mCheckInterval = FindMsgInt16( archive, MSG_CHECK_INTERVAL);
		if (mCheckInterval)
			mCheckIntervalString << mCheckInterval;
	} else {
		mCheckInterval = 0;
	}
	if (version > 4)
		mFilterChain = FindMsgString( archive, MSG_FILTER_CHAIN);
	if (!mFilterChain.Length())
		mFilterChain = BM_DefaultItemLabel;
	if (version<=5) {
		// with version 6 we introduced identities, so we split some info off
		// the pop-account and create appropriate identities from it:
		BmIdentity* ident = new BmIdentity( Key().String(), 
														TheIdentityList.Get());
		ident->POPAccount( Key());
		ident->SMTPAccount( FindMsgString( archive, 
													  BmIdentity::MSG_SMTP_ACCOUNT));
		ident->RealName( FindMsgString( archive, 
												  BmIdentity::MSG_REAL_NAME));
		ident->MailAddr( FindMsgString( archive, 
												  BmIdentity::MSG_MAIL_ADDR));
		ident->MailAliases( FindMsgString( archive, 
													  BmIdentity::MSG_MAIL_ALIASES));
		ident->SignatureName( FindMsgString( archive, 
														 BmIdentity::MSG_SIGNATURE_NAME));
		ident->MarkedAsBitBucket( FindMsgBool( archive, 
															BmIdentity::MSG_MARK_BUCKET));
		TheIdentityList->AddItemToList( ident);
	}
	mHomeFolder = archive->FindString( MSG_HOME_FOLDER);
	if (!mHomeFolder.Length())
		mHomeFolder = BM_MAIL_FOLDER_IN;
	if (version <= 6) {
		// with version 7 we changed the UID-format, we convert old format:
		const char* uidStr;
		const char* pos;
		BmUidInfo uidInfo;
		for( int32 i=0; archive->FindString( MSG_UID, i, &uidStr) == B_OK; ++i) {
			pos = strchr( uidStr, ' ');
			if (pos)
				uidInfo.uid = BmString( uidStr, pos-uidStr);
			else
				uidInfo.uid = uidStr;
			uidInfo.timeDownloaded = time( NULL);
			mUIDs.push_back( uidInfo);
		}
		// initialize attributes introduced in version 7:
		mDeleteMailDelay = 0;
		mDeleteMailDelayString << mDeleteMailDelay;
	} else {
		// load new UID-format:
		const char* uidStr;
		BmUidInfo uidInfo;
		for( int32 i=0; 
			  archive->FindString( MSG_UID, i, &uidStr) == B_OK
			  && archive->FindInt32( MSG_UID_TIME, i, 
			  								 &uidInfo.timeDownloaded) == B_OK;  
			  ++i) {
			uidInfo.uid = uidStr;
			mUIDs.push_back( uidInfo);
		}
		// load attributes introduced in version 7:
		mDeleteMailDelay = archive->FindInt16( MSG_DELETE_DELAY);
		mDeleteMailDelayString << mDeleteMailDelay;
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
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_POP_SERVER, mPOPServer.String())
		||	archive->AddBool( MSG_CHECK_MAIL, mCheckMail)
		||	archive->AddBool( MSG_DELETE_MAIL, mDeleteMailFromServer)
		||	archive->AddInt16( MSG_DELETE_DELAY, mDeleteMailDelay)
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddBool( MSG_MARK_DEFAULT, mMarkedAsDefault)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddInt16( MSG_CHECK_INTERVAL, mCheckInterval)
		||	archive->AddString( MSG_FILTER_CHAIN, mFilterChain.String())
		||	archive->AddString( MSG_HOME_FOLDER, mHomeFolder.String());
	int32 count = mUIDs.size();
	for( int i=0; ret==B_OK && i<count; ++i) {
		ret = archive->AddString( MSG_UID, mUIDs[i].uid.String())
				|| archive->AddInt32( MSG_UID_TIME, mUIDs[i].timeDownloaded);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	IntegrateAppendedArchive( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmPopAccount::IntegrateAppendedArchive( BMessage* archive) {
	switch( archive->what) {
		case BM_APPENDED_UID: {
			BmUidInfo uidInfo;
			uidInfo.uid = archive->FindString( MSG_UID);
			uidInfo.timeDownloaded = archive->FindInt32( MSG_UID_TIME);
			mUIDs.push_back( uidInfo);
		}
	};
}

/*------------------------------------------------------------------------------*\
	GetPOPAddress()
		-	returns the POP3-connect-info as a BNetAddress
\*------------------------------------------------------------------------------*/
bool BmPopAccount::GetPOPAddress( BNetAddress* addr) const {
	if (addr)
		return addr->SetTo( mPOPServer.String(), mPortNr) == B_OK;
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	GetDomainName()
		-	returns the domain of this POP-Account
\*------------------------------------------------------------------------------*/
BmString BmPopAccount::GetDomainName() const {
	int32 dotPos = mPOPServer.FindFirst( ".");
	if (dotPos != B_ERROR) {
		int32 secondDotPos = mPOPServer.FindFirst( ".", dotPos+1);
		if (secondDotPos == B_ERROR)
			return mPOPServer.String();
							// address is just a domain name (no host part), we 
							// return the complete address as domain-part
		else
			return mPOPServer.String()+dotPos+1;
							// return the address without the leading domain part
	} else
		return "";
}

/*------------------------------------------------------------------------------*\
	IsUIDDownloaded( uid)
		-	checks if a mail with the given uid (unique-ID) has already been 
			downloaded
\*------------------------------------------------------------------------------*/
bool BmPopAccount::IsUIDDownloaded( const BmString& uid, 
												time_t* downloadTime) {
	int32 count = mUIDs.size();
	for( int32 i=count-1; i>=0; --i) {
		if (!mUIDs[i].uid.Compare( uid)) {
			if (downloadTime)
				*downloadTime = mUIDs[i].timeDownloaded;
			return true;
		}
	}
	if (downloadTime)
		*downloadTime = 0;
	return false;
}

/*------------------------------------------------------------------------------*\
	MarkUIDAsDownloaded( uid)
		-	marks the given uid as downloaded
		-	this method should be called directly after a message has succesfully
			been stored locally
\*------------------------------------------------------------------------------*/
void BmPopAccount::MarkUIDAsDownloaded( const BmString& uid) {
	BmUidInfo uidInfo;
	uidInfo.uid = uid;
	uidInfo.timeDownloaded = time( NULL);
	mUIDs.push_back( uidInfo);
	// append info about new downloaded UID to settings-file:
	BMessage archive( BM_APPENDED_UID);
	archive.AddString( BmListModel::MSG_ITEMKEY, Key().String());
	archive.AddString( MSG_UID, uid.String());
	archive.AddInt32( MSG_UID_TIME, uidInfo.timeDownloaded);
	ThePopAccountList->AppendArchive( &archive);
}

/*------------------------------------------------------------------------------*\
	AdjustToCurrentServerUids( serverUids)
		-	removes all UIDs unless they are contained in the given vector.
			This way we throw away old UIDs that no longer exist on server.
\*------------------------------------------------------------------------------*/
BmString BmPopAccount::AdjustToCurrentServerUids( 
														const vector<BmString>& serverUids) {
	BmString removedInfo;
	BmUidVect newUids;
	int32 lcount = mUIDs.size();
	int32 scount = serverUids.size();
	for( int32 l=0; l<lcount; ++l) {
		bool found = false;
		for( int32 s=0; s<scount; ++s) {
			if (!mUIDs[l].uid.Compare( serverUids[s])) {
				newUids.push_back( mUIDs[l]);
				found = true;
				break;
			}
		}
		if (!found) {
			removedInfo << "Removed local UID " << mUIDs[l].uid 
							<< " since it is not listed by the server anymore.\n";
		}
	}
	mUIDs = newUids;
	return removedInfo;
}

/*------------------------------------------------------------------------------*\
	CheckInterval( interval)
		-	sets the regular check interval to the given interval (in minutes)
		-	initializes the interval-runner accordingly
\*------------------------------------------------------------------------------*/
void BmPopAccount::CheckInterval( int16 i) { 
	mCheckInterval = i; 
	mCheckIntervalString = i ? BmString()<<i : BM_DEFAULT_STRING;
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
	BM_LOG( BM_LogPop, 
			  BmString("PopAccount.") << Key() << " sets check interval to " 
			  	<< mCheckInterval);
	if (mCheckInterval>0) {
		BMessage* msg = new BMessage( BMM_CHECK_MAIL);
		msg->AddString( BmPopAccountList::MSG_ITEMKEY, Key().String());
		msg->AddBool( BmPopAccountList::MSG_AUTOCHECK, true);
		mIntervalRunner = new BMessageRunner( be_app_messenger, msg, 
														  mCheckInterval*60*1000*1000, -1);
		if (mIntervalRunner->InitCheck() != B_OK)
			BM_SHOWERR( BmString("Could not initialize check-interval runner"
									   "for PopAccount ")<<Key());
	}
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmPopAccount::SanityCheck( BmString& complaint, 
										  BmString& fieldName) const {
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
		complaint = "In order to check this account for mail automatically,\n"
						"the password needs to be stored on disk.";
		fieldName = "pwdstoredondisk";
		return false;
	}
	return true;
}



/********************************************************************************\
	BmPopAccountList
\********************************************************************************/

BmRef< BmPopAccountList> BmPopAccountList::theInstance( NULL);

const char* const BmPopAccountList::MSG_AUTOCHECK = "bm:auto";
const int16 BmPopAccountList::nArchiveVersion = 2;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmPopAccountList* BmPopAccountList
::CreateInstance( BLooper* jobMetaController) {
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
	NeedControllersToContinue( false);
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
	return BmString( TheResources->SettingsPath.Path()) 
				<< "/" << "Pop Accounts";
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the POP3-accounts info from the given message
\*------------------------------------------------------------------------------*/
void BmPopAccountList::InstantiateItems( BMessage* archive) {
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	BM_LOG2( BM_LogMailTracking, 
				BmString("Start of InstantiateItems() for PopAccountList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, 
											  i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find pop-account nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmPopAccount* newAcc = new BmPopAccount( &msg, this);
		BM_LOG3( BM_LogMailTracking, 
					BmString("PopAccount <") << newAcc->Name() << "," 
						<< newAcc->Key() << "> read");
		AddItemToList( newAcc);
	}
	BM_LOG2( BM_LogMailTracking, 
				BmString("End of InstantiateItems() for PopAccountList"));
	mInitCheck = B_OK;
	if (version<2) {
		// with version 2 we introduced identities, so we have split some info
		// off the pop-accounts and have created appropriate identities from it.
		// In order to keep this info we store both lists:
		TheIdentityList->Store();
		this->Store();
	}
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets the POP3-accounts to last saved state
		-	the list of downloaded messages is *not* reset, since resetting it
			might cause Beam to download recent messages again.
\*------------------------------------------------------------------------------*/
void BmPopAccountList::ResetToSaved() {
	BM_LOG2( BM_LogMailTracking, 
				BmString("Start of ResetToSaved() for PopAccountList"));
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	// first we copy all uid-lists into a temp map...
	map<BmString, BmUidVect > uidListMap;
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
		BmUidVect& uidList = uidListMap[acc->Key()];
		if (!uidList.empty())
			acc->mUIDs = uidList;
	}
	BM_LOG2( BM_LogMailTracking, 
				BmString("End of ResetToSaved() for PopAccountList"));
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmPopAccountList::ForeignKeyChanged( const BmString& key, 
														const BmString& oldVal, 
														const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		if (key == BmPopAccount::MSG_FILTER_CHAIN) {
			if (acc && acc->FilterChain() == oldVal)
				acc->FilterChain( newVal);
		} else if (key == BmPopAccount::MSG_HOME_FOLDER) {
			if (acc && acc->HomeFolder() == oldVal)
				acc->HomeFolder( newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
	CheckMail( allAccounts)
		-	checks mail for the accounts that have been marked to be part of the
			primary account-set
		-	if param allAccounts is set, all accounts will be checked
\*------------------------------------------------------------------------------*/
void BmPopAccountList::CheckMail( bool allAccounts) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
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
