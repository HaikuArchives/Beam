/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmSmtp_h
#define _BmSmtp_h

#include <memory>
#include <set>
#include <vector>

#include <Message.h>

#include "BmDaemon.h"

#include "BmNetJobModel.h"

class BmSmtpAccount;

enum {
	BM_SMTP_NEEDS_PWD			= 'bmSp'
};

/*------------------------------------------------------------------------------*\
	BmSmtpStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmSmtpStatusFilter : public BmStatusFilter {
	typedef BmStatusFilter inherited;

public:
	BmSmtpStatusFilter( BmMemIBuf* input, uint32 blockSize=65536);

	// overrides of BmStatusFilter base:
	bool CheckForPositiveAnswer();
	void Reset( BmMemIBuf* input);

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	bool mAtStartOfLine;
	int8 mDigitCount;
	bool mIncludeUpToNewline;
};

/*------------------------------------------------------------------------------*\
	BmSmtp
		-	implements the SMTP-client
		-	each instance represents a single connection to a specific SMTP-server
		-	in general, each BmSmtp is started as a thread which exits when the
			SMTP-session has ended
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmSmtp : public BmNetJobModel {
	typedef BmNetJobModel inherited;
	
	typedef set< BmString> BmRcptSet;
	typedef vector< entry_ref> BmQueuedRefVect;
	
public:
	// message component definitions for additional info:
	static const char* const MSG_PWD;
	static const char* const MSG_REF;

	// job-specifiers:
	static const int32 BM_CHECK_CAPABILITIES_JOB;
	static const int32 BM_SEND_SPECIFIC_MAILS;

	BmSmtp( const BmString& name, BmSmtpAccount* account);
	virtual ~BmSmtp();

	void QueueMail( entry_ref eref);

	bool SupportsTLS() const;
	BmString SuggestAuthType() const;

	inline BmString Name() const			{ return ModelName(); }

	// overrides of netjob-model base:
	void UpdateProgress( uint32 numBytes);
	void SetupAdditionalInfo( BMessage* additionalInfo);

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

protected:
	// overrides of netjob-model base:
	void ExtractBase64(const BmString& text, BmString& base64);
	bool StartEncryption(const char* encType);

private:
	BmRef<BmSmtpAccount> mSmtpAccount;	// Info about our smtp-account

	int32 mMailCount;
	int32 mMsgTotalSize;
	int32 mCurrMailNr;
	int32 mCurrMailSize;

	int32 mState;								// current SMTP-state (refer enum below)
	enum States {
		SMTP_AUTH_VIA_POP = 0,
		SMTP_CONNECT,
		SMTP_HELO,
		SMTP_STARTTLS,
		SMTP_AUTH,
		SMTP_SEND,
		SMTP_QUIT,
		SMTP_DONE,
		SMTP_FINAL
	};

	// stuff needed for internal SMTP-state-loop:
	typedef void (BmSmtp::*TStateMethod)();
	struct SmtpState {
		const char* text;
		TStateMethod func;
		bool skip;
		SmtpState( const char* t, TStateMethod f) 
			: text(t)
			, func(f)
			, skip(false)						{ }
	};
	static SmtpState SmtpStates[SMTP_FINAL];

	// private functions:
	void StateConnect();
	void StateHelo();
	void StateAuthViaPopServer();
	void StateStartTLS();
	void StateAuth();
	void StateSendMails();
	void StateDisconnect();

	void Quit( bool WaitForAnswer=false);
	void Mail( BmMail *mail);
	bool HasStdRcpts( BmMail *mail, BmRcptSet& rcptSet);
	void Rcpt( const BmRcptSet& rcptSet);
	void BccRcpt( BmMail *mail, bool sendDataForEachBcc, 
					  const BmString& headerText);
	void Data( BmMail *mail, const BmString& headerText, BmString forBcc="");
	void UpdateSMTPStatus( const float, const char*, bool failed=false, 
								  bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);

	bool mServerMayHaveSizeLimit;
	bool mServerSupportsDSN;
	bool mServerSupportsTLS;
	BmString mSupportedAuthTypes;
	
	BmQueuedRefVect mQueuedRefVect;

	// Hide copy-constructor and assignment:
	BmSmtp( const BmSmtp&);
	BmSmtp operator=( const BmSmtp&);
};

#endif
