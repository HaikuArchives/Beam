/*
	BmPopper.cpp
		- Implements the main POP3-client-class: BmPopper

		$Id$
*/
/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <memory.h>
#include <memory>

#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "md5.h"

#include "regexx.hh"
#include "split.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmFilter.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmNetEndpointRoster.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

/********************************************************************************\
	BmPopStatusFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPopStatusFilter::BmPopStatusFilter( BmMemIBuf* input, BmNetJobModel* job,
												  uint32 blockSize)
	:	inherited( input, blockSize)
	,	mJob( job)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPopStatusFilter::Filter( const char* srcBuf, uint32& srcLen, 
										  char* destBuf, uint32& destLen) {
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;

	bool needData = false;
	if (mInfoMsg)
		mInfoMsg->FindBool(BmPopper::IMSG_NEED_DATA, &needData);

	if (mHaveStatus) {
		uint32 size = min( destLen, srcLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
	} else {
		while( src<srcEnd && *src!='\n')
			src++;
		uint32 statusSize = src-srcBuf;
		mStatusText.Append( srcBuf, statusSize);
		if (src<srcEnd) {
			src++;								// skip '\n'
			mHaveStatus = true;
			mStatusText.RemoveAll( "\r");
			if (!needData 
			|| mStatusText.Length() && mStatusText.ByteAt(0) != '+')
				// if the status is all we want or an error occurred,
				// we are done:
				mEndReached = true;
		}
		srcLen = src-srcBuf;
		destLen = 0;
	}
	if (mUpdate && destLen)
		mJob->UpdateProgress( destLen);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPopStatusFilter::CheckForPositiveAnswer() {
	if (mStatusText.Length() && mStatusText.ByteAt(0) != '+') {
		BmString err("Server answers: \n");
		err += mStatusText;
		err.RemoveAll( "\r");
		throw BM_network_error( err);
	}
	return true;
}



/********************************************************************************\
	BmPopper
\********************************************************************************/

// message component definitions for additional info:
const char* const BmPopper::MSG_PWD = 	"bm:pwd";

// alternate job-specifiers:
const int32 BmPopper::BM_AUTH_ONLY_JOB = 			1;
					// for authentication only (needed for SMTP-after-POP)
const int32 BmPopper::BM_CHECK_CAPABILITIES_JOB = 2;
					// to find out about supported capabilities

int32 BmPopper::mId = 0;

