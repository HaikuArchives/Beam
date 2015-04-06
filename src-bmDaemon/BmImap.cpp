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
	char c;

	if (!mLiteralCharCount) {
		BmString tagStr;
		if (mInfoMsg)
			tagStr = mInfoMsg->FindString(BmImap::IMSG_NEEDED_TAG);

		// setup a regex-string that can decide whether or not a given line
		// is a status line.
		// IMAP defines '*' (data), '+' (continuation) or the tag as indicator,
		BmString statusRxStr
			= tagStr.Length()
				? BmString("^(\\*|\\+|") << tagStr << ")\\s+"
				: BmString("^(\\*|\\+)\\s+");

		Regexx rx;
		while(src<srcEnd) {
			c = *src++;
			mLineBuf << c;
			if (c == '\n') {
				// now we have a complete line in the ring buffer, we fetch it...
				mLastStatusLine = mLineBuf;
				// ...and check it's status:
				if (rx.exec( mLastStatusLine, statusRxStr)) {
					// this is a status line
					mLastStatusLine.RemoveAll("\r");
					if (mLastStatusLine.ByteAt(0) == '*'
					|| mLastStatusLine.ByteAt(0) == '+')
						// normal status (at top of answer stream) or continuation
						// request
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
	uint32 size = std::min( destLen, srcLen);
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
				: BmString("^\\*\\s+(BAD|NO)\\b");
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
	ImapState( "cleanup...", &BmImap::StateCleanup),
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
	,	mExpungeCount( 0)
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
	if (mConnection && mConnection->IsStopRequested())
		return false;
	return CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB
			 || inherited::ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	SetupAdditionalInfo()
		-
\*------------------------------------------------------------------------------*/
void BmImap::SetupAdditionalInfo( BMessage* additionalInfo)
{
	additionalInfo->AddString(BmNetEndpoint::MSG_CLIENT_CERT_NAME,
									  mImapAccount->ClientCertificate().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_SERVER_NAME,
									  mImapAccount->Server().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_ACCEPTED_CERT_ID,
									  mImapAccount->AcceptedCertID().String());
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
	const float delta = (100.0f / float(IMAP_DONE-skipped));
	try {
		for( mState=IMAP_CONNECT; ShouldContinue() && mState<IMAP_DONE; ++mState) {
			if (ImapStates[mState].skip)
				continue;
			TStateMethod stateFunc = ImapStates[mState].func;
			UpdateIMAPStatus( (mState==IMAP_CONNECT ? 0.0f : delta), NULL);
			(this->*stateFunc)();
			if (!ShouldContinue()) {
				Disconnect();
				break;
			}
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
	catch( std::exception &err) {
		BmString errMsg;
		errMsg << err.what() << " (" << typeid(err).name() << ")";
		HandleError( errMsg);
		return false;
	}
	catch( ...) {
		BmString errMsg;
		errMsg << "The job for account " << mImapAccount->Name()
				 << "received an unknown exception and died!";
		HandleError(errMsg);
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
	std::auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
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
	std::auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "mailbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text.String());
	if (detailText)
		msg->AddString( MSG_TRAILING, detailText);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	UpdateCleanupStatus( delta, currMsg)
		- informs the interested party about the message currently dealt with
\*------------------------------------------------------------------------------*/
void BmImap::UpdateCleanupStatus( const float delta, int32 currMsg) {
	BmString text;
	uint32 count = mCleanupMsgUIDs.size();
	if (count > 0) {
		text = BmString() << currMsg << " of " << count;
	} else {
		text = "none";
	}
	std::auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "mailbar.cleanup");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text.String());
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	UpdateProgress( numBytes)
		-
\*------------------------------------------------------------------------------*/
void BmImap::UpdateProgress( uint32 numBytes)
{
	float delta = (100.0f*float(numBytes))/float(mNewMsgTotalSize ? mNewMsgTotalSize : 1);
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
	BNetAddress addr;
	if (addr.SetTo( mImapAccount->Server().String(),
						 mImapAccount->PortNr()) != B_OK) {
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
		if (!StartEncryption(encryptionType.String()))
			return;
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
	StateStartTLS()
		-
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

	StartEncryption(BmImapAccount::ENCR_TLS);
}

/*------------------------------------------------------------------------------*\
	StartEncryption(encryptionType)
		-	extends activation of SSL/TLS encryption layer with automatic
			updating of newly accepted certificate ID.
\*------------------------------------------------------------------------------*/
bool BmImap::StartEncryption(const char* encType)
{
	bool ok = inherited::StartEncryption(encType);
	if (ok) {
		BmString certID = mConnection->NewAcceptedCertID();
		if (certID.Length() && mImapAccount->AcceptedCertID() != certID) {
			mImapAccount->AcceptedCertID(certID);
			TheRecvAccountList->MarkAsChanged();
		}
	}
	return ok;
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
	if (!authMethod.Length() || authMethod == BmImapAccount::AUTH_NONE)
		return;			// no authentication needed (*very* unlikely...)
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
			AuthCramMD5(mImapAccount->Username(), pwd);
		} else if (authMethod == BmImapAccount::AUTH_DIGEST_MD5) {
			BmString cmd = BmString("AUTHENTICATE DIGEST-MD5");
			SendCommand( cmd);
			BmString serviceUri = BmString("imap/") << mImapAccount->Server();
			AuthDigestMD5(mImapAccount->Username(), pwd, serviceUri);
		} else if (authMethod == BmImapAccount::AUTH_LOGIN) {
			// send username and password as plain text:
			BmString cmd = BmString("LOGIN ") << mImapAccount->Username() << " ";
			SendCommand( cmd, pwd);
		} else {
			throw BM_runtime_error( BmString("Unknown authentication type '")
										   << authMethod << "' found!?! Skipping!");
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
			if (rx.exec( StatusText(), "^\\*\\s+BYE",
						 Regexx::newline | Regexx::nocase)) {
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
	mCleanupMsgUIDs.clear();
	if (mMsgCount) {
		// fetch list with uid and size of every message:
		// add "flags" here to the list of data to fetch.
		cmd = BmString("FETCH 1:") << mMsgCount << " (uid rfc822.size flags)";
		SendCommand( cmd);
		if (!CheckForPositiveAnswer())
			return;
		const BmString& status = StatusText();
		uint32 fetchedCount = rx.exec(
			status, "^\\*\\s+(\\d+)\\s+fetch\\s+(\\([^\\r\\n]*\\))",
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
		// grab individual UID, flags and message size from result:
		vector<uint32> msgSizes;
		mMsgFlags.clear();
		mMsgUIDs.clear();
		BmImapNestedStringList nestedList;
		for(uint32 i=0; i<fetchedCount; ++i) {
			BmString nrStr = rx.match[i].atom[0];
			uint32 nr = atoi(nrStr.String());
			if (nr != i+1)
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' has unexpected msg-nr. in line "
													<< i+1);
			const char* posInText = status.String() + rx.match[i].atom[1].start();
			if (!nestedList.Parse(posInText))
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' has unparsable string list in line "
													<< i+1);
			uint32 listSize = nestedList.Size();
			if (listSize % 2 != 0)
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' has uneven number of items "
													<< "in string list in line "
													<< i+1);
			for(uint32 l = 0; l < listSize; l += 2) {
				const BmString& key = nestedList[l].Text();
				if (key.ICompare("UID") == 0) {
					// compose our uid as "uidvalidity:uid", such that we never
					// confuse UIDs, should the server decide to renumber the messages:
					BmString uid = uidValidity + ":" + nestedList[l+1].Text();
					mMsgUIDs.push_back(uid);
				} else if (key.ICompare("FLAGS") == 0) {
					unsigned flags = StringToFlags(nestedList[l+1]);
					mMsgFlags.push_back(flags);
				} else if (key.ICompare("RFC822.SIZE") == 0) {
					const BmString& sizeStr = nestedList[l+1].Text();
					msgSizes.push_back(atoi(sizeStr.String()));
				} else
					throw BM_network_error( BmString("answer to '") << cmd
														<< "' contains unrequested key '" << key
														<< "' in string list in line "
														<< i+1);
			}
			if (mMsgUIDs.size() != i + 1)
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' is missing UID in line "
													<< i+1);
			if (mMsgFlags.size() != i + 1)
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' is missing FLAGS in line "
													<< i+1);
			if (msgSizes.size() != i + 1)
				throw BM_network_error( BmString("answer to '") << cmd
													<< "' is missing RFC822.SIZE in line "
													<< i+1);
		}

		if (mMsgUIDs.size() != mMsgCount)
			throw BM_network_error( BmString("answer to '") << cmd
												<< "' does not have enough UIDs");
		if (mMsgFlags.size() != mMsgCount)
			throw BM_network_error( BmString("answer to '") << cmd
												<< "' does not have enough FLAGS");
		if (msgSizes.size() != mMsgCount)
			throw BM_network_error( BmString("answer to '") << cmd
												<< "' does not have enough RFC822.SIZEs");

		// compute total size of messages that are new to us:
		for(uint32 i=0; i<mMsgCount; i++) {
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
					// store msg-UID for cleanup state
					mCleanupMsgUIDs.push_back(mMsgUIDs[i]);
				}
			}
		}
	}

	// remove local UIDs that are not listed on the server anymore:
	BmString removedUids = mImapAccount->AdjustToCurrentServerUids( mMsgUIDs);
	BM_LOG( BM_LogRecv, removedUids);

	if (mNewMsgCount == 0)
		UpdateMailStatus( 0, NULL, 0);
}

/*------------------------------------------------------------------------------*\
	StateCleanup()
		-	deletes all old mails from server
\*------------------------------------------------------------------------------*/
void BmImap::StateCleanup() {
	BmString cmd;
	uint32 count = mCleanupMsgUIDs.size();
	if (count == 0)
		return;
	for(uint32 i = 0; i < count; ++i) {
		if (!DeleteMailFromServer(mCleanupMsgUIDs[i]))
			return;
		float delta = 100.0f / float(count != 0 ? count : 1);
		UpdateCleanupStatus( delta, i + 1);
	}
	mCleanupMsgUIDs.clear();
	UpdateCleanupStatus( 100.0, count);
}

/*------------------------------------------------------------------------------*\
	StateRetrieve()
		-	retrieves all new mails from server
\*------------------------------------------------------------------------------*/
void BmImap::StateRetrieve()
{
	UpdateMailStatus( -1, NULL, 0);
	BmString cmd;
	mCurrMailNr = 1;
	for(uint32 i=0; mNewMsgCount>0 && i<mMsgCount; ++i) {
		if (mImapAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is old (according to known UID), we skip it:
			continue;
		}
		// fetch current mail
		BmString serverUID = LocalUidToServerUid(mMsgUIDs[i]);
// TODO: May also add more stuff to FETCH, like "flags"
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
		if ((uint32)mAnswerText.Length() != mNewMsgSizes[mCurrMailNr-1]) {
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
		BmRef<BmMail> mail = new BmMail( mAnswerText, mImapAccount->Name());
		if (mail->InitCheck() != B_OK)
			goto CLEAN_UP;
		// ...set IMAP UID - TODO: Use serverUID instead?
		mail->ImapUID(mMsgUIDs[i]);
		// ...set the message flags
		uint32 flags = mMsgFlags[i];
		if (flags & FLAG_ANSWERED)
			mail->MarkAs("Replied");
		else if (flags & FLAG_SEEN)
			mail->MarkAs("Read");
		else if (flags & FLAG_DRAFT)
			mail->MarkAs("Draft");
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
		bool shouldBeDeleted
			= mImapAccount->ShouldUIDBeDeletedFromServer(mMsgUIDs[i], log);
		BM_LOG2( BM_LogRecv, log);
		if (shouldBeDeleted) {
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
	mExpungeCount++;
	return CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	StateDisconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmImap::StateDisconnect()
{
	if (mExpungeCount) {
		BmString cmd("EXPUNGE");
		SendCommand( cmd);
		if (!CheckForPositiveAnswer())
			return;
	}
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

BmString BmImap::FlagsToString(uint32 flags)
{
	BmString string;
	bool first = true;
	if (flags & FLAG_SEEN) {
		string << "\\Seen";
		first = false;
	}
	if (flags & FLAG_ANSWERED) {
		if (first)
			string << "\\Answered";
		else
			string << " \\Answered";
		first = false;
	}
	if (flags & FLAG_FLAGGED) {
		if (first)
			string << "\\Flagged";
		else
			string << " \\Flagged";
		first = false;
	}
	if (flags & FLAG_DELETED) {
		if (first)
			string << "\\Deleted";
		else
			string << " \\Deleted";
		first = false;
	}
	if (flags & FLAG_DRAFT) {
		if (first)
			string << "\\Draft";
		else
			string << " \\Draft";
		first = false;
	}
	return string;
}

uint32 BmImap::StringToFlags(const BmImapNestedStringList& flagsString)
{
	uint32 flags = 0;
	for(uint32 i = 0; i < flagsString.Size(); ++i) {
		if (flagsString[i].Text() == "\\Seen")
			flags |= FLAG_SEEN;
		else if (flagsString[i].Text() == "\\Answered")
			flags |= FLAG_ANSWERED;
		else if (flagsString[i].Text() == "\\Flagged")
			flags |= FLAG_FLAGGED;
		else if (flagsString[i].Text() == "\\Deleted")
			flags |= FLAG_DELETED;
		else if (flagsString[i].Text() == "\\Draft")
			flags |= FLAG_DRAFT;
	}
	return flags;
}

