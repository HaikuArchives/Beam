/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <ByteOrder.h>
#include <File.h>
#include <Message.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailQuery.h"
#include "BmSmtpAccount.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSmtpAccount
\********************************************************************************/

const char* const BmSmtpAccount::MSG_NAME = 			"bm:name";
const char* const BmSmtpAccount::MSG_USERNAME = 	"bm:username";
const char* const BmSmtpAccount::MSG_PASSWORD = 	"bm:password";
const char* const BmSmtpAccount::MSG_SMTP_SERVER = "bm:smtpserver";
const char* const BmSmtpAccount::MSG_DOMAIN = 		"bm:domain";
const char* const BmSmtpAccount::MSG_ENCRYPTION_TYPE = "bm:encryptionType";
const char* const BmSmtpAccount::MSG_AUTH_METHOD = "bm:authmethod";
const char* const BmSmtpAccount::MSG_PORT_NR = 		"bm:portnr";
const char* const BmSmtpAccount::MSG_ACC_FOR_SAP = "bm:accForSmtpAfterPop";
const char* const BmSmtpAccount::MSG_STORE_PWD = 	"bm:storepwd";
const char* const BmSmtpAccount::MSG_CLIENT_CERT = "bm:clientcert";
const char* const BmSmtpAccount::MSG_ACCEPTED_CERT = 	"bm:acccert";
const int16 BmSmtpAccount::nArchiveVersion = 8;

const char* const BmSmtpAccount::ENCR_AUTO = 		"<auto>";
const char* const BmSmtpAccount::ENCR_STARTTLS = 	"STARTTLS";
const char* const BmSmtpAccount::ENCR_TLS = 			"TLS";
const char* const BmSmtpAccount::ENCR_SSL = 			"SSL";

const char* const BmSmtpAccount::AUTH_AUTO = 			"<auto>";
const char* const BmSmtpAccount::AUTH_SMTP_AFTER_POP= "SMTP-AFTER-POP";
const char* const BmSmtpAccount::AUTH_PLAIN = 			"PLAIN";
const char* const BmSmtpAccount::AUTH_LOGIN = 			"LOGIN";
const char* const BmSmtpAccount::AUTH_CRAM_MD5 = 		"CRAM-MD5";
const char* const BmSmtpAccount::AUTH_DIGEST_MD5 = 	"DIGEST-MD5";
const char* const BmSmtpAccount::AUTH_NONE = 			"<none>";

