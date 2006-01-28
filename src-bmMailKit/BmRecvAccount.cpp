/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Application.h>
#include <ByteOrder.h>
#include <File.h>
#include <Message.h>
#include <MessageRunner.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmIdentity.h"
#include "BmImapAccount.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmRecvAccount.h"
#include "BmPopAccount.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmRecvAccount
\********************************************************************************/

const char* const BmRecvAccount::MSG_NAME = 				"bm:name";
const char* const BmRecvAccount::MSG_USERNAME = 		"bm:username";
const char* const BmRecvAccount::MSG_PASSWORD = 		"bm:password";
const char* const BmRecvAccount::MSG_SERVER = 			"bm:server";
const char* const BmRecvAccount::MSG_CHECK_MAIL = 		"bm:checkmail";
const char* const BmRecvAccount::MSG_DELETE_MAIL = 	"bm:deletemail";
const char* const BmRecvAccount::MSG_DELETE_DELAY = 	"bm:deletedelay";
const char* const BmRecvAccount::MSG_PORT_NR = 			"bm:portnr";
const char* const BmRecvAccount::MSG_UID = 				"bm:uid";
const char* const BmRecvAccount::MSG_UID_TIME = 		"bm:uidtm";
const char* const BmRecvAccount::MSG_ENCRYPTION_TYPE = "bm:encryptionType";
const char* const BmRecvAccount::MSG_AUTH_METHOD = 	"bm:authmethod";
const char* const BmRecvAccount::MSG_MARK_DEFAULT = 	"bm:markdefault";
const char* const BmRecvAccount::MSG_STORE_PWD = 		"bm:storepwd";
const char* const BmRecvAccount::MSG_CHECK_INTERVAL = "bm:checkinterval";
const char* const BmRecvAccount::MSG_FILTER_CHAIN = 	"bm:filterch";
const char* const BmRecvAccount::MSG_HOME_FOLDER =  	"bm:homefold";
const char* const BmRecvAccount::MSG_TYPE = 				"bm:type";
const int16 BmRecvAccount::nArchiveVersion = 11;

const char* const BmRecvAccount::ENCR_AUTO = 		"<auto>";
const char* const BmRecvAccount::ENCR_STARTTLS =	"STARTTLS";
const char* const BmRecvAccount::ENCR_TLS = 			"TLS";
const char* const BmRecvAccount::ENCR_SSL = 			"SSL";

const char* const BmRecvAccount::AUTH_AUTO = 		"<auto>";
const char* const BmRecvAccount::AUTH_CRAM_MD5 = 	"CRAM-MD5";
const char* const BmRecvAccount::AUTH_DIGEST_MD5 = "DIGEST-MD5";

enum {
	BM_APPEND_UID	= 'bmez',
		// a uid that has been downloaded from server
	BM_REMOVE_UID	= 'bmey'
		// a uid that is no longer listed on server
};

