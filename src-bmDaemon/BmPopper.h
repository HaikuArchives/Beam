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

#include <Message.h>

#include "BmNetJobModel.h"

class BmPopAccount;

enum {
	BM_POPPER_NEEDS_PWD						= 'bmPp'
};

/*------------------------------------------------------------------------------*\
	BmPopStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class BmPopStatusFilter : public BmStatusFilter {
	typedef BmStatusFilter inherited;

public:
	BmPopStatusFilter( BmMemIBuf* input, BmNetJobModel* job, 
							 uint32 blockSize=65536);

	// overrides of BmStatusFilter base:
	bool CheckForPositiveAnswer();

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	BmNetJobModel* mJob;

};

/*------------------------------------------------------------------------------*\
	BmPopper
		-	implements the POP-client
		-	each instance represents a single connection to a specific POP-account
		-	in general, each BmPopper is started as a thread which exits when the
			POP-session has ended
\*------------------------------------------------------------------------------*/
class BmPopper : public BmNetJobModel {
	typedef BmNetJobModel inherited;
	
public:
	//	message component definitions for status-msgs:
	static const char* const MSG_POPPER;
	static const char* const MSG_DELTA;
	static const char* const MSG_TRAILING;
	static const char* const MSG_LEADING;

	// message component definitions for additional info:
	static const char* const MSG_PWD;

	// alternate job-specifiers:
	static const int32 BM_AUTH_ONLY_JOB;
							// for authentication only (needed for SMTP-after-POP)
	static const int32 BM_CHECK_AUTH_TYPES_JOB;
							// to find out about supported authentication types

	BmPopper( const BmString& name, BmPopAccount* account);
	virtual ~BmPopper();

	BmString SuggestAuthType() const;
	inline static int32 NextID() 			{ return ++mId; }
	inline BmString Name() const			{ return ModelName(); }

	// overrides of netjob-model base:
	void UpdateProgress( uint32 numBytes);

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

private:
	// internal functions:
	void StateConnect();
	void StateLogin();
	void StateCheck();
	void StateRetrieve();
	void StateDisconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdatePOPStatus( const float, const char*, bool failed=false, bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);

	static int32 mId;							// unique message ID, this is used if a 
													// received message has no UID.

	BmRef<BmPopAccount> mPopAccount;		// Info about our pop-account

	vector<BmString> mMsgUIDs;				// array of unique-IDs, one for each message
	int32 mMsgCount;							// number of msgs found on server

	int32 mCurrMailNr;						// nr of currently handled mail (0 if none)
	int32 mNewMsgCount;						// number of msgs to be received
	vector<int32> mNewMsgSizes;			// sizes of msgs to be received
	int32 mNewMsgTotalSize;					// total-size of msgs to be received

	BmString mServerTimestamp;				// optional timestamp from Server (needed for APOP)

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
