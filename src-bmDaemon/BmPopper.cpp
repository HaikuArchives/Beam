/*
	BmPopper.cpp
		- Implements the main POP3-client-class: BmPopper

		$Id$
*/

#include <stdio.h>

#include <Alert.h>

#include "BmPopper.h"

#undef LOGNAME
#define LOGNAME mPopperInfo->name

/*------------------------------------------------------------------------------
	NewPopper( data)
		-	creates a new popper-instance and starts it immediately
		-	data MUST contain a valid BmPopperInfo*
		-	this is used a thread-entry-function
  ------------------------------------------------------------------------------*/
int32 BmPopper::NewPopper( void* data) {
	BmPopperInfo* pInfo = static_cast<BmPopperInfo*>(data);
	try {
		if (pInfo) {
			BmPopper popper( pInfo);
			popper.Start();
			BmLOG_FINISH(pInfo->name);
		} else
			throw runtime_error("NewPopper(): No valid BmPopperInfo* given!");
		return 0;
	} catch( exception &e) {
		BString text = BString(pInfo->name) << "\n\n" << e.what();
		BAlert *alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		BmLOG_FINISH(pInfo->name);
		return 10;
	}
}

/*------------------------------------------------------------------------------
	PopStates[]
		-	array of POP3-states, each with title and corresponding handler-method
  ------------------------------------------------------------------------------*/
BmPopper::PopState BmPopper::PopStates[BmPopper::POP_FINAL] = {
	PopState( "connect", &BmPopper::Connect),
	PopState( "login", &BmPopper::Login),
	PopState( "check for new mail", &BmPopper::Check),
	PopState( "get messages", &BmPopper::Retrieve),
	PopState( "update", &BmPopper::Update),
	PopState( "done", &BmPopper::Disconnect)
};

/*------------------------------------------------------------------------------
	BmPopper( info)
		-	contructor
  ------------------------------------------------------------------------------*/
BmPopper::BmPopper( BmPopperInfo* info)
	: mPopperInfo( info)
	, mPopServer()
	, mConnected( false)
	, mMsgUIDs( NULL)
	, mMsgCount( 0)
	, mMsgSize( 0)
	, mMsgTotalSize( 1)
	, mState( 0)
{
}

/*------------------------------------------------------------------------------
	~BmPopper()
		-	destructor
		-	frees all associated memory (hopefully)
  ------------------------------------------------------------------------------*/
BmPopper::~BmPopper() { 
	if (mConnected) {
		//	We try to inform POP-server about QUIT, if still connected.
		// This probably means that we ran into an exception, so maybe it's not really
		// a good idea...(?)
		this->Quit();
	}
	if (mMsgUIDs)
		delete [] mMsgUIDs;
	BLooper* looper = mPopperInfo->statusLooper;
	if (looper && ShouldContinue()) {
		// tell interested party that we are done:
		BMessage *msg = new BMessage( BM_POP_DONE);
		msg->AddString( MSG_POPPER, mPopperInfo->name.String());
		looper->PostMessage( msg);
		delete msg;
	}
	delete mPopperInfo->account;
	delete mPopperInfo;
}

/*------------------------------------------------------------------------------
	Start()
		-	the mainloop, steps through all POP3-stages and calls the corresponding handlers
  ------------------------------------------------------------------------------*/
