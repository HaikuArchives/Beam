/*
	BmSmtp.cpp
		- Implements the main SMTP-client-class: BmSmtp

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

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmNetUtil.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmSmtpAccount.h"
#include "BmSmtp.h"
#include "BmPrefs.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME (BString("SMTP_")<<Name())

/*------------------------------------------------------------------------------*\
	SmtpStates[]
		-	array of SMTP-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmSmtp::SmtpState BmSmtp::SmtpStates[BmSmtp::SMTP_FINAL] = {
	SmtpState( "auth via pop...", &BmSmtp::AuthViaPopServer),
	SmtpState( "connect...", &BmSmtp::Connect),
	SmtpState( "helo...", &BmSmtp::Helo),
	SmtpState( "auth...", &BmSmtp::Auth),
	SmtpState( "send...", &BmSmtp::SendMails),
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
	,	mServerMayHaveSizeLimit( false)
	,	mServerSupportsDSN( false)
	,	mPwdAcquisitorFunc( NULL)
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
		-	the mainloop, steps through all SMTP-stages and calls the corresponding
			handlers
\*------------------------------------------------------------------------------*/
bool BmSmtp::StartJob() {

	switch( CurrentJobSpecifier()) {
		case BM_CHECK_CAPABILITIES_JOB: {
			return true;
		}
		default: {
			int32 startState = mSmtpAccount->NeedsAuthViaPopServer() 
											? SMTP_AUTH_VIA_POP 
											: SMTP_CONNECT;
			const float delta = 100.0 / (SMTP_DONE-startState);
			const bool failed=true;
		
			mSmtpServer.InitCheck() == B_OK
													||	BM_THROW_RUNTIME("BmSmtp: could not create NetEndpoint");
			try {
				for( 	mState = startState; ShouldContinue() && mState<SMTP_DONE; ++mState) {
					if (mState == SMTP_AUTH && mSmtpAccount->NeedsAuthViaPopServer())
						continue;
					TStateMethod stateFunc = SmtpStates[mState].func;
					UpdateSMTPStatus( (mState==startState ? 0.0 : delta), NULL);
					(this->*stateFunc)();
				}
				UpdateSMTPStatus( delta, NULL);
				mSmtpAccount->mMailVect.clear();
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
				mSmtpAccount->mMailVect.clear();
				return false;
			}
			return true;
		}
	}
}

