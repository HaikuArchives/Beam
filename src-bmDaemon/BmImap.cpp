/*
	BmImap.cpp
		- Implements the main IMAP-client-class: BmImap

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
#include <stdio.h>

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
#include "BmImapAccount.h"
#include "BmImap.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

/********************************************************************************\
	BmImapStatusFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmImapStatusFilter::BmImapStatusFilter( BmMemIBuf* input, BmNetJobModel* job,
												    uint32 blockSize)
	:	inherited(input, blockSize)
	,	mJob(job)
	,	mLineBuf(1000)
	,	mLiteralCharCount(0)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmImapStatusFilter::Reset( BmMemIBuf* input)
{
	inherited::Reset( input);
	mLiteralCharCount = 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmImapStatusFilter::Filter( const char* srcBuf, uint32& srcLen, 
										   char* destBuf, uint32& destLen)
{
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;

	if (!mLiteralCharCount) {
		BmString tagStr;
		if (mInfoMsg)
			tagStr = mInfoMsg->FindString(BmImap::IMSG_NEEDED_TAG);

		// setup a regex-string that can decide whether or not a given line
		// is a status line.
		// IMAP actually only defines '*' or the tag as indicator, but since
		// we are supporting DIGEST-MD5 and CRAM-MD5 and those do make use
		// of '+' (which I *do* find rather annoying!), we check for '+', too
		BmString statusRxStr 
			= tagStr.Length()
				? BmString("^(\\*|\\+|") << tagStr << ")\\s+"
				: "^(\\*|\\+)\\s+";

		Regexx rx;
		while(src<srcEnd) {
			mLineBuf << *src++;
			if (*src=='\n') {
				mLineBuf << *src++;
				// now we have a complete line in the ring buffer, we fetch it...
				mLastStatusLine = mLineBuf;
				// ...and check it's status:
				if (rx.exec( mLastStatusLine, statusRxStr)) {
					// this is a status line
					mLastStatusLine.RemoveAll("\r");
					if (mLastStatusLine.ByteAt(0) == '*'
					|| mLastStatusLine.ByteAt(0) == '+')
						// normal status (at top of answer stream)
						mStatusText << mLastStatusLine;
					else
						// tagged status (at bottom of answer stream)
						mBottomStatusText << mLastStatusLine;
					mHaveStatus = true;
					srcLen = src-srcBuf;
					destLen = 0;
					// we have reached the end if no tag is expected (the answer
					// will be one line only) or if this is the tagged line:
					if (!tagStr.Length() 
					|| mLastStatusLine.ICompare(tagStr, tagStr.Length()) == 0)
						mEndReached = true;
					// check for a literal:
					if (rx.exec(mLastStatusLine, "\\{(\\d+)\\}\\s*\\n")) {
						BmString literalLen = rx.match[0].atom[0];
						mLiteralCharCount = atoi(literalLen.String());
					}
					return;
				}
				break;
			}
		}
		srcLen = src-srcBuf;
		destLen = 0;
		return;
	}
	uint32 size = min( destLen, srcLen);
	if (mLiteralCharCount) {
		if (size >= mLiteralCharCount) {
			size = mLiteralCharCount;
			mLiteralCharCount = 0;
		} else {
			mLiteralCharCount -= size;
		}
	}
	memcpy( destBuf, srcBuf, size);
	srcLen = destLen = size;
	if (mUpdate && destLen)
		mJob->UpdateProgress( destLen);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmImapStatusFilter::CheckForPositiveAnswer()
{
	BmString tagStr;
	if (mInfoMsg)
		tagStr = mInfoMsg->FindString(BmImap::IMSG_NEEDED_TAG);
	if (mLastStatusLine.Length()) {
		// a problem is indicated by "* " or "<tag> ", followed by either
		// "BAD" or "NO":
		BmString badStatusRxStr 
			= tagStr.Length()
				? BmString("^(\\*|") << tagStr << ")\\s+(BAD|NO)\\b"
				: "^\\*\\s+(BAD|NO)\\b";
		Regexx rx;
		if (rx.exec(mLastStatusLine, badStatusRxStr)) {
			BmString err("Server answers: \n");
			err += mLastStatusLine;
			err.RemoveAll( "\r");
			throw BM_network_error( err);
		}
	}
	return true;
}



/********************************************************************************\
	BmImap
\********************************************************************************/