const char* const BmSmtpAccount::MSG_REF = "ref";
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
	:	inherited( FindMsgString( archive, MSG_NAME), model, 
					  (BmListModelItem*)NULL)
{
	int16 version = FindMsgInt16( archive, MSG_VERSION);
	mUsername = FindMsgString( archive, MSG_USERNAME);
	mPassword = FindMsgString( archive, MSG_PASSWORD);
	mSMTPServer = FindMsgString( archive, MSG_SMTP_SERVER);
	mDomainToAnnounce = FindMsgString( archive, MSG_DOMAIN);
	mAuthMethod = FindMsgString( archive, MSG_AUTH_METHOD);
	if (!mAuthMethod.Length())
		mAuthMethod = AUTH_NONE;
	mPortNr = FindMsgInt16( archive, MSG_PORT_NR);
	mPortNrString << (uint32)mPortNr;
	mPwdStoredOnDisk = FindMsgBool( archive, MSG_STORE_PWD);
	mClientCertificate = archive->FindString( MSG_CLIENT_CERT);
	mAcceptedCertID = archive->FindString( MSG_ACCEPTED_CERT);
	if (version > 1) {
		mAccForSmtpAfterPop = FindMsgString( archive, MSG_ACC_FOR_SAP);
	}
	if (version <= 4) {
		if (mAuthMethod.Length() && mAuthMethod[0] != '<')
			mAuthMethod.ToUpper( );
	}
	if (version <= 5) {
		// with version 6 we introduce auto-detection of best authentication
		// method. This is now the default:
		mAuthMethod = AUTH_AUTO;
	}
	if (version <= 6) {
		// with version 7 we introduce auto-detection of encryption 
		// availability. This is now the default:
		mEncryptionType = ENCR_AUTO;
	} else {
		mEncryptionType = FindMsgString( archive, MSG_ENCRYPTION_TYPE);
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
		||	archive->AddString( MSG_ENCRYPTION_TYPE, mEncryptionType.String())
		||	archive->AddString( MSG_AUTH_METHOD, mAuthMethod.String())
		||	archive->AddInt16( MSG_PORT_NR, mPortNr)
		||	archive->AddBool( MSG_STORE_PWD, mPwdStoredOnDisk)
		||	archive->AddString( MSG_ACC_FOR_SAP, mAccForSmtpAfterPop.String())
		||	archive->AddString( MSG_CLIENT_CERT, mClientCertificate.String())
		||	archive->AddString( MSG_ACCEPTED_CERT, mAcceptedCertID.String());
	return ret;
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
bool BmSmtpAccount::SanityCheck( BmString& complaint, 
											BmString& fieldName) const {
	if (!mSMTPServer.Length()) {
		complaint = "Please enter the address of this account's SMTP-Server.";
		fieldName = "smtpserver";
		return false;
	}
	if ((mAuthMethod != AUTH_NONE && mAuthMethod != AUTH_SMTP_AFTER_POP) 
	&& !mUsername.Length()) {
		complaint = "Please enter a username to use during authentication.";
		fieldName = "username";
		return false;
	}
	if (mAuthMethod==AUTH_SMTP_AFTER_POP && !mAccForSmtpAfterPop.Length()) {
		complaint = "Please select a pop-account to be used for authentication.";
		fieldName = "pop-account";
		return false;
	}
	if (mPortNr<=0) {
		complaint = "Please enter a valid port-nr (1-65535) for this account.";
		fieldName = "portnr";
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	SendMail()
		-	sends the passed mail through this account
\*------------------------------------------------------------------------------*/
void BmSmtpAccount::SendMail( const entry_ref& eref) {
	BMessage archive(BM_JOBWIN_SMTP);
	archive.AddString( BmJobModel::MSG_JOB_NAME, Key().String());
	archive.AddRef( MSG_REF, &eref);
	BLooper* controller = BeamGuiRoster->JobMetaController();
	if (controller)
		controller->PostMessage( &archive);
}

/*------------------------------------------------------------------------------*\
	SendPendingMails()
		-	sends all pending mails for this account (if any)
\*------------------------------------------------------------------------------*/
void BmSmtpAccount::SendPendingMails() {
	BmMailQuery pendingQuery;
	BmString pred = "(MAIL:status = 'Pending') && (MAIL:account = '";
	pred << Key() << "')";
	pendingQuery.SetPredicate(pred);
	pendingQuery.Execute();
	// only start job if there's actually something to do:
	uint32 count = pendingQuery.mRefVect.size();
	if (count > 0) {
		BMessage archive(BM_JOBWIN_SMTP);
		archive.AddString( BmJobModel::MSG_JOB_NAME, Key().String());
		for( uint32 i=0; i<count; ++i)
			archive.AddRef( MSG_REF, &pendingQuery.mRefVect[i]);
		BLooper* controller = BeamGuiRoster->JobMetaController();
		if (controller)
			controller->PostMessage( &archive);
	}
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
BmSmtpAccountList* BmSmtpAccountList::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmSmtpAccountList();
	}
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmSmtpAccountList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmSmtpAccountList::BmSmtpAccountList()
	:	inherited( "SmtpAccountList", BM_LogMailTracking)
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
	BmString name = BmString( BeamRoster->SettingsPath()) 
							<< "/" << "Sending Accounts";
	// this file used to be called "Smtp Accounts", so we automatically
	// rename, if only the old name exists:
	BEntry entry( name.String());
	if (!entry.Exists()) {
		BmString oldName = BmString( BeamRoster->SettingsPath()) 
									<< "/" << "Smtp Accounts";
		BEntry oldEntry( oldName.String());
		oldEntry.Rename( name.String());
	}
	return name;
}

/*------------------------------------------------------------------------------*\
	InstantiateItem( archive)
		-	instantiates one SMTP-account from given message-archive
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::InstantiateItem( BMessage* archive) {
	BmSmtpAccount* newAcc = new BmSmtpAccount( archive, this);
	BM_LOG3( BM_LogMailTracking, 
				BmString("SmtpAccount <") << newAcc->Name() << "," 
					<< newAcc->Key() << "> read");
	AddItemToList( newAcc);
}

/*------------------------------------------------------------------------------*\
	SendPendingMails()
		-	sends pending mails for all accounts
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::SendPendingMails() {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmSmtpAccount* smtpAcc 
			= dynamic_cast<BmSmtpAccount*>( iter->second.Get());
		if (smtpAcc)
			smtpAcc->SendPendingMails();
	}
}

/*------------------------------------------------------------------------------*\
	SendPendingMailsFor( accName)
		-	sends all pending mails for the account specified by accName
\*------------------------------------------------------------------------------*/
void BmSmtpAccountList::SendPendingMailsFor( const BmString accName) {
	BmRef<BmListModelItem> itemRef = FindItemByKey( accName);
	BmSmtpAccount* smtpAcc = dynamic_cast<BmSmtpAccount*>( itemRef.Get());
	if (smtpAcc)
		smtpAcc->SendPendingMails();
}
