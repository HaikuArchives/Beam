/*
	BmPopper.cpp
		- Implements the main POP3-client-class: BmPopper

		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <memory.h>
#include <memory>
#include <stdio.h>

#include "md5.h"

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

/*------------------------------------------------------------------------------*\
	PopStates[]
		-	array of POP3-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmPopper::PopState BmPopper::PopStates[BmPopper::POP_FINAL] = {
	PopState( "connect...", &BmPopper::Connect),
	PopState( "login...", &BmPopper::Login),
	PopState( "check...", &BmPopper::Check),
	PopState( "get...", &BmPopper::Retrieve),
	PopState( "quit...", &BmPopper::Disconnect),
	PopState( "done", NULL)
};

/*------------------------------------------------------------------------------*\
	BmPopper( info)
		-	contructor
\*------------------------------------------------------------------------------*/
BmPopper::BmPopper( const BmString& name, BmPopAccount* account)
	:	BmJobModel( name)
	,	mPopAccount( account)
	,	mPopServer( NULL)
	,	mConnected( false)
	,	mMsgUIDs( NULL)
	,	mMsgCount( 0)
	,	mNewMsgCount( 0)
	,	mMsgSizes( NULL)
	,	mMsgTotalSize( 1)
	,	mState( 0)
	,	mPwdAcquisitorFunc( NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmPopper()
		-	destructor
		-	frees all associated memory (hopefully)
\*------------------------------------------------------------------------------*/
BmPopper::~BmPopper() { 
	if (mConnected) {
		//	We try to inform POP-server about QUIT, if still connected.
		// This probably means that we ran into an exception, so maybe it's not really
		// a good idea...(?)
		Quit();
	}
	TheLogHandler->FinishLog( BM_LOGNAME);
	if (mMsgSizes)
		delete [] mMsgSizes;
	if (mMsgUIDs)
		delete [] mMsgUIDs;
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the Popper should continue to run
		-	in addition to the inherited behaviour, the Popper should continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmPopper::ShouldContinue() {
	return inherited::ShouldContinue() 
			 || CurrentJobSpecifier() == BM_AUTH_ONLY_JOB
			 || CurrentJobSpecifier() == BM_CHECK_AUTH_TYPES_JOB;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all POP3-stages and calls the corresponding 
			handlers
		-	returns whether or not the Popper has completed it's job
\*------------------------------------------------------------------------------*/
bool BmPopper::StartJob() {
	const float delta = (100.0 / POP_DONE);
	try {
		for( mState=POP_CONNECT; ShouldContinue() && mState<POP_DONE; ++mState) {
			TStateMethod stateFunc = PopStates[mState].func;
			UpdatePOPStatus( (mState==POP_CONNECT ? 0.0 : delta), NULL);
			(this->*stateFunc)();
			if (CurrentJobSpecifier() == BM_AUTH_ONLY_JOB && mState==POP_LOGIN)
				return true;
			if (CurrentJobSpecifier() == BM_CHECK_AUTH_TYPES_JOB && mState==POP_CONNECT)
				return true;
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
		if ((e = mPopServer->Error()))
			errstr << "\nerror: " << e << ", " << mPopServer->ErrorStr();
		UpdatePOPStatus( 0.0, NULL, true);
		BmString text = Name() << "\n\n" << errstr;
		HandleError( BmString("BmPopper: ") << text);
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
	msg->AddString( MSG_POPPER, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed)
		msg->AddString( MSG_TRAILING, (BmString(PopStates[mState].text) << " FAILED!").String());
	else if (stopped)
		msg->AddString( MSG_TRAILING, (BmString(PopStates[mState].text) << " Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, PopStates[mState].text);
	if (detailText)
		msg->AddString( MSG_LEADING, detailText);
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
	msg->AddString( MSG_POPPER, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "mailbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text.String());
	if (detailText)
		msg->AddString( MSG_TRAILING, detailText);
	TellControllers( msg.get());
}

/*------------------------------------------------------------------------------*\
	Connect()
		-	Initiates network-connection to POP-server
\*------------------------------------------------------------------------------*/
void BmPopper::Connect() {
	BNetAddress addr;
	delete mPopServer;
	mPopServer = new BNetEndpoint;
	mPopServer->InitCheck() == B_OK		||	BM_THROW_RUNTIME("BmPopper: could not create NetEndpoint");
	if (!mPopAccount->GetPOPAddress( &addr)) {
		BmString s = BmString("Could not determine address of POP-Server ") << mPopAccount->POPServer();
		throw BM_network_error( s);
	}
	status_t err;
	if ((err=mPopServer->Connect( addr)) != B_OK) {
		BmString s = BmString("Could not connect to POP-Server ") << mPopAccount->POPServer() << "\n\bError:\n\t"<<strerror(err);
		throw BM_network_error( s);
	}
	mConnected = true;
	CheckForPositiveAnswer( SINGLE_LINE);
	Regexx rx;
	if (rx.exec( mReplyLine, "(<.+?>)\\s*$", Regexx::newline)) {
		mServerTimestamp = rx.match[0];
	}
}

/*------------------------------------------------------------------------------*\
	SuggestAuthType()
		-	looks at the auth-types supported by the server and selects the most secure
			of those that is supported by Beam.
\*------------------------------------------------------------------------------*/
BmString BmPopper::SuggestAuthType() const {
	if (mServerTimestamp.Length())
		return BmPopAccount::AUTH_APOP;
	else
		return BmPopAccount::AUTH_POP3;
}

/*------------------------------------------------------------------------------*\
	Login()
		-	Sends user/passwd combination and checks result
		-	currently supports POP3- & APOP-authentication
\*------------------------------------------------------------------------------*/
void BmPopper::Login() {
	BmString pwd;
	bool pwdOK = false;
	bool first = true;
	BmString authMethod = mPopAccount->AuthMethod();
	authMethod.ToUpper();
	while(!pwdOK) {
		bool pwdGiven = false;
		if (first && mPopAccount->PwdStoredOnDisk()) {
			// use stored password:
			pwd = mPopAccount->Password();
			pwdGiven = true;
		} else if (mPwdAcquisitorFunc) {
			// ask user about password:
			pwdGiven = mPwdAcquisitorFunc( Name(), pwd);
		}
		if (!pwdGiven) {
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
				SendCommand( cmd+Digest);
			} else
				BM_THROW_RUNTIME( "Server did not supply a timestamp, so APOP doesn't work.");
		} else {
			// authMethod == AUTH_POP3: send username and password as plain text:
			BmString cmd = BmString("USER ") << mPopAccount->Username();
			SendCommand( cmd);
			if (CheckForPositiveAnswer( SINGLE_LINE)) {
				SendCommand( "PASS ", pwd);
			}
		}
		try {
			pwdOK = CheckForPositiveAnswer( SINGLE_LINE);
		} catch( BM_network_error &err) {
			// most probably a wrong password...
			BmString errstr = err.what();
			int e;
			if ((e = mPopServer->Error()))
				errstr << "\nerror: " << e << ", " << mPopServer->ErrorStr();
			BmString text = Name() << "\n\n" << errstr;
			HandleError( BmString("BmPopper: ") << text);
		}
	}
}

/*------------------------------------------------------------------------------*\
	Check()
		-	looks for new mail
\*------------------------------------------------------------------------------*/
void BmPopper::Check() {
	int32 msgNum = 0;

	BmString cmd("STAT");
	SendCommand( cmd);
	if (!CheckForPositiveAnswer( SINGLE_LINE))
		return;
	if (sscanf( mReplyLine.String()+4, "%ld", &mMsgCount) != 1 || mMsgCount < 0)
		throw BM_network_error( "answer to STAT has unknown format");
	if (mMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no messages found, nothing more to do
	}

	delete [] mMsgUIDs;
	mMsgUIDs = new BmString[mMsgCount];
	delete [] mMsgSizes;
	mMsgSizes = new int32[mMsgCount];
	// we try to fetch a list of unique message IDs from server:
	cmd = BmString("UIDL");
	SendCommand( cmd);
	// The UIDL-command may not be implemented by this server, so we 
	// do not require a positive answer, we just hope for it:
	if (!GetAnswer( MULTI_LINE)) 			
		return;									// interrupted, we give up
	if (mReplyLine[0] == '+') {
		// ok, we've got the UIDL-listing, so we fetch it:
		char msgUID[128];
		// fetch UIDLs one per line and store them in array:
		const char *p = mAnswer.String();
		for( int32 i=0; i<mMsgCount; ++i) {
			if (sscanf( p, "%ld %80s", &msgNum, msgUID) != 2 || msgNum <= 0)
				throw BM_network_error( BmString("answer to UIDL has unknown format, msg ") << i+1);
			mMsgUIDs[i] = msgUID;
			// skip to next line:
			if (!(p = strstr( p, "\r\n")))
				throw BM_network_error( BmString("answer to UIDL has unknown format, msg ") << i+1);
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
		int32 msgSize;
		if (!mPopAccount->IsUIDDownloaded( mMsgUIDs[i])) {
			// msg is new (according to unknown UID)
			// fetch msgsize for message...
			if (sscanf( p, "%ld %ld", &msgNum, &msgSize) != 2 || msgNum != i+1)
				throw BM_network_error( BmString("answer to LIST has unknown format, msg ") << i+1);
			// add msg-size to total:
			mMsgTotalSize += msgSize;
			mMsgSizes[mNewMsgCount++] = msgSize;
		}
		// skip to next line:
		if (!(p = strstr( p, "\r\n")))
			throw BM_network_error( BmString("answer to LIST has unknown format, msg ") << i+1);
		p += 2;
	}
	if (mNewMsgCount == 0) {
		UpdateMailStatus( 0, NULL, 0);
		return;									// no new messages found, nothing more to do
	}
}

/*------------------------------------------------------------------------------*\
	Retrieve()
		-	retrieves all new mails from server
\*------------------------------------------------------------------------------*/
void BmPopper::Retrieve() {
	int32 newMailIndex = 0;
	for( int32 i=0; i<mMsgCount; i++) {
		if (mPopAccount->IsUIDDownloaded( mMsgUIDs[i]))
			continue;							// msg is old (according to unknown UID)
		BmString cmd = BmString("RETR ") << i+1;
		SendCommand( cmd);
		if (!CheckForPositiveAnswer( MULTI_LINE, ++newMailIndex))
			return;
		BmMail mail( mAnswer, Name());
		if (mail.InitCheck() != B_OK || !mail.Store())
			return;
		mPopAccount->MarkUIDAsDownloaded( mMsgUIDs[i]);
		//	delete the retrieved message if required:
		if (mPopAccount->DeleteMailFromServer()) {
			cmd = BmString("DELE ") << i+1;
			SendCommand( cmd);
			if (!CheckForPositiveAnswer( SINGLE_LINE))
				return;
		}
	}
	if (mNewMsgCount)
		UpdateMailStatus( 100.0, "done", mNewMsgCount);
}

/*------------------------------------------------------------------------------*\
	Disconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmPopper::Disconnect() {
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
		if (WaitForAnswer) {
			GetAnswer( SINGLE_LINE);
		}
	} catch(...) {	}
	mPopServer->Close();
	mConnected = false;
}

/*------------------------------------------------------------------------------*\
	CheckForPositiveAnswer( SingleLineMode, mailNr)
		-	waits for an answer from server and checks if it is positive
		-	throws an exception if answer is negative
		-	parameters are just passed on
\*------------------------------------------------------------------------------*/
bool BmPopper::CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr) {
	if (GetAnswer( SingleLineMode, mailNr)) {
		if (mReplyLine[0] != '+') {
			BmString err("Server answers: \n");
			err += mReplyLine;
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
	GetAnswer( SingleLineMode, mailNr)
		-	waits for an answer from server and stores it in mAnswer
		-	mailNr > 0 if answer is a mail-message, mailNr==0 otherwise
		-	first line of answer (server-reply-line) is saved into mReplyLine.
			Because of this, the reply-line does not appear in mAnswer.
\*------------------------------------------------------------------------------*/
bool BmPopper::GetAnswer( bool SingleLineMode, int32 mailNr) {
	int32 offset = 0;
	int32 SMALL = 512;
	int32 netBufSize = ThePrefs->GetInt( "NetReceiveBufferSize", 65536);
	int32 bufSize = (mailNr>0 && mMsgSizes[mailNr-1] > netBufSize) 
							? mMsgSizes[mailNr-1]+SMALL*4
							: netBufSize;
	char *buffer;
	bool done = false;
	bool firstBlock = true;
	bool replyLineExtracted = false;
	int32 numBytes = 0;

	if (mailNr)
		BM_LOG2( BM_LogPop, BmString("announced msg-size:") << mMsgSizes[mailNr-1]);
	BM_LOG3( BM_LogPop, BmString("bufSize:") << bufSize);
	mAnswer.SetTo( '\0', bufSize);		// preallocate the bufsize we need
	buffer = mAnswer.LockBuffer( bufSize);
	try {
		do {
			int32 bufFree = bufSize - offset;
			if (bufFree < SMALL) {
				// bufsize is not sufficient, we enlarge the buffer:
				mAnswer.UnlockBuffer( bufSize);
				bufSize *= 2;
				buffer = mAnswer.LockBuffer( bufSize);
				bufFree = bufSize - offset;
				BM_LOG2( BM_LogPop, BmString("bufSize enlarged to:") << bufSize);
			}
			if (bufFree > netBufSize)
				bufFree = netBufSize;
			numBytes = ReceiveBlock( buffer+offset, bufFree);
			if (!replyLineExtracted) {
				// we may have to extract the reply-line from the buffer:
				char* eol;
				if ((eol=strstr( buffer, "\r\n")) != NULL) {
					// reply-line is complete, we extract it from buffer:
					*eol = 0;
					eol += 2;
					mReplyLine = buffer;
					int32 len = mReplyLine.Length();
					strcpy( buffer, eol);
					offset -= (len+2);
					replyLineExtracted = true;
					BM_LOG( BM_LogPop, BmString("<--\n") << mReplyLine);
					if (SingleLineMode || firstBlock && mReplyLine[0]=='-') {
						// if all we expect is the reply-line, or if answer is negative, we are done:
						done = true;
					}
				};
			}
			if (!SingleLineMode) {
				// MULTI_LINE mode
				int32 searchOffset = (offset > 3 ? offset-4 : 0);
				char *endp;
				if ((endp=strstr( buffer+searchOffset, "\r\n.\r\n"))) {
					*(endp+2)='\0';
					// end of multiline-answer is indicated by line consisting only of a dot
					if (!mailNr) {
						BM_LOG2( BM_LogPop, BmString("<--\n") << buffer);
					}
					done = true;
				}
			}
			offset += numBytes;
			firstBlock = false;
			if (mailNr > 0) {
				float delta = (100.0 * numBytes) / (mMsgTotalSize ? mMsgTotalSize : 1);
				BmString text = BmString("size: ") << BytesToString( mMsgSizes[mailNr-1]);
				UpdateMailStatus( delta, text.String(), mailNr);
			}
		} while( !done && numBytes);
		mAnswer.UnlockBuffer( offset);
		if (done) {
			mAnswer.ReplaceAll( "\n..", "\n.");
							// remove padding of "termination octet" (dot) inside message
		} else  {
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
		-	ensures user-feedback is not blocked longer than BmPopper::FeedbackTimeout
		-	waits only BmPopper::ReceiveTimeout seconds for answer,
			throws an exception if no answer has arrived within that timeframe
		-	returns size of received block in bytes
\*------------------------------------------------------------------------------*/
int32 BmPopper::ReceiveBlock( char* buffer, int32 max) {
	int32 numBytes;
	int32 AnswerTimeout = ThePrefs->GetInt("ReceiveTimeout")*1000*1000;
	int32 timeout = AnswerTimeout / BmPopper::FeedbackTimeout;
	bool shouldCont;
	for( int32 round=0; (shouldCont = ShouldContinue()) && round<timeout; ++round) {
		if (mPopServer->IsDataPending( BmPopper::FeedbackTimeout)) {
			if ((numBytes = mPopServer->Receive( buffer, max-1)) > 0) {
				buffer[numBytes] = '\0';
				return numBytes;
			} else if (numBytes < 0) {
				throw BM_network_error( "error during receive");
			}
		}
	}
	if (shouldCont) {
		throw BM_network_error( "timeout during receive from POP-server");
	}
	return 0;
}

/*------------------------------------------------------------------------------*\
	SendCommand( cmd)
		-	sends the specified POP3-command to the server.
\*------------------------------------------------------------------------------*/
void BmPopper::SendCommand( BmString cmd, BmString secret) {
	BmString command;
	if (secret.Length()) {
		command = cmd + secret;
		BM_LOG( BM_LogPop, BmString("-->\n") << cmd << " secret_data_omitted_here");
													// we do not want to log any passwords...
	} else {
		command = cmd;
		BM_LOG( BM_LogPop, BmString("-->\n") << cmd);
	}
	if (!command.Length() || command[command.Length()-1] != '\n')
		command << "\r\n";
	int32 size = command.Length();
	int32 sentSize;
	if ((sentSize = mPopServer->Send( command.String(), size)) != size) {
		throw BM_network_error( BmString("error during send, sent only ") << sentSize << " bytes instead of " << size);
	}
}

/*------------------------------------------------------------------------------*\
	Initialize statics:
\*------------------------------------------------------------------------------*/
int32 BmPopper::mId = 0;
int32 BmPopper::FeedbackTimeout = 200*1000;