void BmPopper::Start() {
	const float delta = (100.0 / POP_DISCONNECT);
	const bool failed=true;

	if (mPopServer.InitCheck() != B_OK) {
		throw runtime_error("BmPopper: could not create NetEndpoint");
	}
	try {
		for( mState=POP_CONNECT; ShouldContinue() && mState<POP_FINAL; ++mState) {
			TStateMethod stateFunc = PopStates[mState].func;
			UpdatePOPStatus( (mState==POP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
snooze( 200*1000);
		}
	}
	catch( runtime_error &err) {
		// a problem occurred, we tell the user:
		BString errstr = err.what();
		int e;
		if ((e = mPopServer.Error()))
			errstr << "\nerror: " << e << ", " << mPopServer.ErrorStr();
		BmLOG( BString("Error: ") << errstr);
		UpdatePOPStatus( 0.0, NULL, failed);
		BString text = BString(mPopperInfo->name) << "\n\n" << errstr;
		BAlert *alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
	}
}

/*------------------------------------------------------------------------------
	ShouldContinue()
		-	determines if any "external event" requires the Popper to stop
		-	most likely, "external event" means that the user has closed the 
			connection-window or the whole app.
  ------------------------------------------------------------------------------*/
bool BmPopper::ShouldContinue() {
	return (!mPopperInfo->aliveFunc || (*mPopperInfo->aliveFunc)());
							// if no aliveFunc was specified, we continue
}
	
/*------------------------------------------------------------------------------
	UpdatePOPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current POP3-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BString "FAILED!" will be shown)
  ------------------------------------------------------------------------------*/
void BmPopper::UpdatePOPStatus( const float delta, const char* detailText, 
										  bool failed) {
	if (mPopperInfo->statusLooper && ShouldContinue()) {
		BMessage *msg = new BMessage( BM_POP_UPDATE_STATE);
		msg->AddString( MSG_POPPER, mPopperInfo->name.String());
		msg->AddFloat( MSG_DELTA, delta);
		if (failed)
			msg->AddString( MSG_LEADING, BString(PopStates[mState].text) << "   FAILED!");
		else
			msg->AddString( MSG_LEADING, PopStates[mState].text);
		if (detailText)
			msg->AddString( MSG_TRAILING, detailText);
		mPopperInfo->statusLooper->PostMessage( msg);
		delete msg;
	}
}

/*------------------------------------------------------------------------------
	UpdateMailStatus( delta, detailText)
		- informs the interested party about the message currently dealt with
  ------------------------------------------------------------------------------*/
void BmPopper::UpdateMailStatus( const float delta, const char* detailText, 
										int32 currMsg) {
	if (mPopperInfo->statusLooper && ShouldContinue()) {
		char text[40];
		if (mMsgCount) {
			sprintf( text, "%ld of %ld", currMsg, mMsgCount);
		} else {
			sprintf( text, "none");
		}
		BMessage *msg = new BMessage( BM_POP_UPDATE_MAILS);
		msg->AddString( MSG_POPPER, mPopperInfo->name.String());
		msg->AddFloat( MSG_DELTA, delta);
		msg->AddString( MSG_LEADING, text);
		if (detailText)
			msg->AddString( MSG_TRAILING, detailText);
		mPopperInfo->statusLooper->PostMessage( msg);
		delete msg;
	}
}

/*------------------------------------------------------------------------------
	Connect()
		-	Initiates network-connection to POP-server
  ------------------------------------------------------------------------------*/
void BmPopper::Connect() {
	if (mPopServer.Connect( mPopperInfo->account->POPAddress()) != B_OK) {
		BString s = BString("Could not connect to POP-Server ") << mPopperInfo->account->POPServer();
		throw network_error( s);
	}
	mConnected = true;
	CheckForPositiveAnswer( SINGLE_LINE);
}

/*------------------------------------------------------------------------------
	Login()
		-	Sends user/passwd combination and checks result
  ------------------------------------------------------------------------------*/
void BmPopper::Login() {
	BString cmd = BString("USER ") << mPopperInfo->account->Username();
	SendCommand( cmd);
	CheckForPositiveAnswer( SINGLE_LINE);
	cmd = BString("PASS ") << mPopperInfo->account->Password();
	SendCommand( cmd);
	CheckForPositiveAnswer( SINGLE_LINE);
}

/*------------------------------------------------------------------------------
	Check()
		-	looks for new mail
  ------------------------------------------------------------------------------*/
void BmPopper::Check() {
	BString cmd("STAT");
	SendCommand( cmd);
	CheckForPositiveAnswer( SINGLE_LINE);
	if (sscanf( mAnswer.String()+4, "%ld %ld", &mMsgCount, &mMsgTotalSize) != 2 || mMsgCount < 0)
		throw network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no messages found, nothing more to do
	}

	// we try to fetch a list of unique message IDs from server:
	mMsgUIDs = new BString[mMsgCount];
	cmd = BString("UIDL");
	SendCommand( cmd);
	// The UIDL-command may not be implemented by this server, so we 
	// do not require a postive answer, we just hope for it:
	GetAnswer( MULTI_LINE);
	if (mAnswer[0] == '+') {
		// ok, we've got the UIDL-listing, so we fetch it:
		int32 msgNum;
		char msgUID[100];
		// listing begins on second line, so we skip the first:
		char *p = strstr( mAnswer.String(), "\r\n");
		if (!p)
			return;
		p += 2;
		// fetch UIDLs one per line and store them in array:
		for( int32 i=0; i<mMsgCount; i++) {
			if (sscanf( p, "%ld %s", &msgNum, msgUID) != 2 || msgNum <= 0)
				throw network_error( "answer to UIDL has unknown format");
			mMsgUIDs[i] = msgUID;
			if (!(p = strstr( p, "\r\n")))
				return;
			p += 2;
		}
	}
}

/*------------------------------------------------------------------------------
	Retrieve()
		-	retrieves all mails from server
  ------------------------------------------------------------------------------*/
void BmPopper::Retrieve() {
	int32 num;
	for( int32 i=0; i<mMsgCount; i++) {
		BString cmd = BString("LIST ") << i+1;
		SendCommand( cmd);
		CheckForPositiveAnswer( SINGLE_LINE);
		if (sscanf( mAnswer.String()+4, "%ld %ld", &num, &mMsgSize) != 2 || num != i+1)
		throw network_error( "answer to LIST has unknown format");
		cmd = BString("RETR ") << i+1;
		SendCommand( cmd);
		CheckForPositiveAnswer( MULTI_LINE, i+1);
snooze( 200*1000);
	}
	UpdateMailStatus( 0, "done", mMsgCount);
	mAnswer.Truncate( BmPopper::NetBufSize, false);
}

/*------------------------------------------------------------------------------
	Update()
		-	deletes the retrieved messages if required
  ------------------------------------------------------------------------------*/
void BmPopper::Update() {
	if (mPopperInfo->account->DeleteMailFromServer()) {
		for( int32 i=0; i<mMsgCount; i++) {
			BString cmd = BString("DELE ") << i+1;
			SendCommand( cmd);
			CheckForPositiveAnswer( SINGLE_LINE);
snooze( 200*1000);
		}
	}
}

/*------------------------------------------------------------------------------
	Disconnect()
		-	tells the server that we are finished
  ------------------------------------------------------------------------------*/
void BmPopper::Disconnect() {
	Quit( true);
}

/*------------------------------------------------------------------------------
	Quit( WaitForAnswer)
		-	sends a QUIT to the server, waiting for answer only 
			if WaitForAnswer==true
		-	normally, we wait for an answer, just if we are shutting down
			because of an error we ignore any answer.
		-	the network-connection is always closed
  ------------------------------------------------------------------------------*/
void BmPopper::Quit( bool WaitForAnswer) {
	BString cmd("QUIT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer) {
			GetAnswer( SINGLE_LINE);
		}
	} catch(...) {	}
	mPopServer.Close();
	mConnected = false;
}

