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
#include <NetEndpoint.h>

#include "BmDataModel.h"

class BmSmtpAccount;

#define BM_SMTP_NEEDS_PWD						'bmSp'

/*------------------------------------------------------------------------------*\
	BmSmtp
		-	implements the SMTP-client
		-	each instance represents a single connection to a specific SMTP-server
		-	in general, each BmSmtp is started as a thread which exits when the
			SMTP-session has ended
\*------------------------------------------------------------------------------*/
class BmSmtp : public BmJobModel {
	typedef BmJobModel inherited;
	
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

	typedef bool BmPwdAcquisitorFunc( const BmString, BmString&);
	inline void SetPwdAcquisitorFunc( BmPwdAcquisitorFunc* func)
													{ mPwdAcquisitorFunc = func; }

	typedef bool BmPopAccAcquisitorFunc( const BmString, BmString&);
	inline void SetPopAccAcquisitorFunc( BmPopAccAcquisitorFunc* func)
													{ mPopAccAcquisitorFunc = func; }

	BmString SuggestAuthType() const;

	inline BmString Name() const			{ return ModelName(); }

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

private:
	static int32 FeedbackTimeout;			// the time a BmSmtp will allow to pass
													// before reacting on any user-action 
													// (like closing the window)

	static const int32 NetBufSize = 2048;

	BmRef<BmSmtpAccount> mSmtpAccount;	// Info about our smtp-account

	BNetEndpoint* mSmtpServer;				// network-connection to SMTP-server
	bool mConnected;							// are we connected to the server?

	BmString mAnswer;							// holds last answer of SMTP-server

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

	// function that asks user for a password:
	BmPwdAcquisitorFunc* mPwdAcquisitorFunc;

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
	void Connect();
	void Helo();
	void AuthViaPopServer();
	void Auth();
	void SendMails();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void Mail( BmMail *mail);
	bool HasStdRcpts( BmMail *mail, BmRcptVect& rcptVect);
	void Rcpt( const BmRcptVect& rcptVect);
	void BccRcpt( BmMail *mail, bool sendDataForEachBcc, 
					  const BmString& headerText, BmStringIBuf& bodyText);
	void Data( BmMail *mail, const BmString& headerText, BmStringIBuf& bodyText,
				  BmString forBcc="");
	void UpdateSMTPStatus( const float, const char*, bool failed=false, bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);
	void StoreAnswer( char* );
	bool CheckForPositiveAnswer();
	bool GetAnswer();
	int32 ReceiveBlock( char* buffer, int32 max);
	void SendCommand( BmString& cmd, bool isSecret=false, bool isMailData=false);

	bool mServerMayHaveSizeLimit;
	bool mServerSupportsDSN;
	BmString mSupportedAuthTypes;

	// Hide copy-constructor and assignment:
	BmSmtp( const BmSmtp&);
	BmSmtp operator=( const BmSmtp&);
};

#endif