/*------------------------------------------------------------------------------*\
	PopStates[]
		-	array of POP3-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmPopper::PopState BmPopper::PopStates[BmPopper::POP_FINAL] = {
	PopState( "connect...", &BmPopper::StateConnect),
	PopState( "capa...", &BmPopper::StateCapa),
	PopState( "starttls...", &BmPopper::StateStartTLS),
	PopState( "auth...", &BmPopper::StateAuth),
	PopState( "check...", &BmPopper::StateCheck),
	PopState( "get...", &BmPopper::StateRetrieve),
	PopState( "quit...", &BmPopper::StateDisconnect),
	PopState( "done", NULL)
};

/*------------------------------------------------------------------------------*\
	BmPopper( info)
		-	contructor
\*------------------------------------------------------------------------------*/
BmPopper::BmPopper( const BmString& name, BmPopAccount* account)
	:	inherited( BmString("POP_")<<name, BM_LogRecv, 
					  new BmPopStatusFilter( NULL, this))
	,	mPopAccount( account)
	,	mCurrMailNr( 0)
	,	mMsgCount( 0)
	,	mNewMsgCount( 0)
	,	mNewMsgTotalSize( 1)
	,	mServerSupportsTLS(false)
	,	mState( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmPopper()
		-	destructor
\*------------------------------------------------------------------------------*/
BmPopper::~BmPopper() { 
	BM_LOG_FINISH( BM_LOGNAME);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the Popper should continue to run
		-	in addition to the inherited behaviour, the Popper should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmPopper::ShouldContinue() {
	if (mConnection && mConnection->IsStopRequested())
		return false;
	return CurrentJobSpecifier() == BM_AUTH_ONLY_JOB
			 || CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB
			 || inherited::ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	SetupAdditionalInfo()
		-	
\*------------------------------------------------------------------------------*/
void BmPopper::SetupAdditionalInfo( BMessage* additionalInfo)
{
	additionalInfo->AddString(BmNetEndpoint::MSG_CLIENT_CERT_NAME,
									  mPopAccount->ClientCertificate().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_SERVER_NAME,
									  mPopAccount->Server().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_ACCEPTED_CERT_ID,
									  mPopAccount->AcceptedCertID().String());
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all POP3-stages and calls the corresponding 
			handlers
		-	returns whether or not the Popper has completed it's job
\*------------------------------------------------------------------------------*/
bool BmPopper::StartJob() {
	for( int32 state = POP_CONNECT; state<POP_DONE; ++state)
		PopStates[state].skip = false;

	if (CurrentJobSpecifier() == BM_AUTH_ONLY_JOB) {
		// in auth-only mode, we skip after authorization:
		PopStates[POP_CHECK].skip = true;
		PopStates[POP_RETRIEVE].skip = true;
	}

	if (CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB) {
		// when checking capabilities, we skip nearly everything:
		PopStates[POP_STARTTLS].skip = true;
		PopStates[POP_AUTH].skip = true;
		PopStates[POP_CHECK].skip = true;
		PopStates[POP_RETRIEVE].skip = true;
	}

	int32 skipped = 0;
	for( int32 state = POP_CONNECT; state<POP_DONE; ++state) {
		if (PopStates[state].skip)
			skipped++;
	}
	const float delta = (100.0 / (POP_DONE-skipped));
	try {
		for( mState=POP_CONNECT; mState<POP_DONE; ++mState) {
			if (PopStates[mState].skip)
				continue;
			TStateMethod stateFunc = PopStates[mState].func;
			UpdatePOPStatus( (mState==POP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
			if (!ShouldContinue()) {
				Disconnect();
				break;
			}
		}
		if (!ShouldContinue())
			UpdatePOPStatus( 0.0, NULL, false, true);
		else
			UpdatePOPStatus( delta, NULL);
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		int e;
		if (mConnection && (e = mConnection->Error())!=B_OK)
			errstr << "\nerror: " << e << ", " << mConnection->ErrorStr();
		UpdatePOPStatus( 0.0, NULL, true);
		BmString text = Name() << ":\n\n" << errstr;
		HandleError( text);
		return false;
	}
	catch( exception &err) {
		BmString errMsg;
		errMsg << err.what() << " (" << typeid(err).name() << ")";
		HandleError( errMsg);
		return false;
	}
	catch( ...) {
		BmString errMsg;
		errMsg << "The job for account " << mPopAccount->Name() 
				 << "received an unknown exception and died!";
		HandleError(errMsg);
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdatePOPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current POP3-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BmString "FAILED!" will be shown)
\*------------------------------------------------------------------------------*/
void BmPopper::UpdatePOPStatus( const float delta, const char* detailText, 
										  bool failed, bool stopped) {
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed) {
		msg->AddString( MSG_TRAILING, 
							(BmString(PopStates[mState].text) << " FAILED!").String());
		msg->AddBool( MSG_FAILED, true);
	} else if (stopped)
		msg->AddString( MSG_TRAILING, 
							(BmString(PopStates[mState].text) 
								<< " Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, PopStates[mState].text);
	if (detailText)
		msg->AddString( MSG_LEADING, detailText);
	if (mConnection && mConnection->EncryptionIsActive())
		msg->AddBool( MSG_ENCRYPTED, true);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	UpdateMailStatus( delta, detailText)
		- informs the interested party about the message currently dealt with
\*------------------------------------------------------------------------------*/
void BmPopper::UpdateMailStatus( const float delta, const char* detailText, 
											int32 currMsg) {
	BmString text;
	if (mNewMsgCount) {
		text = BmString() << currMsg << " of " << mNewMsgCount;
	} else {
		text = "none";
	}
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "mailbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text.String());
	if (detailText)
		msg->AddString( MSG_TRAILING, detailText);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	UpdateProgress( numBytes)
		-
\*------------------------------------------------------------------------------*/
void BmPopper::UpdateProgress( uint32 numBytes) {
	float delta = (100.0*numBytes)/(mNewMsgTotalSize ? mNewMsgTotalSize : 1);
	BmString detailText = BmString("size: ") 
									<< BytesToString( mNewMsgSizes[mCurrMailNr-1]);
	UpdateMailStatus( delta, detailText.String(), mCurrMailNr);
}

/*------------------------------------------------------------------------------*\
	StateConnect()
		-	Initiates network-connection to POP-server
\*------------------------------------------------------------------------------*/
void BmPopper::StateConnect() {
	BNetAddress addr;
	if (addr.SetTo( mPopAccount->Server().String(), 
						 mPopAccount->PortNr()) != B_OK) {
		BmString s = BmString("Could not determine address of POP-Server ") 
							<< mPopAccount->Server();
		throw BM_network_error( s);
	}
	if (!Connect( &addr)) {
		BmString s = BmString("Could not connect to POP-Server ") 
							<< mPopAccount->Server() 
						  	<< "\n\bError:\n\t" << mErrorString;
		throw BM_network_error( s);
	}
	BmString encryptionType = mPopAccount->EncryptionType();
	if (TheNetEndpointRoster->SupportsEncryption()
	&& (encryptionType.ICompare(BmPopAccount::ENCR_TLS) == 0
		|| encryptionType.ICompare(BmPopAccount::ENCR_SSL) == 0)) {
		// straight TLS or SSL, we start the encryption layer: 
		if (!StartEncryption(encryptionType.String()))
			return;
	}
	// accept server greeting (either encrypted or unencrypted):
	CheckForPositiveAnswer();
	Regexx rx;
	if (rx.exec( StatusText(), "(<.+?>)\\s*$", Regexx::newline)) {
		mServerTimestamp = rx.match[0];
	}
}

/*------------------------------------------------------------------------------*\
	StateCapa()
		-	asks server for its capabilities
\*------------------------------------------------------------------------------*/
void BmPopper::StateCapa() {
	BmString cmd("CAPA");
	SendCommand( cmd);
	try {
		CheckForPositiveAnswer( 16384, true);
							// we expect a multiline string as answer
		Regexx rx;
		if (rx.exec( 
			mAnswerText, "^\\s*SASL\\s+(.*?)$", Regexx::newline
		)) {
			mSupportedAuthTypes = rx.match[0].atom[0];
		} else if (rx.exec( 
			mAnswerText, "^\\s*AUTH\\s+(.*?)$", Regexx::newline
		)) {
			mSupportedAuthTypes = rx.match[0].atom[0];
		}
		if (rx.exec( mAnswerText, "^\\s*STLS\\b", Regexx::newline))
			mServerSupportsTLS = true;
		else
			mServerSupportsTLS = false;
	} catch(...) {
	}
}

/*------------------------------------------------------------------------------*\
	StartEncryption(encryptionType)
		-	starts connection encryption
\*------------------------------------------------------------------------------*/
void BmPopper::StateStartTLS() {
	// check if encryption via STARTTLS is requested (and possible):
	BmString encryptionType = mPopAccount->EncryptionType();

	// automatic means: use STARTTLS if available:
	if (encryptionType.ICompare(BmPopAccount::ENCR_AUTO) == 0
	&& mServerSupportsTLS) {
		encryptionType = BmPopAccount::ENCR_STARTTLS;
	}
	
	if (!TheNetEndpointRoster->SupportsEncryption()
	|| encryptionType.ICompare(BmPopAccount::ENCR_STARTTLS) != 0)
		return;

	// let's try to initiate TLS...
	SendCommand( "STLS");
	CheckForPositiveAnswer();

	// start connection encryption:
	StartEncryption(BmPopAccount::ENCR_TLS);
}

/*------------------------------------------------------------------------------*\
	StartEncryption()
		-	extends activation of SSL/TLS encryption layer with automatic
			updating of newly accepted certificate ID.
\*------------------------------------------------------------------------------*/
bool BmPopper::StartEncryption(const char* encType)
{
	bool ok = inherited::StartEncryption(encType);
	if (ok) {
		BmString certID = mConnection->NewAcceptedCertID();
		if (certID.Length() && mPopAccount->AcceptedCertID() != certID) {
			mPopAccount->AcceptedCertID(certID);
			TheRecvAccountList->MarkAsStoreNeeded();
		}
	}
	return ok;
}

/*------------------------------------------------------------------------------*\
	ExtractBase64()
		-	
\*------------------------------------------------------------------------------*/
void BmPopper::ExtractBase64(const BmString& text, BmString& base64)
{
	text.CopyInto(base64, 2, text.Length());
}

/*------------------------------------------------------------------------------*\
	StateAuth()
		-	Sends user/passwd combination and checks result
		-	currently supports POP3- & APOP-authentication
\*------------------------------------------------------------------------------*/
void BmPopper::StateAuth() {
	BmString pwd;
	bool pwdOK = false;
	bool first = true;
	BmString authMethod = mPopAccount->AuthMethod();
	if (authMethod == BmPopAccount::AUTH_AUTO)
		authMethod = SuggestAuthType();
	while(!pwdOK) {
		bool pwdGiven = false;
		if (first && mPopAccount->PwdStoredOnDisk()) {
			// use stored password:
			pwd = mPopAccount->Password();
			pwdGiven = true;
		} else if (ShouldContinue()) {
			// ask user about password:
			BmString text( "Please enter password for POP-Account <");
			text << Name() << ">:";
			pwdGiven = BeamGuiRoster->AskUserForPwd( text, pwd);
		}
		if (!pwdGiven || !ShouldContinue()) {
			// user has cancelled, we stop
			Disconnect();
			StopJob();
			return;
		}
		first = false;
		if (authMethod == BmPopAccount::AUTH_APOP) {
			// APOP-method: 
			if (mServerTimestamp.Length()) {
				BmString secret( mServerTimestamp + pwd);
				BmString Digest;
				char* buf = Digest.LockBuffer(40);	// should only need 33
				MD5Digest( (unsigned char*) secret.String(), buf);
				Digest.UnlockBuffer( -1);
				BmString cmd = BmString("APOP ") << mPopAccount->Username() << " ";
				SendCommand( cmd, Digest);
			} else
				BM_THROW_RUNTIME( "Server did not supply a timestamp, "
										"so APOP doesn't work.");
		} else if (authMethod == BmPopAccount::AUTH_CRAM_MD5) {
			BmString cmd = BmString("AUTH CRAM-MD5");
			SendCommand( cmd);
			AuthCramMD5(mPopAccount->Username(), mPopAccount->Password());
		} else if (authMethod == BmPopAccount::AUTH_DIGEST_MD5) {
			BmString cmd = BmString("AUTH DIGEST-MD5");
			SendCommand( cmd);
			BmString serviceUri = BmString("pop/") << mPopAccount->Server();
			AuthDigestMD5(mPopAccount->Username(), mPopAccount->Password(),
							  serviceUri);
		} else {
			// authMethod == AUTH_POP3: send username and password as plain text:
			BmString cmd = BmString("USER ") << mPopAccount->Username();
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				SendCommand( "PASS ", pwd);
			}
		}
		try {
			if (CheckForPositiveAnswer())
				pwdOK = true;
			else {
				Disconnect();
				StopJob();
				return;
			}
		} catch( BM_network_error &err) {
			// most probably a wrong password...
			BmString errstr = err.what();
			int e;
			if (mConnection && (e = mConnection->Error())!=B_OK)
				errstr << "\nerror: " << e << ", " << mConnection->ErrorStr();
			BmString text = Name() << ":\n\n" << errstr;
			HandleError( text);
		}
	}
}

/*------------------------------------------------------------------------------*\
	StateCheck()
		-	looks for new mail
\*------------------------------------------------------------------------------*/
void BmPopper::StateCheck() {
	uint32 msgNum = 0;

	BmString cmd("STAT");
	SendCommand( cmd);
	if (!CheckForPositiveAnswer())
		return;
	Regexx rx;
	if (!rx.exec( StatusText(), "^\\+OK\\s+(\\d+)", Regexx::nocase))
		throw BM_network_error( "answer to STAT has unknown format");
	BmString numStr = rx.match[0].atom[0];
	mMsgCount = atoi(numStr.String());
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		// we remove all local UIDs, since none are listed on the server:
		BmString removedUids = mPopAccount->AdjustToCurrentServerUids( mMsgUIDs);
		BM_LOG2( BM_LogRecv, removedUids);
		return;									// no messages found, nothing more to do
	}

	// we try to fetch a list of unique message IDs from server:
	cmd = BmString("UIDL");
	SendCommand( cmd);
	try {
		// The UIDL-command may not be implemented by this server, so we 
		// do not require a positive answer, we just hope for it:
		if (!CheckForPositiveAnswer( 16384, true))
			return;								// interrupted, we give up
		// ok, we've got the UIDL-listing, so we fetch it,
		// fetch UIDLs one per line and store them in array:
		int numLines = rx.exec( mAnswerText, "\\s*(\\d+)\\s+(.+?)\\s*$", 
										Regexx::newline | Regexx::global);
		if (numLines < mMsgCount)
			throw BM_network_error(	BmString("answer to UIDL has unknown format"
														", too few lines matching"));
		for( int32 i=0; i<mMsgCount; ++i)
			mMsgUIDs.push_back( rx.match[i].atom[1]);
		// we remove local UIDs that are not listed on the server anymore:
		BmString removedUids = mPopAccount->AdjustToCurrentServerUids( mMsgUIDs);
		BM_LOG( BM_LogRecv, removedUids);
	} catch( BM_network_error& err) {
		// no UIDL-listing from server, we will have to get by without...
	}

	// compute total size of messages that are new to us:
	mNewMsgTotalSize = 0;
	mNewMsgCount = 0;
	cmd = "LIST";
	SendCommand( cmd);
	if (!CheckForPositiveAnswer( 16384, true))
		return;
	vector<BmString> listAnswerVect;
	split( "\r\n", mAnswerText, listAnswerVect);
	uint32 count = mMsgCount;
	if (count != listAnswerVect.size()) {
		BM_LOG( BM_LogRecv, 
				  BmString("Strange: server indicated ")<< mMsgCount
						<< " mails, but LIST received " << listAnswerVect.size()
						<< " lines!");
		if (count > listAnswerVect.size())
			count = listAnswerVect.size();
	}
	for( uint32 i=0; i<count; i++) {
		int32 msgSize;
		if (!mPopAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is new (according to unknown UID)
			// fetch msgsize for message...
			Regexx rx;
			if (!rx.exec( listAnswerVect[i], "^\\s*(\\d+)\\s+(\\d+)\\s*$"))
				throw BM_network_error( 
					BmString("answer to LIST has unknown format, msg ") << i+1
				);
			BmString msgNumStr = rx.match[0].atom[0];
			msgNum = atoi(msgNumStr.String());
			if (msgNum != i+1)
				throw BM_network_error( 
					BmString("answer to LIST has unexpeced msgNo in line ") << i+1
				);
			BmString msgSizeStr = rx.match[0].atom[1];
			msgSize = atoi(msgSizeStr.String());
			// add msg-size to total:
			mNewMsgTotalSize += msgSize;
			mNewMsgSizes.push_back( msgSize);
			mNewMsgCount++;
		} else {
			// msg is old (according to known UID), we may have to remove it now:
			BmString log;
			bool shouldBeRemoved 
				= mPopAccount->ShouldUIDBeDeletedFromServer(mMsgUIDs[i], log);
			BM_LOG2( BM_LogRecv, log);
			if (shouldBeRemoved) {
				cmd = BmString("DELE ") << i+1;
				SendCommand( cmd);
				if (!CheckForPositiveAnswer())
					return;
			}
		}
	}
	if (mNewMsgCount == 0)
		UpdateMailStatus( 0, NULL, 0);
}

/*------------------------------------------------------------------------------*\
	StateRetrieve()
		-	retrieves all new mails from server
\*------------------------------------------------------------------------------*/
void BmPopper::StateRetrieve() {
	BmString cmd;
	mCurrMailNr = 1;
	for( int32 i=0; mNewMsgCount>0 && i<mMsgCount; ++i) {
		if (mPopAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is old (according to known UID), we skip it:
			continue;
		}
		cmd = BmString("RETR ") << i+1;
		SendCommand( cmd);
		time_t before = time(NULL);
		if (!CheckForPositiveAnswer( mNewMsgSizes[mCurrMailNr-1], true, true))
			goto CLEAN_UP;
		if (mAnswerText.Length() > ThePrefs->GetInt("LogSpeedThreshold", 
																  100*1024)) {
			time_t after = time(NULL);
			time_t duration = after-before > 0 ? after-before : 1;
			// log speed for mails that exceed a certain size:
			BM_LOG( BM_LogRecv, 
					  BmString("Received mail of size ")<<mAnswerText.Length()
							<< " bytes in " << duration << " seconds => " 
							<< mAnswerText.Length()/duration/1024.0 << "KB/s");
		}
		if (mAnswerText.Length() != mNewMsgSizes[mCurrMailNr-1]) {
			// as this actually happens (what the heck?) we simply
			// log it if in verbose mode:
			BM_LOG2( BM_LogRecv,
						BmString("Received mail has ") << mAnswerText.Length()
							<< " bytes but it was announced to have " 
							<< mNewMsgSizes[mCurrMailNr-1] << " bytes."
			);
		}
		// now create a mail from the received data...
		BM_LOG2( BM_LogRecv, "Creating mail...");
		BmRef<BmMail> mail = new BmMail( mAnswerText, mPopAccount->Name());
		if (mail->InitCheck() != B_OK)
			goto CLEAN_UP;
		// ...set default folder according to pop-account settings...
		mail->SetDestFolderName( mPopAccount->HomeFolder());
		// ...execute mail-filters for this mail...
		BM_LOG2( BM_LogRecv, "...applying filters (in memory)...");
		mail->ApplyInboundFilters();
		// ...and store mail on disk:
		BM_LOG2( BM_LogRecv, "...storing mail...");
		if (!mail->Store())
			goto CLEAN_UP;
		BM_LOG2( BM_LogRecv, "...done");
		mPopAccount->MarkUIDAsDownloaded( mMsgUIDs[i]);
		//	delete the retrieved message if required to do so immediately:
		BmString log;
		bool shouldBeDeleted
			= mPopAccount->ShouldUIDBeDeletedFromServer(mMsgUIDs[i], log);
		BM_LOG2( BM_LogRecv, log);
		if (shouldBeDeleted) {
			cmd = BmString("DELE ") << i+1;
			SendCommand( cmd);
			if (!CheckForPositiveAnswer())
				goto CLEAN_UP;
		}
		mCurrMailNr++;
	}
	if (mNewMsgCount)
		UpdateMailStatus( 100.0, "done", mNewMsgCount);
CLEAN_UP:
	mCurrMailNr = 0;
}

/*------------------------------------------------------------------------------*\
	StateDisconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmPopper::StateDisconnect() {
	Quit( true);
}

/*------------------------------------------------------------------------------*\
	Quit( WaitForAnswer)
		-	sends a QUIT to the server, waiting for answer only 
			if WaitForAnswer==true
		-	normally, we wait for an answer, just if we are shutting down
			because of an error we ignore any answer.
		-	the network-connection is always closed
\*------------------------------------------------------------------------------*/
void BmPopper::Quit( bool WaitForAnswer) {
	BmString cmd("QUIT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer)
			GetAnswer();
	} catch(...) {	}
	Disconnect();
}

/*------------------------------------------------------------------------------*\
	SupportsTLS()
		-	returns whether or not the server has indicated that it supports 
			the STARTTLS command
\*------------------------------------------------------------------------------*/
bool BmPopper::SupportsTLS() const
{
	return mServerSupportsTLS;
}

/*------------------------------------------------------------------------------*\
	SuggestAuthType()
		-	looks at the auth-types supported by the server and selects 
			the most secure of those that is supported by Beam.
\*------------------------------------------------------------------------------*/
BmString BmPopper::SuggestAuthType() const
{
	if (mSupportedAuthTypes.IFindFirst( BmPopAccount::AUTH_DIGEST_MD5) >= 0)
		return BmPopAccount::AUTH_DIGEST_MD5;
	else if (mSupportedAuthTypes.IFindFirst( BmPopAccount::AUTH_CRAM_MD5) >= 0)
		return BmPopAccount::AUTH_CRAM_MD5;
	else if (mServerTimestamp.Length())
		return BmPopAccount::AUTH_APOP;
	else
		return BmPopAccount::AUTH_POP3;
}