/*------------------------------------------------------------------------------*\
	UpdateSMTPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current SMTP-state
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
	BString text = BString() << currMsg << " of " << mSmtpAccount->mMailVect.size();
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
	BNetAddress addr;
	if (!mSmtpAccount->GetSMTPAddress( &addr) || mSmtpServer.Connect( addr) != B_OK) {
		BString s = BString("Could not connect to SMTP-Server ") << mSmtpAccount->SMTPServer();
		throw BM_network_error( s);
	}
	mConnected = true;
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	Helo()
		-	Sends greeting to server and checks result
		-	EHLO is tried first, aiming to find out more about server capabilities;
			if that fails, HELO is used
\*------------------------------------------------------------------------------*/
void BmSmtp::Helo() {
	BString domain = mSmtpAccount->DomainToAnnounce();
	if (!domain.Length())
		domain = OwnFQDN();
	BString cmd = BString("EHLO ") << domain;
	SendCommand( cmd);
	try {
		CheckForPositiveAnswer();
		Regexx rx;
		if (rx.exec( mAnswer, "^\\d\\d\\d.SIZE\\b", Regexx::newline)) {
			mServerMayHaveSizeLimit = true;
		}
		if (rx.exec( mAnswer, "^\\d\\d\\d.DSN\\b", Regexx::newline)) {
			mServerSupportsDSN = true;
		}
	} catch(...) {
		cmd = BString("HELO ") << domain;
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
}

/*------------------------------------------------------------------------------*\
	AuthViaPopServer()
		-	authenticates through pop-server (SMTP_AFTER_POP)
\*------------------------------------------------------------------------------*/
void BmSmtp::AuthViaPopServer() {
	BmMail* mail = mSmtpAccount->mMailVect[0].Get();
	if (mail) {
		BString sender = mail->Header()->DetermineSender();
		BmRef<BmPopAccount> sendingAcc = ThePopAccountList->FindAccountForAddress( sender);
		BMessage msg(BM_JOBWIN_POP);
		msg.AddString( BmJobModel::MSG_JOB_NAME, sendingAcc->Name().String());
		msg.AddInt32( BmJobModel::MSG_JOB_SPEC, BmPopper::BM_AUTH_ONLY_JOB);
		msg.AddBool( BmJobModel::MSG_JOB_THREAD, false);
		TheJobStatusWin->AddJob( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	Auth()
		-	Sends user/passwd combination and checks result
\*------------------------------------------------------------------------------*/
void BmSmtp::Auth() {
	BString pwd;
	bool pwdOK = false;
	bool first = true;
	BString authMethod = mSmtpAccount->AuthMethod();
	authMethod.ToUpper();
	if (!authMethod.Length())
		return;			// no authentication needed...
	while(!pwdOK) {
		if (first && mSmtpAccount->PwdStoredOnDisk()) {
			pwd = mSmtpAccount->Password();
		} else if (mPwdAcquisitorFunc) {
			if (!mPwdAcquisitorFunc( Name(), pwd)) {
				Disconnect();
				StopJob();
				return;
			}
		} else
			BM_THROW_RUNTIME( "Unable to acquire password !?!");

		first = false;
		if (authMethod == BmSmtpAccount::AUTH_PLAIN) {
			// PLAIN-method: send single base64-encoded string that is composed like this:
			// authenticateID + \0 + username + \0 + password
			// (where authenticateID is currently always empty)
			BString cmd = BString("AUTH PLAIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BString base64;
				cmd = BString("_") << mSmtpAccount->Username() << "_" << mSmtpAccount->Password();
				cmd[0] = '\0';
				cmd[mSmtpAccount->Username().Length()+1] = '\0';
				Encode( "base64", cmd, base64);
				SendCommand( "", base64);
			}
		} else if (authMethod == BmSmtpAccount::AUTH_PLAIN) {
			// LOGIN-method: send base64-encoded username, then send base64-encoded password:
			BString cmd = BString("AUTH LOGIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BString base64;
				Encode( "base64", mSmtpAccount->Username(), base64);
				SendCommand( base64);
				CheckForPositiveAnswer();
				Encode( "base64", mSmtpAccount->Password(), base64);
				SendCommand( "", base64);
			}
		}
		try {
			pwdOK = CheckForPositiveAnswer();
		} catch( BM_network_error &err) {
			// most probably a wrong password...
			BString errstr = err.what();
			int e;
			if ((e = mSmtpServer.Error()))
				errstr << "\nerror: " << e << ", " << mSmtpServer.ErrorStr();
			BString text = Name() << "\n\n" << errstr;
			BM_SHOWERR( BString("BmSmtp: ") << text);
		}
	}
}

/*------------------------------------------------------------------------------*\
	SendMails()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::SendMails() {
	for( uint32 i=0; i<mSmtpAccount->mMailVect.size(); ++i) {
		BmMail* mail = mSmtpAccount->mMailVect[i].Get();
		Mail( mail);
		Rcpt( mail);
		if (ThePrefs->GetBool("SpecialHeaderForEachBcc", true)) {
			Data( mail);
			BccRcpt( mail, true);
		} else {
			BccRcpt( mail, false);
			Data( mail);
		}
		mail->MarkAs( BM_MAIL_STATUS_SENT);
	}
	mSmtpAccount->mMailVect.clear();
}

/*------------------------------------------------------------------------------*\
	Mail()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::Mail( BmMail* mail) {
	BString sender = mail->Header()->DetermineSender();
	Regexx rx;
	if (!rx.exec( sender, "@\\w+")) {
		// no domain part within sender-address, we add our current domain:
		if (sender.FindFirst("@") == B_ERROR)
			sender << "@";
		BString fqdn = mSmtpAccount->DomainToAnnounce();
		sender << OwnDomain( fqdn);
	}
	BString cmd = BString("MAIL from:<") << sender <<">";
	if (mServerMayHaveSizeLimit) {
		int32 mailSize = mail->RawText().Length();
		cmd << " SIZE=" << mailSize;
	}
	SendCommand( cmd);
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	Rcpt()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::Rcpt( BmMail* mail) {
	BmAddrList::const_iterator iter;
	const BmAddressList toList = mail->Header()->GetAddressList( BM_FIELD_TO);
	for( iter=toList.begin(); iter != toList.end(); ++iter) {
		BString cmd = BString("RCPT to:<") << iter->AddrSpec() <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
	const BmAddressList ccList = mail->Header()->GetAddressList( BM_FIELD_CC);
	for( iter=ccList.begin(); iter != ccList.end(); ++iter) {
		BString cmd = BString("RCPT to:<") << iter->AddrSpec() <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
}

/*------------------------------------------------------------------------------*\
	BccRcpt()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::BccRcpt( BmMail* mail, bool sendDataForEachBcc) {
	BmAddrList::const_iterator iter;
	const BmAddressList bccList = mail->Header()->GetAddressList( BM_FIELD_BCC);
	for( iter=bccList.begin(); iter != bccList.end(); ++iter) {
		if (sendDataForEachBcc)
			Mail( mail);
		BString cmd = BString("RCPT to:<") << iter->AddrSpec() <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
		if (sendDataForEachBcc)
			Data( mail, iter->AddrSpec());
	}
}

/*------------------------------------------------------------------------------*\
	Data()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::Data( BmMail* mail, BString forBcc) {
	BString cmd = BString("DATA");
	SendCommand( cmd);
	CheckForPositiveAnswer();
	Regexx rx;
	cmd = mail->RawText();
	if (mail->Header()->GetStrippedFieldVal(BM_FIELD_BCC).Length()) {
		// remove BCC-header from mailtext...
		cmd = rx.replace( cmd, "^Bcc:\\s+.+?\\r\\n(\\s+.*?\\r\\n)*", "", Regexx::newline);
		if (forBcc.Length()) {
			// include BCC for current recipient within header so he/she/it can see
			// how this mail got sent to him/her/it:
			cmd = rx.replace( cmd, "^Mime:\\s+1.0", 
									BString("Mime: 1.0\r\nBcc: ")<<forBcc, Regexx::newline);
		}
	}
	// finally we dotstuff the mailtext...
	cmd = rx.replace( cmd, "^\\.", "..", Regexx::newline | Regexx::global);
	// ... and at a single dot at the end:
	if (cmd[cmd.Length()-1] != '\n')
		cmd << "\r\n";
	cmd << ".\r\n";
	SendCommand( cmd, "", true);
	CheckForPositiveAnswer();
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
	CheckForPositiveAnswer()
		-	waits for an answer from server and checks if it is ok
		-	throws an exception if answer indicates an error
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
			char *endp = buffer+searchOffset;
			if (firstBlock && numBytes>3 && buffer[3] != '-')
				done = true;
			while ( !done && (endp=strstr( endp, "\r\n")) != 0) {
				if (buffer+offset+numBytes >= endp +2/*\r\n*/ +3/*250*/
				&& endp[2+3] != '-') {
					// end of answer is indicated by line starting with three digits, NOT followed
					// by a minus ('-'):
					done = true;
				}
				endp++;
			}
			offset += numBytes;
			firstBlock = false;
		} while( !done && numBytes);
		mAnswer.UnlockBuffer( -1);
		BM_LOG2( BM_LogSmtp, BString("<--\n") << mAnswer);
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
void BmSmtp::SendCommand( BString cmd, BString secret, bool isMailData) {
	BString command;
	if (secret.Length()) {
		command = cmd + secret;
		BM_LOG( BM_LogSmtp, BString("-->\n") << cmd << " secret_data_omitted_here");
													// we do not want to log any passwords...
	} else {
		command = cmd;
		BM_LOG( BM_LogSmtp, BString("-->\n") << cmd);
	}
	if (command[command.Length()-1] != '\n')
		command << "\r\n";
	int32 size = command.Length();
	int32 sentSize;
	if (isMailData) {
		int32 blockSize = ThePrefs->GetInt("NetSendBufferSize", 10*1500);
		for( int32 block=0; block*blockSize < size; ++block) {
			int32 offs = block*blockSize;
			int32 sz = MIN( size-offs, blockSize);
			if ((sentSize = mSmtpServer.Send( command.String()+offs, sz)) != sz) {
				throw BM_network_error( BString("error during send, sent only ") << sentSize << " bytes instead of " << sz);
			}
			float delta = (100.0 * sz) / (size ? size : 1);
			BString text = BString("size: ") << BytesToString( size);
			UpdateMailStatus( delta, text.String(), 1);
		}
	} else {
		if ((sentSize = mSmtpServer.Send( command.String(), size)) != size) {
			throw BM_network_error( BString("error during send, sent only ") << sentSize << " bytes instead of " << size);
		}
	}
}

/*------------------------------------------------------------------------------*\
	Initialize statics:
\*------------------------------------------------------------------------------*/
int32 BmSmtp::FeedbackTimeout = 200*1000;

