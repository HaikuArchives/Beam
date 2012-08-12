/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <ctype.h>
#include <memory.h>
#include <stdio.h>

#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmFilter.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmMemIO.h"
#include "BmNetEndpointRoster.h"
#include "BmNetUtil.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmSmtpAccount.h"
#include "BmSmtp.h"
#include "BmUtil.h"





/********************************************************************************\
	BmSmtpStatusFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSmtpStatusFilter::BmSmtpStatusFilter( BmMemIBuf* input, uint32 blockSize)
	:	inherited( input, blockSize)
	,	mAtStartOfLine( true)
	,	mDigitCount( 0)
	,	mIncludeUpToNewline( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtpStatusFilter::Reset( BmMemIBuf* input) {
	inherited::Reset( input);
	mAtStartOfLine = true;
	mDigitCount = 0;
	mIncludeUpToNewline = false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtpStatusFilter::Filter( const char* srcBuf, uint32& srcLen, 
											char* destBuf, uint32& destLen) {
	const char* src = srcBuf;
	const char* srcEnd = srcBuf+srcLen;

	bool needData = false;
	if (mInfoMsg)
		mInfoMsg->FindBool(BmPopper::IMSG_NEED_DATA, &needData);

	if (mHaveStatus) {
		uint32 size = std::min( destLen, srcLen);
		memcpy( destBuf, srcBuf, size);
		srcLen = destLen = size;
	} else {
		for( ;src<srcEnd; ++src) {
			if (mIncludeUpToNewline) {
				if (*src=='\n') {
					mHaveStatus = true;
					break;
				} else
					continue;
			} else if (mAtStartOfLine || mDigitCount) {
				if (mDigitCount==3 && *src!='-') {
					mIncludeUpToNewline = true;
				} else if (mDigitCount<3 && isdigit( *src))
					mDigitCount++;
				else
					mDigitCount = 0;
			}
			mAtStartOfLine = (*src=='\n');
		}
		uint32 statusSize = src-srcBuf;
		mStatusText.Append( srcBuf, statusSize);
		if (mHaveStatus) {
			src++;								// skip newline
			if (!needData)
				mEndReached = true;
		}
		srcLen = src-srcBuf;
		destLen = 0;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmSmtpStatusFilter::CheckForPositiveAnswer() {
	if (mStatusText.Length() && mStatusText.ByteAt(0) > '3') {
		BmString err("Server answers: \n");
		err += mStatusText;
		err.RemoveAll( "\r");
		throw BM_network_error( err);
	}
	return true;
}



/********************************************************************************\
	BmSmtp
\********************************************************************************/

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME Name()

// message component definitions for additional info:
const char* const BmSmtp::MSG_PWD = 	"bm:pwd";

// job-specifier for checking server capabilities:
const int32 BmSmtp::BM_CHECK_CAPABILITIES_JOB = 1;

/*------------------------------------------------------------------------------*\
	SmtpStates[]
		-	array of SMTP-states, each with title and corresponding handler-method
\*------------------------------------------------------------------------------*/
BmSmtp::SmtpState BmSmtp::SmtpStates[BmSmtp::SMTP_FINAL] = {
	SmtpState( "auth via pop...", &BmSmtp::StateAuthViaPopServer),
	SmtpState( "connect...", &BmSmtp::StateConnect),
	SmtpState( "helo...", &BmSmtp::StateHelo),
	SmtpState( "starttls...", &BmSmtp::StateStartTLS),
	SmtpState( "auth...", &BmSmtp::StateAuth),
	SmtpState( "send...", &BmSmtp::StateSendMails),
	SmtpState( "quit...", &BmSmtp::StateDisconnect),
	SmtpState( "done", NULL)
};

