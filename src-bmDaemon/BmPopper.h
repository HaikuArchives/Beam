/*
	BmPopper.h

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


#ifndef _BmPopper_h
#define _BmPopper_h

#include <memory>

#include <socket.h>
#ifdef BEAM_FOR_BONE
# include <netinet/in.h>
#endif

#include <Message.h>
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "BmDataModel.h"

class BmPopAccount;

#define BM_POPPER_NEEDS_PWD						'bmPp'

/*------------------------------------------------------------------------------*\
	BmPopper
		-	implements the POP-client
		-	each instance represents a single connection to a specific POP-account
		-	in general, each BmPopper is started as a thread which exits when the
			POP-session has ended
\*------------------------------------------------------------------------------*/
class BmPopper : public BmJobModel {
	typedef BmJobModel inherited;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_POPPER = 		"bm:popper";
	static const char* const MSG_DELTA = 		"bm:delta";
	static const char* const MSG_TRAILING = 	"bm:trailing";
	static const char* const MSG_LEADING = 	"bm:leading";

	// message component definitions for additional info:
	static const char* const MSG_PWD = 	"bm:pwd";

	// alternate job-specifiers:
	static const int32 BM_AUTH_ONLY_JOB = 1;
							// for authentication only (needed for SMTP-after-POP)
	static const int32 BM_CHECK_AUTH_TYPES_JOB = 2;
							// to find out about supported authentication types

	BmPopper( const BString& name, BmPopAccount* account);
	virtual ~BmPopper();

	BString SuggestAuthType() const;
	typedef bool BmPwdAcquisitorFunc( const BString, BString&);
	inline void SetPwdAcquisitorFunc( BmPwdAcquisitorFunc* func)
													{ mPwdAcquisitorFunc = func; }
	inline static int32 NextID() 			{ return ++mId; }
	inline BString Name() const			{ return ModelName(); }

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

private:
	// internal functions:
	void Connect();
	void Login();
	void Check();
	void Retrieve();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdatePOPStatus( const float, const char*, bool failed=false, bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);
	void StoreAnswer( char* );
	bool CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr=0);
	bool GetAnswer( bool SingleLineMode, int32 mailNr = 0);
	int32 ReceiveBlock( char* buffer, int32 max);
	void SendCommand( BString cmd, BString secret="");

	static int32 mId;							// unique message ID, this is used if a 
													// received message has no UID.
	static int32 FeedbackTimeout;			// the time a BmPopper will allow to pass
													// before reacting on any user-action 
													// (like closing the window)

	static const bool SINGLE_LINE = true;
	static const bool MULTI_LINE = false;
	static const int32 NetBufSize = 16384;

	BmRef<BmPopAccount> mPopAccount;		// Info about our pop-account

	BNetEndpoint* mPopServer;				// network-connection to POP-server
	bool mConnected;							// are we connected to the server?

	BString* mMsgUIDs;						// array of unique-IDs, one for each message
	int32 mMsgCount;							// number of msgs found on server
	int32 mNewMsgCount;						// number of msgs to be received
	int32* mMsgSizes;							// size of msgs to be received
	int32 mMsgTotalSize;						// total-size of msgs to be received
	BString mAnswer;							// holds last answer of POP-server
	BString mReplyLine;						// holds last server-reply (the answer's first line)
	BString mServerTimestamp;				// optional timestamp from Server (needed for APOP)

	int32 mState;								// current POP3-state (refer enum below)
	enum States {
		POP_CONNECT = 0,
		POP_LOGIN,
		POP_CHECK,
		POP_RETRIEVE,
		POP_DISCONNECT,
		POP_DONE,
		POP_FINAL
	};

	// function that asks user for a password:
	BmPwdAcquisitorFunc* mPwdAcquisitorFunc;

	// stuff needed for internal POP3-state-loop:
	typedef void (BmPopper::*TStateMethod)();
	struct PopState {
		const char* text;
		TStateMethod func;
		PopState( const char* t, TStateMethod f) : text(t), func(f) { }
	};
	static PopState PopStates[POP_FINAL];

	// Hide copy-constructor and assignment:
	BmPopper( const BmPopper&);
	BmPopper operator=( const BmPopper&);

};

#endif
