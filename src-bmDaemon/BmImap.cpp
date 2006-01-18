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
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmImapStatusFilter::Filter( const char* srcBuf, uint32& srcLen, 
										   char* destBuf, uint32& destLen) {
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;

	BmString tagStr;
	if (mInfoMsg)
		tagStr = mInfoMsg->FindString(BmImap::IMSG_NEEDED_TAG);

	BmString line;
	while( src<srcEnd) {
		mLineBuf << *src++;
		if (*src=='\n') {
			mLineBuf << *src++;
			// now we have a complete line in the ring buffer, we fetch it...
			line = mLineBuf;
			// ...and check it's status:
			if (tagStr.Length() && line.ICompare(tagStr, tagStr.Length()) == 0
			|| line.ICompare("* ", 2) == 0) {
				// this is a tagged answer line, this means that we have
				// reached the end of the buffer:
				line.RemoveAll("\r");
				mStatusText << line;
				mLastStatusLine = line;
				mHaveStatus = true;
				srcLen = src-srcBuf;
				destLen = 0;
				// we have reached the end if no tag is expected (the answer
				// will be one line only) or if this is the tagged line:
				if (!tagStr.Length() || line.ICompare(tagStr, tagStr.Length()) == 0)
					mEndReached = true;
				return;
			}
			break;
		}
	}
	uint32 size = min( destLen, (uint32)(src-srcBuf));
	memcpy( destBuf, srcBuf, size);
	srcLen = destLen = size;
	if (mUpdate && destLen)
		mJob->UpdateProgress( destLen);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmImapStatusFilter::CheckForPositiveAnswer() {
	BmString tagStr;
	if (mInfoMsg)
		tagStr = mInfoMsg->FindString(BmImap::IMSG_NEEDED_TAG);
	if (mStatusText.Length()) {
		BmString taggedBad;
		if (tagStr.Length())
			taggedBad = tagStr + " BAD";
		// a problem is indicated by "* BAD" or "<tag> BAD":
		if ((taggedBad.Length() 
			&& mLastStatusLine.ICompare(taggedBad, taggedBad.Length()) == 0)
		|| mLastStatusLine.ICompare("* BAD", 4) == 0) {
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
BmImap::ImapState BmImap::ImapStates[BmImap::IMAP_FINAL] = {
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
	,	mCurrTagNr( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmImap()
		-	destructor
\*------------------------------------------------------------------------------*/
BmImap::~BmImap() { 
	BM_LOG_FINISH( BM_LOGNAME);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the Imapper should continue to run
		-	in addition to the inherited behaviour, the Imapper should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmImap::ShouldContinue() {
	return CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB
			 || inherited::ShouldContinue();
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all IMAP-stages and calls the corresponding 
			handlers
		-	returns whether or not the Imapper has completed it's job
\*------------------------------------------------------------------------------*/
bool BmImap::StartJob() {
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
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdateIMAPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current IMAP3-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BmString "FAILED!" will be shown)
\*------------------------------------------------------------------------------*/
void BmImap::UpdateIMAPStatus( const float delta, const char* detailText, 
										  bool failed, bool stopped) {
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
void BmImap::UpdateProgress( uint32 numBytes) {
	float delta = (100.0*numBytes)/(mNewMsgTotalSize ? mNewMsgTotalSize : 1);
	BmString detailText = BmString("size: ") 
									<< BytesToString( mNewMsgSizes[mCurrMailNr-1]);
	UpdateMailStatus( delta, detailText.String(), mCurrMailNr);
}

/*------------------------------------------------------------------------------*\
	StateConnect()
		-	Initiates network-connection to IMAP-server
\*------------------------------------------------------------------------------*/
void BmImap::StateConnect() {
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
void BmImap::StateCapa() {
	BmString cmd("CAPABILITY");
	TagAndSendCommand( cmd);
	int32 count;
	try {
		CheckForTaggedPositiveAnswer();
		Regexx rx;
		if ((count = rx.exec( 
			StatusText(), "\\bAUTH=([\\S]+)", Regexx::newline
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
void BmImap::StateStartTLS() {
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
	TagAndSendCommand( "STARTTLS");
	CheckForTaggedPositiveAnswer();

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
void BmImap::StateAuth() {
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
			AuthCramMD5(mImapAccount->Username(), mImapAccount->Password());
		} else if (authMethod == BmImapAccount::AUTH_DIGEST_MD5) {
			BmString serviceUri = BmString("imap/") << mImapAccount->Server();
			AuthDigestMD5(mImapAccount->Username(), mImapAccount->Password(),
							  serviceUri);
		} else {
			// authMethod == AUTH_LOGIN: send username and password as plain text:
			BmString cmd = BmString("LOGIN ") << mImapAccount->Username() << " ";
			TagAndSendCommand( cmd, pwd);
		}
		try {
			if (CheckForTaggedPositiveAnswer())
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
void BmImap::StateCheck() {
return;
	uint32 msgNum = 0;

	BmString cmd("SELECT inbox");
	TagAndSendCommand( cmd);
	if (!CheckForTaggedPositiveAnswer())
		return;

	cmd = ("SEARCH unseen");
	TagAndSendCommand( cmd);
	if (!CheckForTaggedPositiveAnswer())
		return;

	if (sscanf( StatusText().String()+4, "%ld", &mMsgCount) != 1 
	|| mMsgCount < 0)
		throw BM_network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		// we remove all local UIDs, since none are listed on the server:
		BmString removedUids = mImapAccount->AdjustToCurrentServerUids( mMsgUIDs);
		BM_LOG2( BM_LogRecv, removedUids);
		return;									// no messages found, nothing more to do
	}

	// we try to fetch a list of unique message IDs from server:
	cmd = BmString("UIDL");
	TagAndSendCommand( cmd);
	try {
		// The UIDL-command may not be implemented by this server, so we 
		// do not require a positive answer, we just hope for it:
		if (!CheckForTaggedPositiveAnswer())
			return;								// interrupted, we give up
		// ok, we've got the UIDL-listing, so we fetch it,
		// fetch UIDLs one per line and store them in array:
		Regexx rx;
		int numLines = rx.exec( mAnswerText, "\\s*(\\d+)\\s+(.+?)\\s*$\\n", 
										Regexx::newline | Regexx::global);
		if (numLines < mMsgCount)
			throw BM_network_error(	BmString("answer to UIDL has unknown format"
														", too few lines matching"));
		for( int32 i=0; i<mMsgCount; ++i)
			mMsgUIDs.push_back( rx.match[i].atom[1]);
		// we remove local UIDs that are not listed on the server anymore:
		BmString removedUids = mImapAccount->AdjustToCurrentServerUids( mMsgUIDs);
		BM_LOG( BM_LogRecv, removedUids);
	} catch( BM_network_error& err) {
		// no UIDL-listing from server, we will have to get by without...
	}

	// compute total size of messages that are new to us:
	mNewMsgTotalSize = 0;
	mNewMsgCount = 0;
	cmd = "LIST";
	TagAndSendCommand( cmd);
	if (!CheckForTaggedPositiveAnswer())
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
		time_t timeDownloaded;
		if (!mImapAccount->IsUIDDownloaded( mMsgUIDs[i], &timeDownloaded)) {
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
			if (!mImapAccount->DeleteMailFromServer()) {
				BM_LOG2( BM_LogRecv, BmString("Leaving mail with UID ")<<mMsgUIDs[i]
											<<" on server\n"
											<<"since user has told us to "
											<<"leave all mails on server.");
			} else {
				time_t expirationTime 
					= timeDownloaded + 60 * 60 * 24 * mImapAccount->DeleteMailDelay();
				time_t now = time(NULL);
				if (expirationTime <= now && mImapAccount->DeleteMailFromServer()) {
					// remove
					BM_LOG( BM_LogRecv, 
							  BmString("Removing mail with UID ")<<mMsgUIDs[i]
									<<" from server\n"
									<<"since it has been downloaded on "
									<<TimeToString( timeDownloaded)
									<<",\nit's expiration time is " 
									<<TimeToString( expirationTime)
									<<"\nand now it is "	<< TimeToString( now));
					cmd = BmString("DELE ") << i+1;
					TagAndSendCommand( cmd);
					if (!CheckForTaggedPositiveAnswer())
						return;
				} else {
					BM_LOG2( BM_LogRecv, 
								BmString("Leaving mail with UID ")<<mMsgUIDs[i]
									<<" on server\n"
									<<"since it has been downloaded on "
									<<TimeToString( timeDownloaded)
									<<",\nit's expiration time is " 
									<<TimeToString( expirationTime)
									<<"\nand now it is "	<< TimeToString( now));
				}
			}
		}
	}
	if (mNewMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no new messages found, nothing to do
	}
}

/*------------------------------------------------------------------------------*\
	StateRetrieve()
		-	retrieves all new mails from server
\*------------------------------------------------------------------------------*/
void BmImap::StateRetrieve() {
return;
	BmString cmd;
	mCurrMailNr = 1;
	for( int32 i=0; i<mMsgCount; ++i) {
		if (mImapAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is old (according to known UID), we skip it:
			continue;
		}
		cmd = BmString("RETR ") << i+1;
		TagAndSendCommand( cmd);
		time_t before = time(NULL);
		if (!CheckForTaggedPositiveAnswer( mNewMsgSizes[mCurrMailNr-1], true))
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
		if (mImapAccount->DeleteMailFromServer() 
		&& mImapAccount->DeleteMailDelay() <= 0) {
			cmd = BmString("DELE ") << i+1;
			TagAndSendCommand( cmd);
			if (!CheckForTaggedPositiveAnswer())
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
void BmImap::StateDisconnect() {
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
void BmImap::Quit( bool WaitForAnswer) {
	BmString cmd("LOGOUT");
	try {
		TagAndSendCommand( cmd);
		if (WaitForAnswer)
			CheckForTaggedPositiveAnswer();
	} catch(...) {	}
	Disconnect();
}

/*------------------------------------------------------------------------------*\
	CheckForTaggedPositiveAnswer()
		-	
\*------------------------------------------------------------------------------*/
bool BmImap::CheckForTaggedPositiveAnswer( uint32 expectedSize, 
														 bool update) 
{
	BMessage infoMsg;
	infoMsg.AddString(IMSG_NEEDED_TAG, mCurrTag.String());
	return CheckForPositiveAnswer( expectedSize, false, update, &infoMsg);
}

/*------------------------------------------------------------------------------*\
	TagAndSendCommand( cmd)
		-	smuggles tag into command and send it off:
\*------------------------------------------------------------------------------*/
void BmImap::TagAndSendCommand( const BmString& cmd, const BmString& secret,
										  bool dotstuffEncoding, bool update) 
{
	mCurrTag = BmString("bm") << ++mCurrTagNr;
	BmString taggedCmd = mCurrTag + " " + cmd;
	SendCommand(taggedCmd, secret, dotstuffEncoding, update);
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