/*------------------------------------------------------------------------------
	CheckForPositiveAnswer( SingleLineMode, mailNr)
		-	waits for an answer from server and checks if it is positive
		-	throws an exception if answer is negative
		-	parameters are just passed on
  ------------------------------------------------------------------------------*/
void BmPopper::CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr) {
	GetAnswer( SingleLineMode, mailNr);
	if (mAnswer[0] != '+') {
		BString err("Server answers: \n");
		err += mAnswer;
		err.RemoveAll( "\r");
		throw network_error( err);
	}
}

/*------------------------------------------------------------------------------
	GetAnswer( SingleLineMode, mailNr)
		-	waits for an answer from server and stores it in mAnswer
		-	mailNr > 0 if answer is a mail-message, mailNr==0 otherwise
  ------------------------------------------------------------------------------*/
void BmPopper::GetAnswer( bool SingleLineMode, int32 mailNr) {
	int32 offset = 0;
	int32 SMALL = 512;
	int32 bufSize = (mailNr>0 && mMsgSize > BmPopper::NetBufSize) 
							? mMsgSize+SMALL+2
							: BmPopper::NetBufSize;
	char *buffer;
	bool done = false;
	bool firstBlock = true;
	BmLOG(BString("bufSize(1):") << bufSize);
	mAnswer.SetTo( '\0', bufSize);
	buffer = mAnswer.LockBuffer( 0);
	try {
		do {
			int32 bufFree = bufSize - offset;
			if (bufFree < SMALL) {
				bufSize *= 2;
				mAnswer.UnlockBuffer( offset);
				buffer = mAnswer.LockBuffer( bufSize);
				bufFree = bufSize - offset;
				BmLOG(BString("bufSize(2):") << bufSize);
			}
			if (bufFree > BmPopper::NetBufSize)
				bufFree = BmPopper::NetBufSize;
			int32 numBytes = ReceiveBlock( buffer+offset, bufFree);
			if (SingleLineMode || firstBlock && *buffer=='-') {
				int32 searchOffset = (offset > 0 ? offset-1 : offset);
				if (strstr( buffer+searchOffset, "\r\n")) {
					BmLOG( BString("<--\n") << buffer);
					done = true;
				}
			} else {		// MULTI_LINE mode
				int32 searchOffset = (offset > 3 ? offset-4 : offset);
				if (strstr( buffer+searchOffset, "\r\n.\r\n")) {
					if (!mailNr) {
						BmLOG( BString("<--\n") << buffer);
					}
					done = true;
				}
			}
			offset += numBytes;
			firstBlock = false;
			if (mailNr > 0) {
				float delta = (100.0 * numBytes) / (mMsgTotalSize ? mMsgTotalSize : 1);
				BString text = BString("size: ") << BytesToString( mMsgSize);
				UpdateMailStatus( delta, text.String(), mailNr);
			}
		} while( !done);
		mAnswer.UnlockBuffer( -1);
	} catch (...) {
		mAnswer.UnlockBuffer( -1);
		throw;
	}
}

