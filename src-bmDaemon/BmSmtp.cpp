/*
	BmSmtp.cpp
		- Implements the main SMTP-client-class: BmSmtp

		$Id$
*/

#include <memory.h>
#include <memory>
#include <stdio.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmSmtp.h"
#include "BmSmtpAccount.h"
#include "BmPrefs.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME (BString("SMTP_")<<Name())

/*------------------------------------------------------------------------------*\
	SmtpStates[]
		-	array of SMTP3-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmSmtp::SmtpState BmSmtp::SmtpStates[BmSmtp::SMTP_FINAL] = {
	SmtpState( "connect...", &BmSmtp::Connect),
	SmtpState( "auth...", &BmSmtp::Auth),
	SmtpState( "rcpt...", &BmSmtp::Rcpt),
	SmtpState( "data...", &BmSmtp::Data),
	SmtpState( "quit...", &BmSmtp::Disconnect),
	SmtpState( "done", NULL)
};

/*------------------------------------------------------------------------------*\
	BmSmtp( info)
		-	contructor
\*------------------------------------------------------------------------------*/
BmSmtp::BmSmtp( const BString& name, BmSmtpAccount* account)
	:	BmJobModel( name)
	,	mSmtpAccount( account)
	,	mSmtpServer()
	,	mConnected( false)
	,	mState( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmSmtp()
		-	destructor
		-	frees all associated memory (hopefully)
\*------------------------------------------------------------------------------*/
BmSmtp::~BmSmtp() { 
	if (mConnected) {
		//	We try to inform SMTP-server about QUIT, if still connected.
		// This probably means that we ran into an exception, so maybe it's not really
		// a good idea...(?)
		Quit();
	}
	TheLogHandler->FinishLog( BM_LOGNAME);
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all SMTP-stages and calls the corresponding handlers
\*------------------------------------------------------------------------------*/
bool BmSmtp::StartJob() {

	const float delta = (100.0 / SMTP_DONE);
	const bool failed=true;

	mSmtpServer.InitCheck() == B_OK									||	BM_THROW_RUNTIME("BmSmtp: could not create NetEndpoint");
	try {
		for( mState=SMTP_CONNECT; ShouldContinue() && mState<SMTP_DONE; ++mState) {
			TStateMethod stateFunc = SmtpStates[mState].func;
			UpdateSMTPStatus( (mState==SMTP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
		}
		UpdateSMTPStatus( delta, NULL);
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BString errstr = err.what();
		int e;
		if ((e = mSmtpServer.Error()))
			errstr << "\nerror: " << e << ", " << mSmtpServer.ErrorStr();
		UpdateSMTPStatus( 0.0, NULL, failed);
		BString text = Name() << "\n\n" << errstr;
		BM_SHOWERR( BString("BmSmtp: ") << text);
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdateSMTPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current SMTP3-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BString "FAILED!" will be shown)
\*------------------------------------------------------------------------------*/
void BmSmtp::UpdateSMTPStatus( const float delta, const char* detailText, 
										  bool failed) {
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_SMTP, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed)
		msg->AddString( MSG_TRAILING, BString(SmtpStates[mState].text) << " FAILED!");
	else
		msg->AddString( MSG_TRAILING, SmtpStates[mState].text);
	if (detailText)
		msg->AddString( MSG_LEADING, detailText);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	UpdateMailStatus( delta, detailText)
		- informs the interested party about the message currently dealt with
\*------------------------------------------------------------------------------*/
void BmSmtp::UpdateMailStatus( const float delta, const char* detailText, 
											int32 currMsg) {
	BString text;
/*
	if (mNewMsgCount) {
		text = BString() << currMsg << " of " << mNewMsgCount;
	} else {
		text = "none";
	}
*/
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_SMTP, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "mailbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text);
	if (detailText)
		msg->AddString( MSG_TRAILING, detailText);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	Initiates network-connection to SMTP-server
\*------------------------------------------------------------------------------*/
void BmSmtp::Connect() {
	if (mSmtpServer.Connect( mSmtpAccount->SMTPAddress()) != B_OK) {
		BString s = BString("Could not connect to SMTP-Server ") << mSmtpAccount->SMTPServer();
		throw BM_network_error( s);
	}
	mConnected = true;
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	Auth()
		-	Sends user/passwd combination and checks result
\*------------------------------------------------------------------------------*/
void BmSmtp::Auth() {
/*
	BString cmd = BString("USER ") << mSmtpAccount->Username();
	SendCommand( cmd);
	if (CheckForPositiveAnswer( SINGLE_LINE)) {
		cmd = BString("PASS ") << mSmtpAccount->Password();
		SendCommand( cmd);
		CheckForPositiveAnswer( SINGLE_LINE);
	}
*/
}

/*------------------------------------------------------------------------------*\
	Rcpt()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::Rcpt() {
/*
	int32 msgNum = 0;

	BString cmd("STAT");
	SendCommand( cmd);
	if (!CheckForPositiveAnswer( SINGLE_LINE))
		return;
	if (sscanf( mReplyLine.String()+4, "%ld", &mMsgCount) != 1 || mMsgCount < 0)
		throw BM_network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no messages found, nothing more to do
	}

	// we try to fetch a list of unique message IDs from server:
	mMsgUIDs = new BString[mMsgCount];
	mMsgSizes = new int32[mMsgCount];
	cmd = BString("UIDL");
	SendCommand( cmd);
	// The UIDL-command may not be implemented by this server, so we 
	// do not require a postive answer, we just hope for it:
	if (!GetAnswer( MULTI_LINE)) 			
		return;									// interrupted, we give up
	if (mReplyLine[0] == '+') {
		// ok, we've got the UIDL-listing, so we fetch it:
		char msgUID[71];
		// fetch UIDLs one per line and store them in array:
		const char *p = mAnswer.String();
		for( int32 i=0; i<mMsgCount; ++i) {
			if (sscanf( p, "%ld %70s", &msgNum, msgUID) != 2 || msgNum <= 0)
				throw BM_network_error( BString("answer to UIDL has unknown format, msg ") << i+1);
			mMsgUIDs[i] = msgUID;
			// skip to next line:
			if (!(p = strstr( p, "\r\n")))
				throw BM_network_error( BString("answer to UIDL has unknown format, msg ") << i+1);
			p += 2;
		}
	} else {
		// no UIDL-listing from server, we will have to get by without...
	}

	// compute total size of messages that are new to us:
	mMsgTotalSize = 0;
	mNewMsgCount = 0;
	cmd = "LIST";
	SendCommand( cmd);
	if (!CheckForPositiveAnswer( MULTI_LINE))
		return;
	const char *p = mAnswer.String();
	for( int32 i=0; i<mMsgCount; i++) {
		if (!mSmtpAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is new (according to unknown UID)
			// fetch msgsize for message...
			if (sscanf( p, "%ld %ld", &msgNum, &mMsgSizes[i]) != 2 || msgNum != i+1)
				throw BM_network_error( BString("answer to LIST has unknown format, msg ") << i+1);
			// add msg-size to total:
			mMsgTotalSize += mMsgSizes[i];
			mNewMsgCount++;
		}
		// skip to next line:
		if (!(p = strstr( p, "\r\n")))
			throw BM_network_error( BString("answer to LIST has unknown format, msg ") << i+1);
		p += 2;
	}
	if (mNewMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no new messages found, nothing more to do
	}
*/
}

/*------------------------------------------------------------------------------*\
	Data()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::Data() {
/*
	int32 newMailIndex = 0;
	for( int32 i=0; i<mMsgCount; i++) {
		if (mSmtpAccount->IsUIDDownloaded( mMsgUIDs[i]))
			continue;							// msg is old (according to unknown UID)
		BString cmd = BString("RETR ") << i+1;
		SendCommand( cmd);
		if (!CheckForPositiveAnswer( MULTI_LINE, ++newMailIndex))
			return;
		BmMail mail( mAnswer, Name());
		if (mail.InitCheck() != B_OK || !mail.Store())
			return;
		mSmtpAccount->MarkUIDAsDownloaded( mMsgUIDs[i]);
		//	delete the retrieved message if required:
		if (mSmtpAccount->DeleteMailFromServer()) {
			cmd = BString("DELE ") << i+1;
			SendCommand( cmd);
			if (!CheckForPositiveAnswer( SINGLE_LINE))
				return;
		}
	}
	if (mNewMsgCount)
		UpdateMailStatus( 100.0, "done", mNewMsgCount);
*/
}

/*------------------------------------------------------------------------------*\
	Disconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmSmtp::Disconnect() {
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
void BmSmtp::Quit( bool WaitForAnswer) {
	BString cmd("QUIT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer) {
			GetAnswer();
		}
	} catch(...) {	}
	mSmtpServer.Close();
	mConnected = false;
}

/*------------------------------------------------------------------------------*\
	CheckForPositiveAnswer( SingleLineMode, mailNr)
		-	waits for an answer from server and checks if it is positive
		-	throws an exception if answer is negative
		-	parameters are just passed on
\*------------------------------------------------------------------------------*/
bool BmSmtp::CheckForPositiveAnswer() {
	if (GetAnswer()) {
		if (mAnswer[0] > '3') {
			BString err("Server answers: \n");
			err += mAnswer;
			err.RemoveAll( "\r");
			throw BM_network_error( err);
		}
		return true;
	} else {
		// user has interrupted
		return false;
	}
}

/*------------------------------------------------------------------------------*\
	GetAnswer()
		-	waits for an answer from server and stores it in mAnswer
\*------------------------------------------------------------------------------*/
bool BmSmtp::GetAnswer() {
	int32 offset = 0;
	int32 SMALL = 128;
	int32 bufSize = BmSmtp::NetBufSize;
	char *buffer;
	bool done = false;
	bool firstBlock = true;
	int32 numBytes = 0;

	BM_LOG3( BM_LogSmtp, BString("bufSize:") << bufSize);
	mAnswer.SetTo( '\0', bufSize);		// preallocate the bufsize we need
	buffer = mAnswer.LockBuffer( 0);
	try {
		do {
			int32 bufFree = bufSize - offset;
			if (bufFree < SMALL) {
				// bufsize is not sufficient, we enlarge the buffer:
				bufSize *= 2;
				mAnswer.UnlockBuffer( offset);
				buffer = mAnswer.LockBuffer( bufSize);
				bufFree = bufSize - offset;
				BM_LOG2( BM_LogSmtp, BString("bufSize enlarged to:") << bufSize);
			}
			if (bufFree > BmSmtp::NetBufSize)
				bufFree = BmSmtp::NetBufSize;
			numBytes = ReceiveBlock( buffer+offset, bufFree);
			// we check if we have reached the end:
			int32 searchOffset = (offset > 0 ? offset-1 : 0);
			char *endp;
			while ( (endp=strstr( buffer+searchOffset, "\r\n")) != 0) {
				if (buffer+offset+numBytes >= endp +2/*\r\n*/ +3/*250*/
				&& endp[2+3] != '-')
					// end of answer is indicated by line starting with three digits, NOT followed
					// by a minus ('-'):
				BM_LOG2( BM_LogSmtp, BString("<--\n") << buffer);
				done = true;
			}
			offset += numBytes;
			firstBlock = false;
		} while( !done && numBytes);
		mAnswer.UnlockBuffer( -1);
		if (!done) {
			//	numBytes == 0, interrupt by external event (user)
			mAnswer = "";
		}
	} catch (...) {
		mAnswer.UnlockBuffer( -1);
		throw;
	}
	return done;
}

/*------------------------------------------------------------------------------*\
	ReceiveBlock( buffer, max)
		-	receives a block of a specified size (<=max) from server and appends
			it to mAnswer (well, writes it into mAnswer's buffer)
		-	ensures user-feedback is not blocked longer than BmSmtp::FeedbackTimeout
		-	waits only BmSmtp::ReceiveTimeout seconds for answer,
			throws an exception if no answer has arrived within that timeframe
		-	returns size of received block in bytes
\*------------------------------------------------------------------------------*/
int32 BmSmtp::ReceiveBlock( char* buffer, int32 max) {
	int32 numBytes;
	int32 AnswerTimeout = ThePrefs->GetInt("ReceiveTimeout")*1000*1000;
	int32 timeout = AnswerTimeout / BmSmtp::FeedbackTimeout;
	bool shouldCont;
	for( int32 round=0; (shouldCont = ShouldContinue()) && round<timeout; ++round) {
		if (mSmtpServer.IsDataPending( BmSmtp::FeedbackTimeout)) {
			if ((numBytes = mSmtpServer.Receive( buffer, max-1)) > 0) {
				buffer[numBytes] = '\0';
				return numBytes;
			} else if (numBytes < 0) {
				throw BM_network_error( "error during receive");
			}
		}
	}
	if (shouldCont) {
		throw BM_network_error( "timeout during receive from SMTP-server");
	}
	return 0;
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	sends the specified SMTP-command to the server.
\*------------------------------------------------------------------------------*/
void BmSmtp::SendCommand( BString cmd) {
	cmd << "\r\n";
	int32 size = cmd.Length(), sentSize;
	if (cmd.IFindFirst("PASS") != B_ERROR) {
		BM_LOG( BM_LogSmtp, "-->\nPASS password_omitted_here");
													// we do not want to log the password...
	} else {
		BM_LOG( BM_LogSmtp, BString("-->\n") << cmd);
	}
	if ((sentSize = mSmtpServer.Send( cmd.String(), size)) != size) {
		throw BM_network_error( BString("error during send, sent only ") << sentSize << " bytes instead of " << size);
	}
}

/*------------------------------------------------------------------------------*\
	Initialize statics:
\*------------------------------------------------------------------------------*/
int32 BmSmtp::FeedbackTimeout = 200*1000;

