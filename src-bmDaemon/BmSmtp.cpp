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
#include "BmMemIO.h"
#include "BmNetUtil.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmSmtpAccount.h"
#include "BmSmtp.h"
#include "BmPrefs.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME (BmString("SMTP_")<<Name())

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
BmSmtp::BmSmtp( const BmString& name, BmSmtpAccount* account)
	:	BmJobModel( name)
	,	mSmtpAccount( account)
	,	mSmtpServer( NULL)
	,	mConnected( false)
	,	mState( 0)
	,	mServerMayHaveSizeLimit( false)
	,	mServerSupportsDSN( false)
	,	mPwdAcquisitorFunc( NULL)
	,	mPopAccAcquisitorFunc( NULL)
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
	delete mSmtpServer;
	TheLogHandler->FinishLog( BM_LOGNAME);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether or not the Smtp-job should continue to run
		-	in addition to the inherited behaviour, the Smtp-job will continue
			when it executes special jobs (not BM_DEFAULT_JOB), since in that
			case there are no controllers present.
\*------------------------------------------------------------------------------*/
bool BmSmtp::ShouldContinue() {
	return inherited::ShouldContinue() 
			 || CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all SMTP-stages and calls the corresponding
			handlers
\*------------------------------------------------------------------------------*/
bool BmSmtp::StartJob() {
	int32 startState = mSmtpAccount->NeedsAuthViaPopServer() 
								? SMTP_AUTH_VIA_POP 
								: SMTP_CONNECT;
	const float delta = 100.0 / (SMTP_DONE-startState);
	const bool failed=true;
		
	try {
		for( 	mState = startState; ShouldContinue() && mState<SMTP_DONE; ++mState) {
			if (mState == SMTP_AUTH && mSmtpAccount->NeedsAuthViaPopServer())
				continue;
			TStateMethod stateFunc = SmtpStates[mState].func;
			UpdateSMTPStatus( (mState==startState ? 0.0 : delta), NULL);
			(this->*stateFunc)();
			if (CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB && mState==SMTP_HELO) {
				UpdateSMTPStatus( delta*(SMTP_DONE-mState), NULL);
				return true;
			}
		}
		if (!ShouldContinue())
			UpdateSMTPStatus( 0.0, NULL, false, true);
		else
			UpdateSMTPStatus( delta, NULL);
		mSmtpAccount->mMailVect.clear();
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		int e;
		if ((e = mSmtpServer->Error()))
			errstr << "\nerror: " << e << ", " << mSmtpServer->ErrorStr();
		UpdateSMTPStatus( 0.0, NULL, failed);
		BmString text = Name() << "\n\n" << errstr;
		HandleError( BmString("BmSmtp: ") << text);
		mSmtpAccount->mMailVect.clear();
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	UpdateSMTPStatus( delta, detailText, failed)
		-	informs the interested party about a change in the current SMTP-state
		-	failed==true means that we only want to indicate the failure of the
			current stage (the BmString "FAILED!" will be shown)
\*------------------------------------------------------------------------------*/
void BmSmtp::UpdateSMTPStatus( const float delta, const char* detailText, 
										  bool failed, bool stopped) {
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_SMTP, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed)
		msg->AddString( MSG_TRAILING, (BmString(SmtpStates[mState].text) << " FAILED!").String());
	else if (stopped)
		msg->AddString( MSG_TRAILING, (BmString(SmtpStates[mState].text) << " Stopped!").String());
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
	BmString text = BmString() << currMsg << " of " << mSmtpAccount->mMailVect.size();
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_SMTP, Name().String());
	msg->AddString( BmJobModel::MSG_DOMAIN, "mailbar");
	msg->AddFloat( MSG_DELTA, delta);
	msg->AddString( MSG_LEADING, text.String());
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
	delete mSmtpServer;
	mSmtpServer = new BNetEndpoint;
	mSmtpServer->InitCheck() == B_OK		||	BM_THROW_RUNTIME("BmSmtp: could not create NetEndpoint");
	if (!mSmtpAccount->GetSMTPAddress( &addr) || mSmtpServer->Connect( addr) != B_OK) {
		BmString s = BmString("Could not connect to SMTP-Server ") << mSmtpAccount->SMTPServer();
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
		-	if EHLO succeeds, info about the server-capabilities are extracted from
			the answer
\*------------------------------------------------------------------------------*/
void BmSmtp::Helo() {
	BmString domain = mSmtpAccount->DomainToAnnounce();
	if (!domain.Length())
		domain = OwnFQDN();
	BmString cmd = BmString("EHLO ") << domain;
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
		if (rx.exec( mAnswer, "^\\d\\d\\d.AUTH\\b(.*?)$", Regexx::newline)) {
			mSupportedAuthTypes = rx.match[0].atom[0];
		}
	} catch(...) {
		cmd = BmString("HELO ") << domain;
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
}

/*------------------------------------------------------------------------------*\
	SuggestAuthType()
		-	looks at the auth-types supported by the server and selects the most secure
			of those that is supported by Beam.
\*------------------------------------------------------------------------------*/
BmString BmSmtp::SuggestAuthType() const {
	if (mSupportedAuthTypes.IFindFirst( BmSmtpAccount::AUTH_LOGIN) != B_ERROR)
		return BmSmtpAccount::AUTH_LOGIN;
	else if (mSupportedAuthTypes.IFindFirst( BmSmtpAccount::AUTH_PLAIN) != B_ERROR)
		return BmSmtpAccount::AUTH_PLAIN;
	else
		return "";
}

/*------------------------------------------------------------------------------*\
	AuthViaPopServer()
		-	authenticates through pop-server (SMTP_AFTER_POP)
\*------------------------------------------------------------------------------*/
void BmSmtp::AuthViaPopServer() {
	BmMail* mail = mSmtpAccount->mMailVect[0].Get();
	if (mail) {
		BmString sender = mail->Header()->DetermineSender();
		BmString accName = mSmtpAccount->AccForSmtpAfterPop();
		BmRef<BmPopAccount> sendingAcc 
			= dynamic_cast<BmPopAccount*>( ThePopAccountList->FindItemByKey( accName).Get());;
		if (!sendingAcc) {
			// no default pop-account set, we try to find out by looking at the
			// sender address:
			sendingAcc = ThePopAccountList->FindAccountForAddress( sender);
		}
		if (!sendingAcc) {
			// still no pop-account found, we ask user, if possible:
			if (mPopAccAcquisitorFunc) {
				if (!mPopAccAcquisitorFunc( Name(), accName)) {
					Disconnect();
					StopJob();
					return;
				}
				sendingAcc = dynamic_cast<BmPopAccount*>( ThePopAccountList->FindItemByKey( accName).Get());
			}
		}
		if (!sendingAcc)
			BM_THROW_RUNTIME( BmString("Sorry, could not determine pop-account for address '")<<sender<<"'\n\n Smtp-after-Pop authentication failed, giving up.");
		BmRef<BmPopper> popper( new BmPopper( sendingAcc->Name(), sendingAcc.Get()));
		popper->SetPwdAcquisitorFunc( BmPopperView::AskUserForPwd);
		popper->StartJobInThisThread( BmPopper::BM_AUTH_ONLY_JOB);
	}
}

/*------------------------------------------------------------------------------*\
	Auth()
		-	Sends user/passwd combination and checks result
		-	currently supports PLAIN- and LOGIN-authentication
\*------------------------------------------------------------------------------*/
void BmSmtp::Auth() {
	BmString pwd;
	bool pwdOK = false;
	bool first = true;
	BmString authMethod = mSmtpAccount->AuthMethod();
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
			BmString cmd = BmString("AUTH PLAIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BmString base64;
				cmd = BmString("_") << mSmtpAccount->Username() << "_" << mSmtpAccount->Password();
				int32 len=cmd.Length();
				char* buf = cmd.LockBuffer( len);
				buf[0] = '\0';
				buf[mSmtpAccount->Username().Length()+1] = '\0';
				cmd.UnlockBuffer( len);
				Encode( "base64", cmd, base64);
				SendCommand( base64, true);
			}
		} else if (authMethod == BmSmtpAccount::AUTH_LOGIN) {
			// LOGIN-method: send base64-encoded username, then send base64-encoded password:
			BmString cmd = BmString("AUTH LOGIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BmString base64;
				Encode( "base64", mSmtpAccount->Username(), base64);
				SendCommand( base64);
				CheckForPositiveAnswer();
				Encode( "base64", mSmtpAccount->Password(), base64);
				SendCommand( base64, true);
			}
		}
		try {
			pwdOK = CheckForPositiveAnswer();
		} catch( BM_network_error &err) {
			// most probably a wrong password...
			BmString errstr = err.what();
			int e;
			if ((e = mSmtpServer->Error()))
				errstr << "\nerror: " << e << ", " << mSmtpServer->ErrorStr();
			BmString text = Name() << "\n\n" << errstr;
			HandleError( BmString("BmSmtp: ") << text);
		}
	}
}

/*------------------------------------------------------------------------------*\
	SendMails()
		-	sends the queued mails to the server (invokes MAIL, RCPT and DATA-commands)
		-	depending on the handling of Bcc-recipients, each mail is only sent to
			the server once (SpecialHeaderForEachBcc=false) or each mail is being
			sent once for the standard recipients (To, Cc) plus a personalized version
			for each Bcc-recipient (SpecialHeaderForEachBcc=true)
\*------------------------------------------------------------------------------*/
void BmSmtp::SendMails() {
	Regexx rx;
	for( uint32 i=0; i<mSmtpAccount->mMailVect.size(); ++i) {
		BmMail* mail = mSmtpAccount->mMailVect[i].Get();
		if (!mail) {
			BM_LOGERR( BmString("SendMails(): mail no. ") << i << " is NULL, skipping it.");
			continue;
		}

		BmString headerText = mail->HeaderText();
		if (mail->IsRedirect() && !mail->Header()->IsFieldEmpty(BM_FIELD_RESENT_BCC)) {
			// remove RESENT-BCC-header from mailtext...
			headerText = rx.replace( headerText, "^Resent-Bcc:\\s+.+?\\r\\n(\\s+.*?\\r\\n)*", 
											 "", Regexx::newline);
		} else if (!mail->IsRedirect() && !mail->Header()->IsFieldEmpty(BM_FIELD_BCC)) {
			// remove BCC-header from mailtext...
			headerText = rx.replace( headerText, "^Bcc:\\s+.+?\\r\\n(\\s+.*?\\r\\n)*", 
											 "", Regexx::newline);
		}

		BmStringIBuf bodyText( mail->RawText().String()+mail->HeaderLength());

		BmRcptVect rcptVect;
		if (ThePrefs->GetBool("SpecialHeaderForEachBcc", true)) {
			if (HasStdRcpts( mail, rcptVect)) {
				Mail( mail);
				Rcpt( rcptVect);
				Data( mail, headerText, bodyText);
			}
			BccRcpt( mail, true, headerText, bodyText);
		} else {
			Mail( mail);
			if (HasStdRcpts( mail, rcptVect))
				Rcpt( rcptVect);
			BccRcpt( mail, false, headerText, bodyText);
			Data( mail, headerText, bodyText);
		}
		mail->MarkAs( BM_MAIL_STATUS_SENT);
	}
	mSmtpAccount->mMailVect.clear();
}

/*------------------------------------------------------------------------------*\
	Mail( mail)
		-	announces the given mail to the server
\*------------------------------------------------------------------------------*/
void BmSmtp::Mail( BmMail* mail) {
	BmString sender = mail->Header()->DetermineSender();
	Regexx rx;
	if (!rx.exec( sender, "@\\w+")) {
		// no domain part within sender-address, we add our current domain:
		if (sender.FindFirst("@") == B_ERROR)
			sender << "@";
		BmString fqdn = mSmtpAccount->DomainToAnnounce();
		sender << OwnDomain( fqdn);
	}
	BmString cmd = BmString("MAIL from:<") << sender <<">";
	if (mServerMayHaveSizeLimit) {
		int32 mailSize = mail->RawText().Length();
		cmd << " SIZE=" << mailSize;
	}
	SendCommand( cmd);
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	bool HasStdRcpts( mail)
		-	determines the list of standard-recipients (from TO & CC)
		-	returns true if at least one recipient has been set, false otherwise
			(the latter case might occur if a mail only contains BCCs)
\*------------------------------------------------------------------------------*/
bool BmSmtp::HasStdRcpts( BmMail* mail, BmRcptVect& rcptVect) {
	BmAddrList::const_iterator iter;
	const BmAddressList toList = mail->IsRedirect() 
											? mail->Header()->GetAddressList( BM_FIELD_RESENT_TO)
											: mail->Header()->GetAddressList( BM_FIELD_TO);
	for( iter=toList.begin(); iter != toList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		rcptVect.push_back( iter->AddrSpec());
	}
	const BmAddressList ccList = mail->IsRedirect() 
											? mail->Header()->GetAddressList( BM_FIELD_RESENT_CC)
											: mail->Header()->GetAddressList( BM_FIELD_CC);
	for( iter=ccList.begin(); iter != ccList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		rcptVect.push_back( iter->AddrSpec());
	}
	return rcptVect.size()>0;
}

/*------------------------------------------------------------------------------*\
	bool Rcpt( rcptVect)
		-	announces all given recipients to the server
\*------------------------------------------------------------------------------*/
void BmSmtp::Rcpt( const BmRcptVect& rcptVect) {
	for( uint32 i=0; i<rcptVect.size(); ++i) {
		BmString cmd = BmString("RCPT to:<") << rcptVect[i] <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
}

/*------------------------------------------------------------------------------*\
	BccRcpt( mail, sendDataForEachBcc)
		-	announces all Bcc-recipients of given mail to the server
		-	if param sendDataForEachBcc is set, a new mail will be created for each 
			Bcc-recipient, containing only it inside the Bcc-header
\*------------------------------------------------------------------------------*/
void BmSmtp::BccRcpt( BmMail* mail, bool sendDataForEachBcc,
							 const BmString& headerText, BmStringIBuf& bodyText) {
	BmAddrList::const_iterator iter;
	const BmAddressList bccList = mail->IsRedirect() 
											? mail->Header()->GetAddressList( BM_FIELD_RESENT_BCC)
											: mail->Header()->GetAddressList( BM_FIELD_BCC);
	for( iter=bccList.begin(); iter != bccList.end(); ++iter) {
		if (sendDataForEachBcc)
			Mail( mail);
		BmString cmd = BmString("RCPT to:<") << iter->AddrSpec() <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
		if (sendDataForEachBcc)
			Data( mail, headerText, bodyText, iter->AddrSpec());
	}
}

/*------------------------------------------------------------------------------*\
	Data( mail, forBcc)
		-	sends the given mail (the mailtext) to the server
		-	if param forBcc is set, the contained address is set as the mail's
			Bcc-header (only this address)
\*------------------------------------------------------------------------------*/
void BmSmtp::Data( BmMail* mail, const BmString& headerText, BmStringIBuf& bodyText,
						 BmString forBcc) {
	BmString cmd( "DATA");
	SendCommand( cmd);
	CheckForPositiveAnswer();
	BmString bcc;
	if (mail->IsRedirect()) {
		if (forBcc.Length()) {
			// include BCC for current recipient within header so he/she/it can see
			// how this mail got sent to him/her/it:
			bcc = BmString("Resent-Bcc: ") << forBcc;
		}
	} else {
		if (forBcc.Length()) {
			// include BCC for current recipient within header so he/she/it can see
			// how this mail got sent to him/her/it:
			bcc = BmString("Bcc: ") << forBcc;
		}
	}
	BmStringOBuf cmdBuf( bcc.Length()+headerText.Length()+bodyText.Size()+4096);
	cmdBuf.Write( bcc);
	cmdBuf.Write( headerText);
	BmDotstuffEncoder dotStuffed( bodyText);
	cmdBuf.Write( dotStuffed);
	cmdBuf.Write( ".\r\n");
	cmd.Adopt( cmdBuf.TheString());
	SendCommand( cmd, false, true);
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
	BmString cmd("QUIT");
	try {
		SendCommand( cmd);
		if (WaitForAnswer) {
			GetAnswer();
		}
	} catch(...) {	}
	mSmtpServer->Close();
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
			BmString err("Server answers: \n");
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

	BM_LOG3( BM_LogSmtp, BmString("bufSize:") << bufSize);
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
				BM_LOG2( BM_LogSmtp, BmString("bufSize enlarged to:") << bufSize);
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
		mAnswer.UnlockBuffer( offset);
		BM_LOG2( BM_LogSmtp, BmString("<--\n") << mAnswer);
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
		if (mSmtpServer->IsDataPending( BmSmtp::FeedbackTimeout)) {
			if ((numBytes = mSmtpServer->Receive( buffer, max-1)) > 0) {
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
	SendCommand( cmd, isSecret, isMailData)
		-	sends the specified SMTP-command to the server.
\*------------------------------------------------------------------------------*/
void BmSmtp::SendCommand( BmString& command, bool isSecret, bool isMailData) {
	if (!isSecret) {
		// we do not want to log any passwords...
		if (isMailData)
			BM_LOG3( BM_LogSmtp, BmString("-->\n") << command);
		else
			BM_LOG( BM_LogSmtp, BmString("-->\n") << command);
	}
	if (!command.Length() || (!isMailData && command[command.Length()-1] != '\n'))
		command << "\r\n";
	int32 size = command.Length();
	int32 sentSize;
	if (isMailData) {
		int32 blockSize = ThePrefs->GetInt("NetSendBufferSize", 10*1500);
		for( int32 block=0; block*blockSize < size; ++block) {
			int32 offs = block*blockSize;
			int32 sz = MIN( size-offs, blockSize);
			if ((sentSize = mSmtpServer->Send( command.String()+offs, sz)) != sz) {
				throw BM_network_error( BmString("error during send, sent only ") << sentSize << " bytes instead of " << sz);
			}
			float delta = (100.0 * sz) / (size ? size : 1);
			BmString text = BmString("size: ") << BytesToString( size);
			UpdateMailStatus( delta, text.String(), 1);
		}
	} else {
		if ((sentSize = mSmtpServer->Send( command.String(), size)) != size) {
			throw BM_network_error( BmString("error during send, sent only ") << sentSize << " bytes instead of " << size);
		}
	}
}

/*------------------------------------------------------------------------------*\
	Initialize statics:
\*------------------------------------------------------------------------------*/
int32 BmSmtp::FeedbackTimeout = 200*1000;