/*------------------------------------------------------------------------------
	ReceiveBlock( buffer, max)
		-	receives a block of a specified size (<=max) from server and appends
			it to mAnswer
		-	ensures user-feedback is not blocked longer than BmPopper::FeedbackTimeout
		-	waits only BmPopper::ReceiveTimeout seconds for answer,
			throws an exception if no answer has arrived within that timeframe
		-	returns size of received block in bytes
  ------------------------------------------------------------------------------*/
int32 BmPopper::ReceiveBlock( char* buffer, int32 max) {
	int32 numBytes;
	int32 AnswerTimeout = Beam::Prefs->ReceiveTimeout()*1000*1000;
	int32 timeout = AnswerTimeout / BmPopper::FeedbackTimeout;
	bool shouldCont;
	for( int32 round=0; (shouldCont = ShouldContinue()) && round<timeout; ++round) {
		if (mPopServer.IsDataPending( BmPopper::FeedbackTimeout)) {
			if ((numBytes = mPopServer.Receive( buffer, max-1)) > 0) {
				buffer[numBytes] = '\0';
				return numBytes;
			} else if (numBytes < 0) {
				throw network_error( "error during receive");
			}
		}
	}
	if (shouldCont) {
		throw network_error( "timeout during receive from POP-server");
	}
	return 0;
}

/*------------------------------------------------------------------------------
	SendCommand( cmd)
		-	sends the specified POP3-command to the server.
  ------------------------------------------------------------------------------*/
void BmPopper::SendCommand( BString cmd) {
	cmd << "\r\n";
	int32 size = cmd.Length(), sentSize;
	if (cmd.IFindFirst("PASS") != B_ERROR) {
		BmLOG( "-->\nPASS password_omitted_here");
													// we do not want to log the password...
	} else {
		BmLOG( BString("-->\n") << cmd);
	}
	if ((sentSize = mPopServer.Send( cmd.String(), size)) != size) {
		throw network_error( BString("error during send, sent only ") << sentSize << " bytes instead of " << size);
	}
}

/*------------------------------------------------------------------------------
	Initialize statics:
  ------------------------------------------------------------------------------*/
int32 BmPopper::mId = 0;
int32 BmPopper::FeedbackTimeout = 200*1000;

