/*
	BmSmtp.h

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


#ifndef _BmSmtp_h
#define _BmSmtp_h

#include <memory>

#include <Message.h>

#include "BmNetJobModel.h"

class BmSmtpAccount;

#define BM_SMTP_NEEDS_PWD						'bmSp'

/*------------------------------------------------------------------------------*\
	BmSmtpStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class BmSmtpStatusFilter : public BmStatusFilter {
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
class BmSmtp : public BmNetJobModel {
	typedef BmNetJobModel inherited;
	
	typedef vector< BmString> BmRcptVect;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_SMTP = 		"bm:smtp";
	static const char* const MSG_DELTA = 		"bm:delta";
	static const char* const MSG_TRAILING = 	"bm:trailing";
	static const char* const MSG_LEADING = 	"bm:leading";

	// message component definitions for additional info:
	static const char* const MSG_PWD = 	"bm:pwd";

	// job-specifier for checking server capabilities:
	static const int32 BM_CHECK_CAPABILITIES_JOB = 1;

	BmSmtp( const BmString& name, BmSmtpAccount* account);
	virtual ~BmSmtp();

	typedef bool BmPopAccAcquisitorFunc( const BmString, BmString&);
	inline void SetPopAccAcquisitorFunc( BmPopAccAcquisitorFunc* func)
													{ mPopAccAcquisitorFunc = func; }

	BmString SuggestAuthType() const;

	inline BmString Name() const			{ return ModelName(); }

	// overrides of netjob-model base:
	void UpdateProgress( uint32 numBytes);

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

private:
	BmRef<BmSmtpAccount> mSmtpAccount;	// Info about our smtp-account

	int32 mMsgTotalSize;
	int32 mCurrMailNr;
	int32 mCurrMailSize;

	int32 mState;								// current SMTP-state (refer enum below)
	enum States {
		SMTP_AUTH_VIA_POP = 0,
		SMTP_CONNECT,
		SMTP_HELO,
		SMTP_AUTH,
		SMTP_SEND,
		SMTP_QUIT,
		SMTP_DONE,
		SMTP_FINAL
	};

	// function that asks user for a pop-account (needed for SmtpAfterPop)
	BmPopAccAcquisitorFunc* mPopAccAcquisitorFunc;

	// stuff needed for internal SMTP-state-loop:
	typedef void (BmSmtp::*TStateMethod)();
	struct SmtpState {
		const char* text;
		TStateMethod func;
		SmtpState( const char* t, TStateMethod f) : text(t), func(f) { }
	};
	static SmtpState SmtpStates[SMTP_FINAL];

	// private functions:
	void StateConnect();
	void StateHelo();
	void StateAuthViaPopServer();
	void StateAuth();
	void StateSendMails();
	void StateDisconnect();

	void Quit( bool WaitForAnswer=false);
	void Mail( BmMail *mail);
	bool HasStdRcpts( BmMail *mail, BmRcptVect& rcptVect);
	void Rcpt( const BmRcptVect& rcptVect);
	void BccRcpt( BmMail *mail, bool sendDataForEachBcc, const BmString& headerText);
	void Data( BmMail *mail, const BmString& headerText, BmString forBcc="");
	void UpdateSMTPStatus( const float, const char*, bool failed=false, bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);

	bool mServerMayHaveSizeLimit;
	bool mServerSupportsDSN;
	BmString mSupportedAuthTypes;

	// Hide copy-constructor and assignment:
	BmSmtp( const BmSmtp&);
	BmSmtp operator=( const BmSmtp&);
};

#endif