/*------------------------------------------------------------------------------*\
	BmSmtp( info)
		-	contructor
\*------------------------------------------------------------------------------*/
BmSmtp::BmSmtp( const BmString& name, BmSmtpAccount* account)
	:	inherited( BmString("SMTP_")<<name, BM_LogSmtp, 
					  new BmSmtpStatusFilter( NULL))
	,	mSmtpAccount( account)
	,	mMailCount( 0)
	,	mMsgTotalSize( 0)
	,	mCurrMailNr( 0)
	,	mCurrMailSize( 0)
	,	mState( 0)
	,	mServerMayHaveSizeLimit( false)
	,	mServerSupportsDSN( false)
	,	mServerSupportsTLS( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmSmtp()
		-	destructor
		-	frees all associated memory (hopefully)
\*------------------------------------------------------------------------------*/
BmSmtp::~BmSmtp() { 
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
	if (mConnection && mConnection->IsStopRequested())
		return false;
	return CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB
			 || inherited::ShouldContinue() ;
}

/*------------------------------------------------------------------------------*\
	SetupAdditionalInfo()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::SetupAdditionalInfo( BMessage* additionalInfo)
{
	additionalInfo->AddString(BmNetEndpoint::MSG_CLIENT_CERT_NAME,
									  mSmtpAccount->ClientCertificate().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_SERVER_NAME,
									  mSmtpAccount->SMTPServer().String());
	additionalInfo->AddString(BmNetEndpoint::MSG_ACCEPTED_CERT_ID,
									  mSmtpAccount->AcceptedCertID().String());
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the mainloop, steps through all SMTP-stages and calls the 
			corresponding handlers
\*------------------------------------------------------------------------------*/
bool BmSmtp::StartJob() {
	int32 startState = SMTP_CONNECT;
	if (mSmtpAccount->NeedsAuthViaPopServer()
	&& CurrentJobSpecifier() != BM_CHECK_CAPABILITIES_JOB)
		// authentication through pop-server takes place before anything else:
		startState = SMTP_AUTH_VIA_POP;

	for( int32 state = startState; state<SMTP_DONE; ++state)
		SmtpStates[state].skip = false;

	if (mSmtpAccount->NeedsAuthViaPopServer()) {
		// authorization is done via POP-server, so we skip our own:
		SmtpStates[SMTP_AUTH].skip = true;
	}

	if (CurrentJobSpecifier() == BM_CHECK_CAPABILITIES_JOB) {
		// when checking capabilities, we skip the real stuff (but we
		// still take the STARTTLS-step, as the encrypted EHLO might
		// yield a different result from the non-encrypted EHLO before!):
		SmtpStates[SMTP_AUTH].skip = true;
		SmtpStates[SMTP_SEND].skip = true;
	}
	
	int32 skipped = 0;
	for( int32 state = startState; state<SMTP_DONE; ++state) {
		if (SmtpStates[state].skip)
			skipped++;
	}

	SmtpStates[SMTP_STARTTLS].skip = mSmtpAccount->NeedsAuthViaPopServer();

	const float delta = 100.0f / float(SMTP_DONE-startState-skipped);
	const bool failed=true;
	try {
		for( 	mState = startState; 
				ShouldContinue() && mState<SMTP_DONE; ++mState) {
			if (SmtpStates[mState].skip)
				continue;
			TStateMethod stateFunc = SmtpStates[mState].func;
			UpdateSMTPStatus( (mState==startState ? 0.0f : delta), NULL);
			(this->*stateFunc)();
			if (!ShouldContinue()) {
				Disconnect();
				break;
			}
		}
		if (!ShouldContinue())
			UpdateSMTPStatus( 0.0, NULL, false, true);
		else
			UpdateSMTPStatus( delta, NULL);
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BmString errstr = err.what();
		int e;
		if (mConnection && (e = mConnection->Error())!=B_OK)
			errstr << "\nerror: " << e << ", " << mConnection->ErrorStr();
		UpdateSMTPStatus( 0.0, NULL, failed);
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
		errMsg << "The job for account " << mSmtpAccount->Name() 
				 << "received an unknown exception and died!";
		HandleError(errMsg);
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
	std::auto_ptr<BMessage> msg( new BMessage( BM_JOB_UPDATE_STATE));
	msg->AddString( MSG_MODEL, Name().String());
	msg->AddString( MSG_DOMAIN, "statbar");
	msg->AddFloat( MSG_DELTA, delta);
	if (failed) {
		msg->AddString( MSG_TRAILING, 
							 (BmString(SmtpStates[mState].text) 
								<< " FAILED!").String());
		msg->AddBool( MSG_FAILED, true);
	} else if (stopped)
		msg->AddString( MSG_TRAILING, 
							 (BmString(SmtpStates[mState].text) 
							 	<< " Stopped!").String());
	else
		msg->AddString( MSG_TRAILING, SmtpStates[mState].text);
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
void BmSmtp::UpdateMailStatus( const float delta, const char* detailText, 
										 int32 currMsg) {
	BmString text = BmString() << currMsg << " of " << mMailCount;
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
	UpdateProgress( numBytes)
		-
\*------------------------------------------------------------------------------*/
void BmSmtp::UpdateProgress( uint32 numBytes) {
	float delta 
		= (100.0f * float(numBytes)) / float(mMsgTotalSize ? mMsgTotalSize : 1);
	BmString detailText = BmString("size: ") << BytesToString( mCurrMailSize);
	UpdateMailStatus( delta, detailText.String(), mCurrMailNr);
}

/*------------------------------------------------------------------------------*\
	StateConnect()
		-	Initiates network-connection to SMTP-server
\*------------------------------------------------------------------------------*/
void BmSmtp::StateConnect() {
	BNetAddress addr;
	if (addr.SetTo( mSmtpAccount->SMTPServer().String(), 
						 mSmtpAccount->PortNr()) != B_OK) {
		BmString s = BmString("Could not determine address of SMTP-Server ") 
							<< mSmtpAccount->SMTPServer();
		throw BM_network_error( s);
	}
	if (!Connect( &addr)) {
		BmString s = BmString("Could not connect to SMTP-Server ") 
							<< mSmtpAccount->SMTPServer() 
						 	<< "\n\bError:\n\t"<< mErrorString;
		throw BM_network_error( s);
	}
	BmString encryptionType = mSmtpAccount->EncryptionType();
	if (TheNetEndpointRoster->SupportsEncryption()
	&& (encryptionType.ICompare(BmSmtpAccount::ENCR_TLS) == 0
		|| encryptionType.ICompare(BmSmtpAccount::ENCR_SSL) == 0)) {
		// straight TLS or SSL, we start the encryption layer: 
		if (!StartEncryption(encryptionType.String()))
			return;
	}
	// accept server greeting (either encrypted or unencrypted):
	CheckForPositiveAnswer();
}

/*------------------------------------------------------------------------------*\
	StateHelo()
		-	Sends greeting to server and checks result
		-	EHLO is tried first, aiming to find out more about server 
			capabilities; if that fails, HELO is used
		-	if EHLO succeeds, info about the server-capabilities are extracted
			from the answer
\*------------------------------------------------------------------------------*/
void BmSmtp::StateHelo() {
	BmString domain = mSmtpAccount->DomainToAnnounce();
	if (!domain.Length())
		domain = BeamRoster->OwnFQDN();
	BmString cmd = BmString("EHLO ") << domain;
	SendCommand( cmd);
	try {
		CheckForPositiveAnswer();
		Regexx rx;
		if (rx.exec( StatusText(), "^\\d\\d\\d.SIZE\\b", Regexx::newline)) {
			mServerMayHaveSizeLimit = true;
		}
		if (rx.exec( StatusText(), "^\\d\\d\\d.DSN\\b", Regexx::newline)) {
			mServerSupportsDSN = true;
		}
		if (rx.exec( 
			StatusText(), "^\\d\\d\\d.AUTH\\s+(.*?)$", Regexx::newline
		)) {
			mSupportedAuthTypes = rx.match[0].atom[0];
		} else if (rx.exec( 
			StatusText(), "^\\d\\d\\d.AUTH\\s*=\\s*(.*?)$", Regexx::newline
		)) {
			mSupportedAuthTypes = rx.match[0].atom[0];
		}
		if (rx.exec( StatusText(), "^\\d\\d\\d.STARTTLS\\b", Regexx::newline)) {
			mServerSupportsTLS = true;
		}
	} catch(...) {
		cmd = BmString("HELO ") << domain;
		SendCommand( cmd);
		CheckForPositiveAnswer();
	}
}

/*------------------------------------------------------------------------------*\
	StateStartTLS()
		-	starts TLS encryption if required
\*------------------------------------------------------------------------------*/
void BmSmtp::StateStartTLS() {
	// check if encryption via STARTTLS is requested (and possible):
	BmString encryptionType = mSmtpAccount->EncryptionType();

	// automatic means: use STARTTLS if available:
	if (encryptionType.ICompare(BmSmtpAccount::ENCR_AUTO) == 0
	&& mServerSupportsTLS) {
		encryptionType = BmSmtpAccount::ENCR_STARTTLS;
	}
	
	if (!TheNetEndpointRoster->SupportsEncryption()
	|| encryptionType.ICompare(BmSmtpAccount::ENCR_STARTTLS) != 0)
		return;

	// let's try to initiate TLS...
	SendCommand( "STARTTLS");
	CheckForPositiveAnswer();

	if (!StartEncryption(BmSmtpAccount::ENCR_TLS))
		return;

	// STARTTLS resets the SMTP-session to initial state, so we need
	// to send a second EHLO/HELO:
	StateHelo();
}

/*------------------------------------------------------------------------------*\
	StartEncryption()
		-	extends activation of SSL/TLS encryption layer with automatic
			updating of newly accepted certificate ID.
\*------------------------------------------------------------------------------*/
bool BmSmtp::StartEncryption(const char* encType)
{
	bool ok = inherited::StartEncryption(encType);
	if (ok) {
		BmString certID = mConnection->NewAcceptedCertID();
		if (certID.Length() && mSmtpAccount->AcceptedCertID() != certID) {
			mSmtpAccount->AcceptedCertID(certID);
			TheSmtpAccountList->MarkAsChanged();
		}
	}
	return ok;
}

/*------------------------------------------------------------------------------*\
	StateAuthViaPopServer()
		-	authenticates through pop-server (SMTP_AFTER_POP)
\*------------------------------------------------------------------------------*/
void BmSmtp::StateAuthViaPopServer() {
	BmString accName = mSmtpAccount->AccForSmtpAfterPop();
	BmRef<BmPopAccount> sendingAcc = dynamic_cast<BmPopAccount*>( 
		TheRecvAccountList->FindItemByKey( accName).Get()
	);
	if (!sendingAcc)
		BM_THROW_RUNTIME( BmString("Sorry, could not determine pop-account")
									<< "'\n\n Smtp-after-Pop authentication failed, "
										"giving up.");
	BmRef<BmPopper> popper = new BmPopper(sendingAcc->Name(), sendingAcc.Get());
	popper->StartJobInThisThread( BmPopper::BM_AUTH_ONLY_JOB);
}

/*------------------------------------------------------------------------------*\
	ExtractBase64()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtp::ExtractBase64(const BmString& text, BmString& base64)
{
	text.CopyInto(base64, 4, text.Length());
}

/*------------------------------------------------------------------------------*\
	StateAuth()
		-	Sends user/passwd combination and checks result
		-	currently supports PLAIN- and LOGIN-authentication
\*------------------------------------------------------------------------------*/
void BmSmtp::StateAuth() {
	BmString pwd;
	bool pwdOK = false;
	bool first = true;
	BmString authMethod = mSmtpAccount->AuthMethod();
	if (authMethod == BmSmtpAccount::AUTH_AUTO)
		authMethod = SuggestAuthType();
	if (!authMethod.Length() || authMethod == BmSmtpAccount::AUTH_NONE)
		return;			// no authentication needed...
	while(!pwdOK) {
		if (first && mSmtpAccount->PwdStoredOnDisk()) {
			pwd = mSmtpAccount->Password();
		} else {
			BmString text( "Please enter password for SMTP-Account <");
			text << Name() << ">:";
			if (!ShouldContinue() || !BeamGuiRoster->AskUserForPwd( text, pwd)) {
				Disconnect();
				StopJob();
				return;
			}
		}

		first = false;
		if (authMethod == BmSmtpAccount::AUTH_PLAIN) {
			// PLAIN-method: send single base64-encoded string that is composed
			// like this:
			// authenticateID + \0 + username + \0 + password
			// (where authenticateID is currently always empty)
			BmString cmd = BmString("AUTH PLAIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BmString base64;
				cmd = BmString("_") << mSmtpAccount->Username() << "_" 
							<< mSmtpAccount->Password();
				int32 len=cmd.Length();
				char* buf = cmd.LockBuffer( len);
				buf[0] = '\0';
				buf[mSmtpAccount->Username().Length()+1] = '\0';
				cmd.UnlockBuffer( len);
				Encode( "base64", cmd, base64);
				SendCommand( "", base64);
			}
		} else if (authMethod == BmSmtpAccount::AUTH_LOGIN) {
			// LOGIN-method: send base64-encoded username, then send 
			// base64-encoded password:
			BmString cmd = BmString("AUTH LOGIN");
			SendCommand( cmd);
			if (CheckForPositiveAnswer()) {
				BmString base64;
				Encode( "base64", mSmtpAccount->Username(), base64);
				SendCommand( base64);
				CheckForPositiveAnswer();
				Encode( "base64", mSmtpAccount->Password(), base64);
				SendCommand( "", base64);
			}
		} else if (authMethod == BmSmtpAccount::AUTH_CRAM_MD5) {
			BmString cmd = BmString("AUTH CRAM-MD5");
			SendCommand( cmd);
			AuthCramMD5(mSmtpAccount->Username(), mSmtpAccount->Password());
		} else if (authMethod == BmSmtpAccount::AUTH_DIGEST_MD5) {
			BmString cmd = BmString("AUTH DIGEST-MD5");
			SendCommand( cmd);
			BmString serviceUri = BmString("smtp/") << mSmtpAccount->SMTPServer();
			AuthDigestMD5(mSmtpAccount->Username(), mSmtpAccount->Password(),
							  serviceUri);
		} else {
			throw BM_runtime_error( BmString("Unknown authentication type '")
										   << authMethod << "' found!?! Skipping!");
		}
		try {
			pwdOK = CheckForPositiveAnswer();
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
	StateSendMails()
		-	sends the queued mails to the server (invokes MAIL, RCPT and 
			DATA-commands)
		-	depending on the handling of Bcc-recipients, each mail is only sent to
			the server once (SpecialHeaderForEachBcc=false) or each mail is being
			sent once for the standard recipients (To, Cc) plus a personalized 
			version for each Bcc-recipient (SpecialHeaderForEachBcc=true)
\*------------------------------------------------------------------------------*/
void BmSmtp::StateSendMails() {
	mMailCount = mQueuedRefVect.size();

	mMsgTotalSize = 0;
	vector<BmRef<BmMailRef> > mailRefs;
	BmRef<BmMail> mail;
	for( int32 i=0; i<mMailCount; ++i) {
		mailRefs.push_back(BmMailRef::CreateInstance( mQueuedRefVect[i]));
		if (mailRefs[i] && mailRefs[i]->IsValid())
			mMsgTotalSize += int32(mailRefs[i]->Size());
	}

	Regexx rx;
	mCurrMailNr = 1;
	for( int32 i=0; i<mMailCount; ++i, ++mCurrMailNr) {
		if (!mailRefs[i] || !mailRefs[i]->IsValid()) {
			BM_LOGERR( BmString("SendMails(): mail no. ") << i+1
								<< " can't be found, skipping it.");
			continue;
		}
		BmRef<BmMail> mail = BmMail::CreateInstance( mailRefs[i].Get());
		if (mail) {
			if (mail->InitCheck() != B_OK)
				mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
			if (mail->InitCheck() != B_OK) {
				BM_LOGERR( BmString("SendMails(): mail no. ") << i+1
									<< " can't be read, skipping it.");
				continue;
			}
		}
		mCurrMailSize = mail->RawText().Length();

		BmString headerText = mail->HeaderText();
		if (!mail->Header()->IsFieldEmpty(BM_FIELD_RESENT_BCC)) {
			// remove RESENT-BCC-header from mailtext...
			headerText = rx.replace( 
				headerText, 
				"^Resent-Bcc:\\s*.+?\\r\\n(\\s+.*?\\r\\n)*", 
				"", Regexx::newline
			);
		}
		if (!mail->Header()->IsFieldEmpty(BM_FIELD_BCC)) {
			// remove BCC-header from mailtext...
			headerText = rx.replace( 
				headerText, 
				"^Bcc:\\s*.+?\\r\\n(\\s+.*?\\r\\n)*", 
				"", Regexx::newline
			);
		}

		try {
			BmRcptSet rcptSet;
			if (ThePrefs->GetBool("SpecialHeaderForEachBcc")) {
				if (HasStdRcpts( mail.Get(), rcptSet)) {
					Mail( mail.Get());
					Rcpt( rcptSet);
					Data( mail.Get(), headerText);
				}
				BccRcpt( mail.Get(), true, headerText);
			} else {
				Mail( mail.Get());
				if (HasStdRcpts( mail.Get(), rcptSet))
					Rcpt( rcptSet);
				BccRcpt( mail.Get(), false, headerText);
				Data( mail.Get(), headerText);
			}
			if (ShouldContinue()) {
				mail->MarkAs( BM_MAIL_STATUS_SENT);
				mail->ApplyOutboundFilters();
					// give filters a chance that check for 'Sent'-status...
			}
		} catch( BM_runtime_error &err) {
			// a problem occurred, we tell the user:
			BM_LOGERR( BmString("SendMails(): mail no. ") << i+1
								<< " couldn't be sent.\n\nError:\n" << err.what());
			mail->MarkAs( BM_MAIL_STATUS_ERROR);
				// mark mail as ERROR since it couldn't be sent
			SendCommand("RSET");
				// reset SMTP-state in order to start afresh with next mail
		}
	}
	mCurrMailSize = 0;
	mCurrMailNr = 0;
}

/*------------------------------------------------------------------------------*\
	StateDisconnect()
		-	tells the server that we are finished
\*------------------------------------------------------------------------------*/
void BmSmtp::StateDisconnect() {
	Quit( true);
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
bool BmSmtp::HasStdRcpts( BmMail* mail, BmRcptSet& rcptSet) {
	BmAddrList::const_iterator iter;
	const BmAddressList& toList 
		= mail->IsRedirect() 
			? mail->Header()->GetAddressList( BM_FIELD_RESENT_TO)
			: mail->Header()->GetAddressList( BM_FIELD_TO);
	for( iter=toList.begin(); iter != toList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		rcptSet.insert( iter->AddrSpec());
	}
	const BmAddressList& ccList 
		= mail->IsRedirect() 
			? mail->Header()->GetAddressList( BM_FIELD_RESENT_CC)
			: mail->Header()->GetAddressList( BM_FIELD_CC);
	for( iter=ccList.begin(); iter != ccList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		rcptSet.insert( iter->AddrSpec());
	}
	return rcptSet.size() > 0;
}

/*------------------------------------------------------------------------------*\
	bool Rcpt( rcptSet)
		-	announces all given recipients to the server
\*------------------------------------------------------------------------------*/
void BmSmtp::Rcpt( const BmRcptSet& rcptSet) {
	BmRcptSet::const_iterator rcptIter;
	for( rcptIter = rcptSet.begin(); rcptIter != rcptSet.end(); ++rcptIter) {
		BmString cmd = BmString("RCPT to:<") << (*rcptIter) << ">";
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
							 const BmString& headerText) {
	BmAddrList::const_iterator iter;
	const BmAddressList& bccList 
		= mail->IsRedirect() 
			? mail->Header()->GetAddressList( BM_FIELD_RESENT_BCC)
			: mail->Header()->GetAddressList( BM_FIELD_BCC);
	for( iter=bccList.begin(); iter != bccList.end(); ++iter) {
		if (sendDataForEachBcc)
			Mail( mail);
		BmString cmd = BmString("RCPT to:<") << iter->AddrSpec() <<">";
		SendCommand( cmd);
		CheckForPositiveAnswer();
		if (sendDataForEachBcc)
			Data( mail, headerText, iter->AddrSpec());
	}
}

/*------------------------------------------------------------------------------*\
	Data( mail, forBcc)
		-	sends the given mail (the mailtext) to the server
		-	if param forBcc is set, the contained address is set as the mail's
			Bcc-header (only this address)
\*------------------------------------------------------------------------------*/
void BmSmtp::Data( BmMail* mail, const BmString& headerText, BmString forBcc) {
	BmString cmd( "DATA");
	SendCommand( cmd);
	CheckForPositiveAnswer();
	BmString completeHeader;
	if (forBcc.Length()) {
		if (mail->IsRedirect()) {
			// include BCC for current recipient within header so he/she/it can
			// see how this mail got sent to him/her/it:
			completeHeader = BmString("Resent-Bcc: ") << forBcc << "\r\n" 
										<< headerText;
		} else {
			// include BCC for current recipient within header so he/she/it can
			// see how this mail got sent to him/her/it:
			completeHeader = BmString("Bcc: ") << forBcc << "\r\n" << headerText;
		}
	} else
		completeHeader = headerText;
	BmStringIBuf sendBuf( completeHeader);
	sendBuf.AddBuffer( mail->RawText().String()+mail->HeaderLength());
	time_t before = time(NULL);
	SendCommandBuf( sendBuf, "", true, true);
	int32 len = mail->RawText().Length();
	if (len > ThePrefs->GetInt("LogSpeedThreshold", 100*1024)) {
		time_t after = time(NULL);
		time_t duration = after-before > 0 ? after-before : 1;
		// log speed for mails that exceed a certain size:
		BM_LOG( BM_LogSmtp, 
				  BmString("Sent mail of size ") << len
						<< " bytes in " << duration << " seconds => " 
						<< len/duration/1024.0 << "KB/s");
	}
	CheckForPositiveAnswer();
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
		if (WaitForAnswer)
			GetAnswer();
	} catch(...) {	}
	Disconnect();
}

/*------------------------------------------------------------------------------*\
	QueueMail()
		-	adds given entry_ref to the queue of mails that shall be send
\*------------------------------------------------------------------------------*/
void BmSmtp::QueueMail( entry_ref eref)
{
	mQueuedRefVect.push_back( eref);
}

/*------------------------------------------------------------------------------*\
	SupportsTLS()
		-	returns whether or not the server has indicated that it supports 
			the STARTTLS command
\*------------------------------------------------------------------------------*/
bool BmSmtp::SupportsTLS() const
{
	return mServerSupportsTLS;
}

/*------------------------------------------------------------------------------*\
	SuggestAuthType(bool* supportsStartTls)
		-	looks at the auth-types supported by the server and selects the most 
			secure of those that is supported by Beam.
\*------------------------------------------------------------------------------*/
BmString BmSmtp::SuggestAuthType() const {
	if (mSupportedAuthTypes.IFindFirst( BmSmtpAccount::AUTH_DIGEST_MD5) >= 0)
		return BmSmtpAccount::AUTH_DIGEST_MD5;
	else if (mSupportedAuthTypes.IFindFirst( BmSmtpAccount::AUTH_CRAM_MD5) >= 0)
		return BmSmtpAccount::AUTH_CRAM_MD5;
	else if (mSupportedAuthTypes.IFindFirst( BmSmtpAccount::AUTH_LOGIN) >= 0)
		return BmSmtpAccount::AUTH_LOGIN;
	else if (mSupportedAuthTypes.IFindFirst( 
		BmSmtpAccount::AUTH_PLAIN
	) != B_ERROR)
		return BmSmtpAccount::AUTH_PLAIN;
	else
		return "";
}
