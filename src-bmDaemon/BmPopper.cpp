/*
	BmPopper.cpp		-	$Id$
*/

#include <malloc.h>
#include <stdio.h>

#include <Alert.h>

#include "BmPopper.h"

int32 BmPopper::ReceiveTimeout = 10*1000*1000;
int32 BmPopper::PasswordTimeout = 20*1000*1000;

//-------------------------------------------------
int32 BmPopper::NewPopper( void* data) {
	try {
		BmPopper popper( (BmPopperInfo*)data);
		popper.Start();
		return 0;
	} catch( exception &e) {
		BAlert *alert = new BAlert( NULL, e.what(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return 10;
	}
}

//-------------------------------------------------
BmPopper::PopState BmPopper::PopStates[BmPopper::POP_FINAL] = {
	PopState( "connect", &BmPopper::Connect),
	PopState( "login", &BmPopper::Login),
	PopState( "check for new mail", &BmPopper::Check),
	PopState( "get messages", &BmPopper::Retrieve),
	PopState( "update", &BmPopper::Update),
	PopState( "disconnect", &BmPopper::Disconnect)
};

//-------------------------------------------------
BmPopper::BmPopper( BmPopperInfo* info)
	: mPopperInfo( info)
	, mPopServer()
	, mState( 0)
	, mDataPtr( NULL)
	, mConnected( false)
	, mMsgUIDs( NULL)
	, mMsgCount( 0)
	, log( info->name)
{
}

//-------------------------------------------------
BmPopper::~BmPopper() { 
	if (mConnected)
		this->Quit();
	if (mDataPtr)
		free( mDataPtr);
	if (mMsgUIDs)
		delete [] mMsgUIDs;
	BLooper* looper = mPopperInfo->statusLooper;
	if (looper) {
		BMessage *msg = new BMessage( BM_POP_DONE);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
		looper->PostMessage( msg);
		delete msg;
	}
	delete mPopperInfo->account;
	delete mPopperInfo;
}

//-------------------------------------------------
void BmPopper::Start() {
	static const float delta = (100.0 / POP_DISCONNECT);

	if (mPopServer.InitCheck() != B_OK) {
		throw runtime_error("BmPopper: could not create NetEndpoint");
	}
	try {
		for( mState=POP_CONNECT; ShouldContinue() && mState<POP_FINAL; ++mState) {
			TStateMethod stateFunc = PopStates[mState].func;
			UpdateStateBar( (mState==POP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
snooze( 200*1000);
		}
	}
	catch( runtime_error &err) {
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

//-------------------------------------------------
bool BmPopper::ShouldContinue() {
	return (!mPopperInfo->aliveFunc || (*mPopperInfo->aliveFunc)());
}
	
//-------------------------------------------------
void BmPopper::UpdateStateBar( const float delta, const char* trailingText) {
	if (mPopperInfo->statusLooper) {
		BMessage *msg = new BMessage( BM_POP_UPDATE_STATE);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
		msg->AddFloat( MSG_DELTA, delta);
		msg->AddString( MSG_LEADING, PopStates[mState].text);
		if (trailingText)
			msg->AddString( MSG_TRAILING, trailingText);
		mPopperInfo->statusLooper->PostMessage( msg);
		delete msg;
	}
}

//-------------------------------------------------
void BmPopper::UpdateMailBar( const float delta, const char* trailingText, 
										int32 curr, int32 max) {
	if (mPopperInfo->statusLooper) {
		char text[40];
		if (max) {
			sprintf( text, "%ld of %ld", curr, max);
		} else {
			sprintf( text, "none");
		}
		BMessage *msg = new BMessage( BM_POP_UPDATE_MAILS);
		msg->AddString( MSG_POPPER, mPopperInfo->name);
		msg->AddFloat( MSG_DELTA, delta);
		msg->AddString( MSG_LEADING, text);
		if (trailingText)
			msg->AddString( MSG_TRAILING, trailingText);
		mPopperInfo->statusLooper->PostMessage( msg);
		delete msg;
	}
}

//-------------------------------------------------
void BmPopper::Connect() {
	if (mPopServer.Connect( mPopperInfo->account->POPAddress()) != B_OK) {
		BString s("Could not connect to POP-Server ");
		s << mPopperInfo->account->POPServer().String();
		throw network_error( s);
	}
	mConnected = true;
	StoreAnswer( CheckForPositiveAnswer( SINGLE_LINE));
}

//-------------------------------------------------
void BmPopper::Login() {
	BString cmd = BString("USER ") << mPopperInfo->account->Username();
	SendCommand( cmd);
	StoreAnswer( CheckForPositiveAnswer( SINGLE_LINE));
	cmd = BString("PASS ") << mPopperInfo->account->Password();
	SendCommand( cmd);
	StoreAnswer( CheckForPositiveAnswer( SINGLE_LINE, BmPopper::PasswordTimeout));
}

//-------------------------------------------------
void BmPopper::Check() {
	BString cmd("STAT");
	SendCommand( cmd);
	StoreAnswer( CheckForPositiveAnswer( SINGLE_LINE));
	if (sscanf( mDataPtr+4, "%ld", &mMsgCount) != 1 || mMsgCount < 0)
		throw network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0)
		return;
	mMsgUIDs = new BString[mMsgCount];
	cmd = BString("UIDL");
	SendCommand( cmd);
	StoreAnswer( GetAnswer( MULTI_LINE));
	if (*mDataPtr == '+') {
		int32 msgNum;
		char msgUID[100];
		char *p = strstr( mDataPtr, "\r\n");
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

//-------------------------------------------------
void BmPopper::Retrieve() {
	static const float delta = (100.0 / mMsgCount);
	for( int32 i=0; i<mMsgCount; i++) {
		UpdateMailBar( delta, NULL, i+1, mMsgCount);
		BString cmd = BString("RETR ") << i+1;
		SendCommand( cmd);
		StoreAnswer( CheckForPositiveAnswer( MULTI_LINE));
snooze( 200*1000);
	}
}

//-------------------------------------------------
void BmPopper::Update() {
}

//-------------------------------------------------
void BmPopper::Disconnect() {
	Quit( true);
}

//-------------------------------------------------
void BmPopper::Quit( bool WaitForAnswer) {
	BString cmd("QUIT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer) {
			StoreAnswer( GetAnswer( SINGLE_LINE));
		}
	} catch(...) {	}
	mPopServer.Close();
	mConnected = false;
}

//-------------------------------------------------
void BmPopper::StoreAnswer( char* buf) {
	if (mDataPtr) 
		free( mDataPtr);
	mDataPtr = buf;
}

//-------------------------------------------------
char* BmPopper::CheckForPositiveAnswer( bool SingleLineMode, int32 AnswerTimeout) {
	char* answer = GetAnswer( SingleLineMode, AnswerTimeout);
	if (*answer != '+') {
		BString err("Server answers: \n");
		err += answer;
		err.RemoveSet("\r");
		free( answer);
		throw network_error( err);
	}
	return answer;
}

//-------------------------------------------------
char* BmPopper::GetAnswer( bool SingleLineMode, int32 AnswerTimeout) {
	int32 bufSize = 4096;
	const int32 SMALL = 512;
	const int32 HUGE = 1024*1024;		// One Megabyte of POP-reply is too much!
	char *buffer = static_cast<char*>(malloc( bufSize));
	int32 offset = 0;
	bool done = false;
	try {
		if (!buffer) {
			throw runtime_error("BmPopper: could not get memory via malloc()");
		}
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
			int32 numBytes = ReceiveBlock( bufPos, bufFree, AnswerTimeout);
			offset += numBytes;
			if (SingleLineMode || offset==0 && *bufPos=='-') {
				if (strstr( bufPos, "\r\n")) {
					done = true;
				}
			} else {		// MULTI_LINE mode
				if (strstr( bufPos, "\r\n.\r\n")) {
					done = true;
				}
			}
		} while( !done);
		return buffer;
	}
	catch (...) {
		free( buffer);
		throw;
	}
}

//-------------------------------------------------
int32 BmPopper::ReceiveBlock( char* buffer, int32 max, int32 AnswerTimeout) {
	int32 numBytes;
	int32 timeout = AnswerTimeout / BmPopper::FeedbackTimeout;
	for( int32 round=0; ShouldContinue() && round<timeout; ++round) {
		if (mPopServer.IsDataPending( BmPopper::FeedbackTimeout)) {
			if ((numBytes = mPopServer.Receive( buffer, max-1)) > 0) {
				buffer[numBytes] = '\0';
				BmLOG( BString("<--\n") << buffer);
				return numBytes;
			} else if (numBytes < 0) {
				throw network_error( "error during receive");
			}
		}
	}
	throw network_error( "timeout during receive from POP-server");
}

//-------------------------------------------------
void BmPopper::SendCommand( BString &cmd) {
	cmd += "\r\n";
	int32 size = cmd.Length();
	if (cmd.FindFirst("PASS") == 0)
		BmLOG( "-->\nPASS password_omitted_here");
	else
		BmLOG( BString("-->\n") << cmd);
	if (mPopServer.Send( cmd.String(), size) != size) {
		throw network_error( "error during send");
	}
}

//-------------------------------------------------
int32 BmPopper::mId = 0;
int32 BmPopper::FeedbackTimeout = 200*1000;

