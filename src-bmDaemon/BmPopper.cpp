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
	try {
		BmPopperInfo* pInfo = static_cast<BmPopperInfo*>(data);
		if (pInfo) {
			BmPopper popper( pInfo);
			popper.Start();
		} else
			throw "NewPopper(): No valid BmPopperInfo* given!";
		return 0;
	} catch( exception &e) {
		BAlert *alert = new BAlert( NULL, e.what(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
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
	PopState( "disconnect", &BmPopper::Disconnect)
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
	if (looper) {
		// tell interested party that we are done:
		BMessage *msg = new BMessage( BM_POP_DONE);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
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
		BString errstr( err.what());
		int e;
		if ((e = mPopServer.Error()))
			errstr << "\nerror: " << e << ", " << mPopServer.ErrorStr();
		BmLOG( BString("Error: ") << errstr.String());
		BAlert *alert = new BAlert( NULL, errstr.String(), "OK", NULL, NULL, 
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
	UpdatePOPStatus( delta, detailText)
		- informs the interested party about a change in the current POP3-state
  ------------------------------------------------------------------------------*/
void BmPopper::UpdatePOPStatus( const float delta, const char* detailText) {
	if (mPopperInfo->statusLooper) {
		BMessage *msg = new BMessage( BM_POP_UPDATE_STATE);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
		msg->AddFloat( MSG_DELTA, delta);
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
	if (mPopperInfo->statusLooper) {
		char text[40];
		if (mMsgCount) {
			sprintf( text, "%ld of %ld", currMsg, mMsgCount);
		} else {
			sprintf( text, "none");
		}
		BMessage *msg = new BMessage( BM_POP_UPDATE_MAILS);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
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
		BString s("Could not connect to POP-Server ");
		s << mPopperInfo->account->POPServer().String();
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
	if (sscanf( mAnswer.c_str()+4, "%ld %ld", &mMsgCount, &mMsgTotalSize) != 2 || mMsgCount < 0)
		throw network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no messages found, nothing more to do
	}

	// we try to fetch a list of unique message IDs from server:
	mMsgUIDs = new BString[mMsgCount];
	cmd = BString("UIDL");
	SendCommand( cmd);
	GetAnswer( MULTI_LINE);
	if (mAnswer[0] == '+') {
		int32 msgNum;
		char msgUID[100];
		char *p = strstr( mAnswer.c_str(), "\r\n");
		if (!p)
			return;
		p += 2;
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
	
  ------------------------------------------------------------------------------*/
void BmPopper::Retrieve() {
	int32 num;
	for( int32 i=0; i<mMsgCount; i++) {
		BString cmd = BString("LIST ") << i+1;
		SendCommand( cmd);
		CheckForPositiveAnswer( SINGLE_LINE);
		if (sscanf( mAnswer.c_str()+4, "%ld %ld", &num, &mMsgSize) != 2 || num != i+1)
		throw network_error( "answer to LIST has unknown format");
		cmd = BString("RETR ") << i+1;
		SendCommand( cmd);
		CheckForPositiveAnswer( MULTI_LINE, i+1);
snooze( 200*1000);
	}
}

/*------------------------------------------------------------------------------
	
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
	
  ------------------------------------------------------------------------------*/
void BmPopper::Disconnect() {
	Quit( true);
}

/*------------------------------------------------------------------------------
	
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
	
  ------------------------------------------------------------------------------*/
void BmPopper::CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr) {
	GetAnswer( SingleLineMode, mailNr);
	if (mAnswer[0] != '+') {
		BString err("Server answers: \n");
		err += mAnswer.c_str();
		err.RemoveSet("\r");
		throw network_error( err);
	}
}

/*------------------------------------------------------------------------------
	
  ------------------------------------------------------------------------------*/
void BmPopper::GetAnswer( bool SingleLineMode, int32 mailNr) {
	int32 bufSize = 4096;
	const int32 SMALL = 512;
	const int32 HUGE = 32*1024*1024;		// more than 32 MB of POP-reply is too much!
	int32 offset = 0;
	mAnswer = "";
	bool done = false;
	char *buffer = static_cast<char*>(malloc( bufSize));
	if (!buffer)
		throw runtime_error("BmPopper: could not get memory via malloc()");
	try {
		bool firstBlock = true;
		do {
			int32 bufFree = bufSize-offset;
			if (bufFree < SMALL) {
				if (bufSize >= HUGE) {
					throw network_error("BmPopper: reply from server too huge (>1MB)");
				}
				bufFree += bufSize;
				bufSize *= 2;
				if (!(buffer = static_cast<char*>(realloc( buffer, bufSize)))) {
					throw runtime_error("BmPopper: could not get memory via realloc()");
				}
			}
			char *bufPos = &buffer[offset];
			int32 numBytes = ReceiveBlock( bufPos, bufFree);
			offset += numBytes;
			if (SingleLineMode || firstBlock && *buffer=='-') {
				if (strstr( bufPos, "\r\n")) {
					done = true;
				}
			} else {		// MULTI_LINE mode
				if (strstr( bufPos, "\r\n.\r\n")) {
					done = true;
				}
			}
			firstBlock = false;
			if (mailNr > 0) {
				float delta = (100.0 * numBytes) / (mMsgTotalSize ? mMsgTotalSize : 1);
				BString text = BString("(") << BytesToString( mMsgSize).c_str() << ")  ";
				UpdateMailStatus( delta, text.String(), mailNr);
			}
		} while( !done);
		mAnswer = buffer;
		free( buffer);
		buffer = 0;
	}
	catch (...) {
		if (buffer)
			free( buffer);
		throw;
	}
}

/*------------------------------------------------------------------------------
	
  ------------------------------------------------------------------------------*/
int32 BmPopper::ReceiveBlock( char* buffer, int32 max) {
	int32 numBytes;
	int32 AnswerTimeout = Beam::Prefs->ReceiveTimeout()*1000*1000;
	int32 timeout = AnswerTimeout / BmPopper::FeedbackTimeout;
	for( int32 round=0; ShouldContinue() && round<timeout; ++round) {
		if (mPopServer.IsDataPending( BmPopper::FeedbackTimeout)) {
			if ((numBytes = mPopServer.Receive( buffer, max-1)) > 0) {
				buffer[numBytes] = '\0';
//BmLOG( BString("<--\n") << buffer);
				return numBytes;
			} else if (numBytes < 0) {
				throw network_error( "error during receive");
			}
		}
	}
	throw network_error( "timeout during receive from POP-server");
}

/*------------------------------------------------------------------------------
	Sends the specified POP3-command to the server.
  ------------------------------------------------------------------------------*/
void BmPopper::SendCommand( BString &cmd) {
	cmd += "\r\n";
	int32 size = cmd.Length(), sentSize;
	if (cmd.FindFirst("PASS") == 0) {
		BmLOG( "-->\nPASS password_omitted_here");
													// we do not want to log the password...
	} else {
		BmLOG( BString("-->\n") << cmd);
	}
	if ((sentSize = mPopServer.Send( cmd.String(), size)) != size) {
		throw network_error( BString("error during send, sent only ") << sentSize << " bytes instead of " << size);
	}
}

//-------------------------------------------------
int32 BmPopper::mId = 0;
int32 BmPopper::FeedbackTimeout = 200*1000;