/*------------------------------------------------------------------------------*\
	BmRecvAccount()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmRecvAccount::BmRecvAccount( const char* name, BmRecvAccountList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mCheckMail( true)
	,	mDeleteMailFromServer( false)
	,	mDeleteMailDelay( 0)
	,	mPortNr( 0)
	,	mPortNrString( "")
	,	mEncryptionType( "")
	,	mAuthMethod( AUTH_AUTO)
	,	mMarkedAsDefault( false)
	,	mPwdStoredOnDisk( false)
	,	mCheckInterval( 0)
	,	mCheckIntervalString( "")
	,	mIntervalRunner( NULL)
	,	mFilterChain( BM_DefaultItemLabel)
	,	mHomeFolder( "in")
{
}

/*------------------------------------------------------------------------------*\
	BmRecvAccount( archive)
		-	c'tor
		-	constructs a BmRecvAccount from a BMessage
\*------------------------------------------------------------------------------*/
BmRecvAccount::BmRecvAccount( BMessage* archive, BmRecvAccountList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, 
										  (BmListModelItem*)NULL)
	,	mIntervalRunner( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mCheckMail = FindMsgBool( archive, MSG_CHECK_MAIL);
	mDeleteMailFromServer = FindMsgBool( archive, MSG_DELETE_MAIL);
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << (uint32)mPortNr;
	mAuthMethod = FindMsgString( archive, MSG_AUTH_METHOD);
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
		ident->RecvAccount( Key());
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
		mHomeFolder = BmMailFolder::IN_FOLDER_NAME;
	if (version <= 6) {
		// with version 7 we changed the UID-format, we convert old format:
		const char* uidStr;
		const char* pos;
		BmString uid;
		for( int32 i=0; archive->FindString( MSG_UID, i, &uidStr) == B_OK; ++i) {
			pos = strchr( uidStr, ' ');
			if (pos)
				uid = BmString( uidStr, pos-uidStr);
			else
				uid = uidStr;
			mUIDs[uid] = time( NULL);
		}
		// initialize attributes introduced in version 7:
		mDeleteMailDelay = 0;
		mDeleteMailDelayString << mDeleteMailDelay;
	} else {
		// load new UID-format:
		const char* uidStr;
		int32 timeDownloaded;
		for( int32 i=0; 
			  archive->FindString( MSG_UID, i, &uidStr) == B_OK
			  && archive->FindInt32( MSG_UID_TIME, i, &timeDownloaded) == B_OK;
			  ++i) {
			mUIDs[uidStr] = timeDownloaded;
		}
		// load attributes introduced in version 7:
		mDeleteMailDelay = archive->FindInt16( MSG_DELETE_DELAY);
		mDeleteMailDelayString << mDeleteMailDelay;
	}
	if (version <= 7) {
		if (mAuthMethod[0] != '<')
			mAuthMethod.ToUpper();
	}
	if (version <= 8) {
		// with version 9 we introduce auto-detection of best authentication
		// method. This is now the default:
		mAuthMethod = AUTH_AUTO;
	}
	if (version <= 9) {
		// with version 10 we introduce auto-detection of encryption 
		// availability. This is now the default:
		mEncryptionType = ENCR_AUTO;
	} else {
		mEncryptionType = FindMsgString( archive, MSG_ENCRYPTION_TYPE);
	}
	if (version <= 10) {
		// with version 11 we renamed "bm:popserver" to "bm:server":
		mServer = FindMsgString( archive, "bm:popserver");
	} else
		mServer = FindMsgString( archive, MSG_SERVER);
}

/*------------------------------------------------------------------------------*\
	~BmRecvAccount()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmRecvAccount::~BmRecvAccount() {
	delete mIntervalRunner;
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmRecvAccount into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmRecvAccount::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_USERNAME, mUsername.String())
		||	archive->AddString( MSG_PASSWORD, mPassword.String())
		||	archive->AddString( MSG_SERVER, mServer.String())
		||	archive->AddBool( MSG_CHECK_MAIL, mCheckMail)
		||	archive->AddBool( MSG_DELETE_MAIL, mDeleteMailFromServer)
		||	archive->AddInt16( MSG_DELETE_DELAY, mDeleteMailDelay)
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddString( MSG_ENCRYPTION_TYPE, mEncryptionType.String())
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddBool( MSG_MARK_DEFAULT, mMarkedAsDefault)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddInt16( MSG_CHECK_INTERVAL, mCheckInterval)
		||	archive->AddString( MSG_FILTER_CHAIN, mFilterChain.String())
		||	archive->AddString( MSG_HOME_FOLDER, mHomeFolder.String())
		||	archive->AddString( MSG_TYPE, Type());
	int32 i=0;
	BmUidMap::const_iterator iter;
	for( iter = mUIDs.begin(); ret==B_OK && iter!=mUIDs.end(); ++iter, ++i) {
		ret = archive->AddString( MSG_UID, iter->first.String())
				|| archive->AddInt32( MSG_UID_TIME, iter->second);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	ExecuteAction( action)
		-	
\*------------------------------------------------------------------------------*/
void BmRecvAccount::ExecuteAction( BMessage* action) {
	BmString uid;
	switch( action->what) {
		case BM_APPEND_UID: {
			uid = action->FindString( MSG_UID);
			mUIDs[uid] = action->FindInt32( MSG_UID_TIME);
			break;
		}
		case BM_REMOVE_UID: {
			uid = action->FindString( MSG_UID);
			mUIDs.erase( uid);
			break;
		}
	};
}