// message component definitions for additional info:
const char* const BmImap::MSG_PWD = 	"bm:pwd";

// alternate job-specifiers:
const int32 BmImap::BM_CHECK_CAPABILITIES_JOB = 1;
					// to find out about supported capabilities

const char* const BmImap::IMSG_NEEDED_TAG = "neededTag";

int32 BmImap::mId = 0;

/*------------------------------------------------------------------------------*\
	ImapStates[]
		-	array of IMAP-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmImap::ImapState BmImap::ImapStates[BmImap::IMAP_FINAL] = 
{
	ImapState( "connect...", &BmImap::StateConnect),
	ImapState( "capa...", &BmImap::StateCapa),
	ImapState( "starttls...", &BmImap::StateStartTLS),
	ImapState( "auth...", &BmImap::StateAuth),
	ImapState( "check...", &BmImap::StateCheck),
	ImapState( "get...", &BmImap::StateRetrieve),
	ImapState( "quit...", &BmImap::StateDisconnect),
	ImapState( "done", NULL)
};

/*------------------------------------------------------------------------------*\
	BmImap( info)
		-	contructor
\*------------------------------------------------------------------------------*/
BmImap::BmImap( const BmString& name, BmImapAccount* account)
	:	inherited( BmString("IMAP_")<<name, BM_LogRecv, 
					  new BmImapStatusFilter( NULL, this))
	,	mImapAccount( account)
	,	mCurrMailNr( 0)
	,	mMsgCount( 0)
	,	mNewMsgCount( 0)
	,	mNewMsgTotalSize( 1)
	,	mServerSupportsTLS(false)
	,	mState( 0)
	,	mTaggedMode( false)
	,	mCurrTagNr( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmImap()
		-	destructor
\*------------------------------------------------------------------------------*/
BmImap::~BmImap()
{
	BM_LOG_FINISH( BM_LOGNAME);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the Imapper should continue to run
		-	in addition to the inherited behaviour, the Imapper should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmImap::ShouldContinue()
{
	return CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB
			 || inherited::ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all IMAP-stages and calls the corresponding 
			handlers
		-	returns whether or not the Imapper has completed it's job
\*------------------------------------------------------------------------------*/
bool BmImap::StartJob()
{
	for( int32 state = IMAP_CONNECT; state<IMAP_DONE; ++state)
		ImapStates[state].skip = false;

	if (CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB) {
		// when checking capabilities, we skip nearly everything:
		ImapStates[IMAP_STARTTLS].skip = true;
		ImapStates[IMAP_AUTH].skip = true;
		ImapStates[IMAP_CHECK].skip = true;
		ImapStates[IMAP_RETRIEVE].skip = true;
	}

	int32 skipped = 0;
	for( int32 state = IMAP_CONNECT; state<IMAP_DONE; ++state) {
		if (ImapStates[state].skip)
			skipped++;
	}
	const float delta = (100.0 / (IMAP_DONE-skipped));
	try {
		for( mState=IMAP_CONNECT; ShouldContinue() && mState<IMAP_DONE; ++mState) {
			if (ImapStates[mState].skip)
				continue;
			TStateMethod stateFunc = ImapStates[mState].func;
			UpdateIMAPStatus( (mState==IMAP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
		}
		if (!ShouldContinue())
			UpdateIMAPStatus( 0.0, NULL, false, true);
		else
			UpdateIMAPStatus( delta, NULL);
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		int e;
		if (mConnection && (e = mConnection->Error())!=B_OK)
			errstr << "\nerror: " << e << ", " << mConnection->ErrorStr();
		UpdateIMAPStatus( 0.0, NULL, true);
		BmString text = Name() << ":\n\n" << errstr;
		HandleError( text);
		return false;
	}
	catch( Regexx::Exception &e) {
		// a problem occurred, we tell the user:
		BmString errstr = e.message();
		UpdateIMAPStatus( 0.0, NULL, true);
		BmString text = Name() << ":\n\nRegex: " << errstr;
		HandleError( text);
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdateIMAPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current IMAP3-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BmString "FAILED!" will be shown)
\*------------------------------------------------------------------------------*/
void BmImap::UpdateIMAPStatus( const float delta, const char* detailText, 
										  bool failed, bool stopped)
{
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed) {
		msg->AddString( MSG_TRAILING, 
							(BmString(ImapStates[mState].text) << " FAILED!").String());
		msg->AddBool( MSG_FAILED, true);
	} else if (stopped)
		msg->AddString( MSG_TRAILING, 
							(BmString(ImapStates[mState].text) 
								<< " Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, ImapStates[mState].text);
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
void BmImap::UpdateMailStatus( const float delta, const char* detailText, 
											int32 currMsg)
{
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
void BmImap::UpdateProgress( uint32 numBytes)
{
	float delta = (100.0*numBytes)/(mNewMsgTotalSize ? mNewMsgTotalSize : 1);
	BmString detailText = BmString("size: ") 
									<< BytesToString( mNewMsgSizes[mCurrMailNr-1]);
	UpdateMailStatus( delta, detailText.String(), mCurrMailNr);
}

/*------------------------------------------------------------------------------*\
	StateConnect()
		-	Initiates network-connection to IMAP-server
\*------------------------------------------------------------------------------*/
void BmImap::StateConnect()
{
	BmString server;
	uint16 port;
	mImapAccount->AddressInfo( server, port);
	BNetAddress addr;
	if (addr.SetTo( server.String(), port) != B_OK) {
		BmString s = BmString("Could not determine address of IMAP-Server ") 
							<< mImapAccount->Server();
		throw BM_network_error( s);
	}
	if (!Connect( &addr)) {
		BmString s = BmString("Could not connect to IMAP-Server ") 
							<< mImapAccount->Server() 
						  	<< "\n\bError:\n\t" << mErrorString;
		throw BM_network_error( s);
	}
	BmString encryptionType = mImapAccount->EncryptionType();
	if (TheNetEndpointRoster->SupportsEncryption()
	&& (encryptionType.ICompare(BmImapAccount::ENCR_TLS) == 0
		|| encryptionType.ICompare(BmImapAccount::ENCR_SSL) == 0)) {
		// straight TLS or SSL, we start the encryption layer: 
		if (mConnection->StartEncryption(encryptionType.String()) != B_OK)
			throw BM_network_error( "Failed to start connection encryption.\n");
	}
	// accept server greeting (either encrypted or unencrypted):
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	StateCapa()
		-	asks server for its capabilities
\*------------------------------------------------------------------------------*/
void BmImap::StateCapa()
{
	SetTaggedMode(true);
	BmString cmd("CAPABILITY");
	SendCommand( cmd);
	int32 count;
	try {
		if (!CheckForPositiveAnswer())
			return;
		Regexx rx;
		if ((count = rx.exec( 
			StatusText(), "\\bAUTH=([\\S]+)", Regexx::newline | Regexx::global
		))) {
			mSupportedAuthTypes.Truncate(0);
			for(int32 i=0; i<count; ++i)
				mSupportedAuthTypes << (i ? "," : "" ) << rx.match[i].atom[0];
		}
		if (rx.exec( StatusText(), "\\bSTARTTLS\\b", Regexx::newline))
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
void BmImap::StateStartTLS()
{
	// check if encryption via STARTTLS is requested (and possible):
	BmString encryptionType = mImapAccount->EncryptionType();

	// automatic means: use STARTTLS if available:
	if (encryptionType.ICompare(BmImapAccount::ENCR_AUTO) == 0
	&& mServerSupportsTLS) {
		encryptionType = BmImapAccount::ENCR_STARTTLS;
	}
	
	if (!TheNetEndpointRoster->SupportsEncryption()
	|| encryptionType.ICompare(BmImapAccount::ENCR_STARTTLS) != 0)
		return;

	// let's try to initiate TLS...
	SendCommand( "STARTTLS");
	if (!CheckForPositiveAnswer())
		return;

	// start connection encryption:
	status_t error = mConnection->StartEncryption(BmImapAccount::ENCR_TLS);
	if (error != B_OK)
		throw BM_network_error( "Failed to start connection encryption.\n");
}

/*------------------------------------------------------------------------------*\
	ExtractBase64()
		-	
\*------------------------------------------------------------------------------*/
void BmImap::ExtractBase64(const BmString& text, BmString& base64)
{
	text.CopyInto(base64, 2, text.Length());
}

/*------------------------------------------------------------------------------*\
	StateAuth()
		-	Sends user/passwd combination and checks result
\*------------------------------------------------------------------------------*/
void BmImap::StateAuth()
{
	BmString pwd;
	bool pwdOK = false;
	bool first = true;
	BmString authMethod = mImapAccount->AuthMethod();
	if (authMethod == BmImapAccount::AUTH_AUTO)
		authMethod = SuggestAuthType();
	while(!pwdOK) {
		bool pwdGiven = false;
		if (first && mImapAccount->PwdStoredOnDisk()) {
			// use stored password:
			pwd = mImapAccount->Password();
			pwdGiven = true;
		} else if (ShouldContinue()) {
			// ask user about password:
			BmString text( "Please enter password for IMAP-Account <");
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
		if (authMethod == BmImapAccount::AUTH_CRAM_MD5) {
			BmString cmd = BmString("AUTHENTICATE CRAM-MD5");
			SendCommand( cmd);
			AuthCramMD5(mImapAccount->Username(), mImapAccount->Password());
		} else if (authMethod == BmImapAccount::AUTH_DIGEST_MD5) {
			BmString cmd = BmString("AUTHENTICATE DIGEST-MD5");
			SendCommand( cmd);
			BmString serviceUri = BmString("imap/") << mImapAccount->Server();
			AuthDigestMD5(mImapAccount->Username(), mImapAccount->Password(),
							  serviceUri);
		} else {
			// authMethod == AUTH_LOGIN: send username and password as plain text:
			BmString cmd = BmString("LOGIN ") << mImapAccount->Username() << " ";
			SendCommand( cmd, pwd);
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
			// let's see if the server disconnected
			Regexx rx;
			if (rx.exec( StatusText(), "^\\*\\s+BYE", Regexx::newline)) {
				throw;
			} else {
				// it's most probably a wrong password...
				BmString errstr = err.what();
				int e;
				if (mConnection && (e = mConnection->Error())!=B_OK)
					errstr << "\nerror: " << e << ", " << mConnection->ErrorStr();
				BmString text = Name() << ":\n\n" << errstr;
				HandleError( text);
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	StateCheck()
		-	looks for new mail (only in inbox)
\*------------------------------------------------------------------------------*/
void BmImap::StateCheck()
{
	// select inbox and fetch number of existing messages...
	BmString cmd("SELECT inbox");
	SendCommand( cmd);
	if (!CheckForPositiveAnswer())
		return;
	Regexx rx;
	if (!rx.exec( StatusText(), "\\*\\s+(\\d+)\\s+exists", 
					  Regexx::newline | Regexx::nocase))
		throw BM_network_error( BmString("answer to '") << cmd 
											<< "' has unknown format");
	BmString msgCountStr = rx.match[0].atom[0];
	mMsgCount = atoi(msgCountStr.String());
	if (mMsgCount < 0)
		mMsgCount = 0;
	// ...and uidvalidity (domain of UIDs)
	BmString uidValidity;
	if (rx.exec( StatusText(), "\\buidvalidity\\s+(\\d+)", 
					 Regexx::newline | Regexx::nocase))
		uidValidity = rx.match[0].atom[0];

	mNewMsgTotalSize = 0;
	mNewMsgCount = 0;
	if (mMsgCount) {
		// fetch list with uid and size of every message:
		cmd = BmString("FETCH 1:") << mMsgCount << " (uid rfc822.size)";
		SendCommand( cmd);
		if (!CheckForPositiveAnswer())
			return;
		int fetchedCount = rx.exec( 
			StatusText(), 
			"^\\*\\s+(\\d+)\\s+fetch\\s+\\(\\s*uid\\s+(\\d+)\\s+rfc822\\.size\\s+(\\d+)", 
			Regexx::newline | Regexx::nocase | Regexx::global
		);
		if (!fetchedCount)
			throw BM_network_error( BmString("answer to '") << cmd 
												<< "' has unknown format");
		if (fetchedCount != mMsgCount) {
			BM_LOG( BM_LogRecv, 
					  BmString("Strange: server indicated ")<< mMsgCount
							<< " mails, but FETCH received " << fetchedCount
							<< " lines!");
			if (fetchedCount > mMsgCount)
				fetchedCount = mMsgCount;
		}
		// grab individual UID and message size from result:
		vector<int32> msgSizes;
		mMsgUIDs.clear();
		for( int32 i=0; i<fetchedCount; ++i) {
			BmString nrStr = rx.match[i].atom[0];
			int32 nr = atoi(nrStr.String());
			if (nr != i+1)
				throw BM_network_error( BmString("answer to '") << cmd 
													<< "has unexpeced msg-nr. in line "
													<< i+1);
			// compose our uid as "uidvalidity:uid", such that we never
			// confuse UIDs, should the server decide to renumber the messages:
			BmString uid = uidValidity + ":" + rx.match[i].atom[1];
			mMsgUIDs.push_back(uid);
			//
			BmString sizeStr = rx.match[i].atom[2];
			msgSizes.push_back(atoi(sizeStr.String()));
		}
	
		// compute total size of messages that are new to us:
		for( int32 i=0; i<mMsgCount; i++) {
			if (!mImapAccount->IsUIDDownloaded( mMsgUIDs[i])) {
				// msg is new (according to unknown UID)
				// add msg-size to total:
				mNewMsgTotalSize += msgSizes[i];
				mNewMsgSizes.push_back( msgSizes[i]);
				mNewMsgCount++;
			} else {
				// msg is old (according to known UID), we may have to remove it now:
				BmString log;
				bool shouldBeRemoved 
					= mImapAccount->ShouldUIDBeDeletedFromServer(mMsgUIDs[i], log);
				BM_LOG2( BM_LogRecv, log);
				if (shouldBeRemoved) {
					BM_LOG2( BM_LogRecv, log);
					if (!DeleteMailFromServer(mMsgUIDs[i]))
						return;
				}
			}
		}
	}

	// remove local UIDs that are not listed on the server anymore:
	BmString removedUids = mImapAccount->AdjustToCurrentServerUids( mMsgUIDs);
	BM_LOG( BM_LogRecv, removedUids);

	if (mNewMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no new messages found, nothing to do
	}
}

/*------------------------------------------------------------------------------*\
	StateRetrieve()
		-	retrieves all new mails from server
\*------------------------------------------------------------------------------*/
void BmImap::StateRetrieve()
{
	BmString cmd;
	mCurrMailNr = 1;
	for( int32 i=0; i<mMsgCount; ++i) {
		if (mImapAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is old (according to known UID), we skip it:
			continue;
		}
		// fetch current mail
		BmString serverUID = LocalUidToServerUid(mMsgUIDs[i]);
		cmd = BmString("UID FETCH ") << serverUID << " rfc822";
		SendCommand( cmd);
		time_t before = time(NULL);
		if (!CheckForPositiveAnswer( mNewMsgSizes[mCurrMailNr-1], false, true))
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
			// oops, we better complain:
			throw BM_network_error( 
				BmString("Received mail has ") << mAnswerText.Length()
					<< " bytes but it was announced to have " 
					<< mNewMsgSizes[mCurrMailNr-1] << " bytes!"
			);
		}
		// now create a mail from the received data...
		BM_LOG2( BM_LogRecv, "Creating mail...");
		BmRef<BmMail> mail = new BmMail( mAnswerText, mImapAccount->Name());
		if (mail->InitCheck() != B_OK)
			goto CLEAN_UP;
		// ...set default folder according to pop-account settings...
		mail->SetDestFolderName( mImapAccount->HomeFolder());
		// ...execute mail-filters for this mail...
		BM_LOG2( BM_LogRecv, "...applying filters (in memory)...");
		mail->ApplyInboundFilters();
		// ...and store mail on disk:
		BM_LOG2( BM_LogRecv, "...storing mail...");
		if (!mail->Store())
			goto CLEAN_UP;
		BM_LOG2( BM_LogRecv, "...done");
		mImapAccount->MarkUIDAsDownloaded( mMsgUIDs[i]);
		//	delete the retrieved message if required to do so immediately:
		BmString log;
		if (mImapAccount->ShouldUIDBeDeletedFromServer(mMsgUIDs[i], log)) {
			BM_LOG2( BM_LogRecv, log);
			if (!DeleteMailFromServer(mMsgUIDs[i]))
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
	LocalUidToServerUid(uid)
		-	converts the local UID to the one given by server (by removing the 
			uidvalidity from the local uid).
\*------------------------------------------------------------------------------*/
BmString BmImap::LocalUidToServerUid(const BmString& uid) const
{
	BmString serverUID;
	int32 pos = uid.FindFirst(':');
	if (pos >= 0)
		serverUID.SetTo(uid.String()+pos+1);
	else
		serverUID = uid;
	return serverUID;
}

/*------------------------------------------------------------------------------*\
	DeleteMailFromServer(uid)
		-	deletes the mail with the given UID
\*------------------------------------------------------------------------------*/
bool BmImap::DeleteMailFromServer(const BmString& uid)
{
	// we need to split off the uidvalidity from our local UID:
	BmString serverUID = LocalUidToServerUid(uid);
	BmString cmd;
	cmd = BmString("UID STORE ") << serverUID << " flags.silent (\\deleted)";
	SendCommand( cmd);
	return CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	StateDisconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmImap::StateDisconnect()
{
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
void BmImap::Quit( bool WaitForAnswer)
{
	BmString cmd("LOGOUT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer)
			CheckForPositiveAnswer();
	} catch(...) {	}
	Disconnect();
}

/*------------------------------------------------------------------------------*\
	SetTaggedMode()
		-	activates/deactivates the use of tags. 
		-	Most of IMAP actually runs in	tagged mode, only the greetings 
			isn't tagged.
\*------------------------------------------------------------------------------*/
void BmImap::SetTaggedMode(bool tagged)
{
	mTaggedMode = tagged;
	if (!mTaggedMode)
		mCurrTag.Truncate(0);
}

/*------------------------------------------------------------------------------*\
	CheckForPositiveAnswer()
		-	adds current tag to info msg such that status filter knows what
			to look for.
\*------------------------------------------------------------------------------*/
bool BmImap::CheckForPositiveAnswer( uint32 expectedSize, 
												 bool /*dotstuffDecoding*/,
												 bool update,
												 BMessage* infoMsg)
{
	if (!infoMsg) {
		mInfoMsg.MakeEmpty();
		infoMsg = &mInfoMsg;
	}
	infoMsg->AddString(IMSG_NEEDED_TAG, mCurrTag.String());
	return inherited::CheckForPositiveAnswer( expectedSize, false, update, 
															infoMsg);
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	smuggles tag into command (if required) and sends it off:
\*------------------------------------------------------------------------------*/
void BmImap::SendCommand( const BmString& cmd, const BmString& secret,
								  bool dotstuffEncoding, bool update)
{
	if (mTaggedMode) {
		mCurrTag = BmString("bm") << ++mCurrTagNr;
		BmString taggedCmd = mCurrTag + " " + cmd;
		inherited::SendCommand(taggedCmd, secret, dotstuffEncoding, update);
	} else
		inherited::SendCommand(cmd, secret, dotstuffEncoding, update);
}

/*------------------------------------------------------------------------------*\
	SupportsTLS()
		-	returns whether or not the server has indicated that it supports 
			the STARTTLS command
\*------------------------------------------------------------------------------*/
bool BmImap::SupportsTLS() const
{
	return mServerSupportsTLS;
}

/*------------------------------------------------------------------------------*\
	SuggestAuthType()
		-	looks at the auth-types supported by the server and selects 
			the most secure of those that is supported by Beam.
\*------------------------------------------------------------------------------*/
BmString BmImap::SuggestAuthType() const
{
	if (mSupportedAuthTypes.IFindFirst( BmImapAccount::AUTH_DIGEST_MD5) >= 0)
		return BmImapAccount::AUTH_DIGEST_MD5;
	else if (mSupportedAuthTypes.IFindFirst( BmImapAccount::AUTH_CRAM_MD5) >= 0)
		return BmImapAccount::AUTH_CRAM_MD5;
	else
		return BmImapAccount::AUTH_LOGIN;
}