/*------------------------------------------------------------------------------*\
	AddressInfo()
		-	returns the connect-info
\*------------------------------------------------------------------------------*/
void BmRecvAccount::AddressInfo( BmString& server, uint16& port) const {
	server = mServer;
	port = mPortNr;
}

/*------------------------------------------------------------------------------*\
	GetDomainName()
		-	returns the domain of this Account
\*------------------------------------------------------------------------------*/
BmString BmRecvAccount::GetDomainName() const {
	int32 dotPos = mServer.FindFirst( ".");
	if (dotPos != B_ERROR) {
		int32 secondDotPos = mServer.FindFirst( ".", dotPos+1);
		if (secondDotPos == B_ERROR)
			return mServer.String();
							// address is just a domain name (no host part), we 
							// return the complete address as domain-part
		else
			return mServer.String()+dotPos+1;
							// return the address without the leading domain part
	} else
		return "";
}

/*------------------------------------------------------------------------------*\
	IsUIDDownloaded( uid)
		-	checks if a mail with the given uid (unique-ID) has already been 
			downloaded
\*------------------------------------------------------------------------------*/
bool BmRecvAccount::IsUIDDownloaded( const BmString& uid) const {
	BmUidMap::const_iterator iter = mUIDs.find(uid);
	return iter != mUIDs.end();
}

/*------------------------------------------------------------------------------*\
	MarkUIDAsDownloaded( uid)
		-	marks the given uid as downloaded
		-	this method should be called directly after a message has succesfully
			been stored locally
\*------------------------------------------------------------------------------*/
void BmRecvAccount::MarkUIDAsDownloaded( const BmString& uid) {
	time_t timeDownloaded = time( NULL);
	// we add the uid...
	mUIDs[uid] = timeDownloaded;
	// ...and append info about new downloaded UIDs to settings-file
	// (just in order to be sure not to lose any info in case of a crash...):
	BMessage action( BM_APPEND_UID);
	action.AddString( BmListModel::MSG_ITEMKEY, Key().String());
	action.AddString( MSG_UID, uid.String());
	action.AddInt32( MSG_UID_TIME, timeDownloaded);
	TheRecvAccountList->StoreAction(&action);
}

/*------------------------------------------------------------------------------*\
	ShouldUIDBeDeletedFromServer( uid)
		-	checks if a mail with the given uid (unique-ID) should be deleted
			from the server now
\*------------------------------------------------------------------------------*/
bool BmRecvAccount::ShouldUIDBeDeletedFromServer( const BmString& uid,
																  BmString& logOutput) const
{
	if (!DeleteMailFromServer()) {
		logOutput 
			= BmString("Leaving mail with UID ") << uid
				<< " on server\n"
				<< "since user has told us to leave all mails on server.";
	} else {
		BmUidMap::const_iterator iter = mUIDs.find(uid);
		if (iter == mUIDs.end())
			// hm, UID is unknown locally, we better leave it
			return false;

		time_t timeDownloaded = iter->second;
		time_t expirationTime 
			= timeDownloaded + 60 * 60 * 24 * DeleteMailDelay();
		time_t now = time(NULL);
		if (expirationTime <= now) {
			logOutput 
				= BmString("Removing mail with UID ") << uid << " from server\n"
						<< "since it has been downloaded on "
						<< TimeToString( timeDownloaded)
						<< ",\nit's expiration time is " 
						<< TimeToString( expirationTime)
						<< "\nand now it is " << TimeToString( now);
			return true;
		} else {
			logOutput 
				= BmString("Leaving mail with UID ") << uid << " on server\n"
					<< "since it has been downloaded on "
					<< TimeToString( timeDownloaded)
					<< ",\nit's expiration time is " 
					<< TimeToString( expirationTime)
					<< "\nand now it is " << TimeToString( now);
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	AdjustToCurrentServerUids( serverUids)
		-	removes all UIDs unless they are contained in the given vector.
			This way we throw away old UIDs that no longer exist on server.
\*------------------------------------------------------------------------------*/
BmString BmRecvAccount
::AdjustToCurrentServerUids(const vector<BmString>& serverUids)
{
	BmString removedInfo;
	BmUidMap::iterator iter;
	BmUidMap::iterator curr;
	for( iter = mUIDs.begin(); iter != mUIDs.end(); ) {
		curr = iter++;
		bool found = false;
		for( uint32 s=0; s<serverUids.size(); ++s) {
			if (curr->first.Compare( serverUids[s])) {
				found = true;
				break;
			}
		}
		if (!found) {
			BmString uid = curr->first;
			removedInfo << "Removed local UID " << uid
							<< " since it is not listed by the server anymore.\n";
			// remove the UID...
			mUIDs.erase(curr);
			// ...and append info about removed UID to settings-file
			// (just in order to be sure not to lose any info in case of a crash...):
			BMessage action( BM_REMOVE_UID);
			action.AddString( BmListModel::MSG_ITEMKEY, Key().String());
			action.AddString( MSG_UID, uid.String());
			TheRecvAccountList->StoreAction(&action);
		}
	}
	return removedInfo;
}

/*------------------------------------------------------------------------------*\
	CheckInterval( interval)
		-	sets the regular check interval to the given interval (in minutes)
		-	initializes the interval-runner accordingly
\*------------------------------------------------------------------------------*/
void BmRecvAccount::CheckInterval( int16 i) { 
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
void BmRecvAccount::SetupIntervalRunner() {
	delete mIntervalRunner;
	mIntervalRunner = NULL;
	BM_LOG( BM_LogRecv, 
			  BmString("RecvAccount ") << Type() << ":" << Key() 
			  	<< " sets check interval to " << mCheckInterval);
	if (mCheckInterval>0) {
		BMessage msg( JobType());
		msg.AddString( BmRecvAccountList::MSG_ITEMKEY, Key().String());
		msg.AddBool( BmRecvAccountList::MSG_AUTOCHECK, true);
		mIntervalRunner = new BMessageRunner( 
			be_app_messenger, &msg, 
			static_cast< bigtime_t>( mCheckInterval)*60*1000*1000, 
			-1
		);
		if (mIntervalRunner->InitCheck() != B_OK)
			BM_SHOWERR( BmString("Could not initialize check-interval runner"
									   "for RecvAccount ")<<Key());
	}
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
\*------------------------------------------------------------------------------*/
bool BmRecvAccount::SanityCheck( BmString& complaint, 
											BmString& fieldName) const {
	if (!mUsername.Length()) {
		complaint = "Please enter a username for this account.";
		fieldName = "username";
		return false;
	}
	if (!mServer.Length()) {
		complaint = BmString("Please enter the address of this account's ")
							<< Type() << "-Server.";
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
	BmRecvAccountList
\********************************************************************************/

BmRef< BmRecvAccountList> BmRecvAccountList::theInstance( NULL);

const char* const BmRecvAccountList::MSG_AUTOCHECK = "bm:auto";
const int16 BmRecvAccountList::nArchiveVersion = 2;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmRecvAccountList* BmRecvAccountList
::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmRecvAccountList();
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmRecvAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmRecvAccountList::BmRecvAccountList()
	:	inherited( "RecvAccountList", BM_LogMailTracking) 
{
	mStoredActionManager.MaxCacheSize(1);
		// store all actions (downloaded UIDs) immediately
	NeedControllersToContinue( false);
}

/*------------------------------------------------------------------------------*\
	~BmRecvAccountList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmRecvAccountList::~BmRecvAccountList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settins-file for the recv-accounts-list
\*------------------------------------------------------------------------------*/
const BmString BmRecvAccountList::SettingsFileName() {
	BmString name = BmString( BeamRoster->SettingsPath()) 
							<< "/" << "Receiving Accounts";
	// this file used to be called "Pop Accounts", so we automatically
	// rename, if only the old name exists:
	BEntry entry( name.String());
	if (!entry.Exists()) {
		BmString oldName = BmString( BeamRoster->SettingsPath()) 
									<< "/" << "Pop Accounts";
		BEntry oldEntry( oldName.String());
		oldEntry.Rename( name.String());
	}
	return name;
}

/*------------------------------------------------------------------------------*\
	InstantiateItem( archive)
		-	instantiates one account from the given archive
\*------------------------------------------------------------------------------*/
void BmRecvAccountList::InstantiateItem( BMessage* archive) {
	int16 version = 0;
	archive->FindInt16( MSG_VERSION, &version);
	BmString accType;
	if (version < 11)
		accType = BmPopAccount::nType;
	else
		accType = FindMsgString( archive, BmRecvAccount::MSG_TYPE);
	BmRecvAccount* newAcc = NULL;
	if (accType == BmPopAccount::nType)
		newAcc = new BmPopAccount( archive, this);
	else
		newAcc = new BmImapAccount( archive, this);
	BM_LOG3( BM_LogMailTracking, 
				BmString("RecvAccount <") << newAcc->Type() << "," 
					<< newAcc->Name() << "," << newAcc->Key() << "> read");
	AddItemToList( newAcc);
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets the accounts to last saved state
		-	the list of downloaded messages is *not* reset, since resetting it
			might cause Beam to download recent messages again.
\*------------------------------------------------------------------------------*/
void BmRecvAccountList::ResetToSaved() {
	BM_LOG2( BM_LogMailTracking, 
				BmString("Start of ResetToSaved() for RecvAccountList"));
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	// reset to saved state
	Cleanup();
	StartJobInThisThread();
	BM_LOG2( BM_LogMailTracking, 
				BmString("End of ResetToSaved() for RecvAccountList"));
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmRecvAccountList::ForeignKeyChanged( const BmString& key, 
														const BmString& oldVal, 
														const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmRecvAccount* acc = dynamic_cast< BmRecvAccount*>( iter->second.Get());
		if (key == BmRecvAccount::MSG_FILTER_CHAIN) {
			if (acc && acc->FilterChain() == oldVal)
				acc->FilterChain( newVal);
		} else if (key == BmRecvAccount::MSG_HOME_FOLDER) {
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
void BmRecvAccountList::CheckMail( bool allAccounts) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmRecvAccount* acc = dynamic_cast< BmRecvAccount*>( iter->second.Get());
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
void BmRecvAccountList::CheckMailFor( BmString accName, bool isAutoCheck) {
	BmRef<BmListModelItem> itemRef = FindItemByKey(accName);
	BmRecvAccount* recvAcc = dynamic_cast<BmRecvAccount*>(itemRef.Get());
	if (!recvAcc)
		return;
	BMessage archive(recvAcc->JobType());
	archive.AddString( BmJobModel::MSG_JOB_NAME, accName.String());
	archive.AddBool( MSG_AUTOCHECK, isAutoCheck);
	BLooper* controller = BeamGuiRoster->JobMetaController();
	if (controller)
		controller->PostMessage( &archive);
}
