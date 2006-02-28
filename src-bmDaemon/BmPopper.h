/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPopper_h
#define _BmPopper_h

#include <memory>

#include <Message.h>

#include "BmDaemon.h"

#include "BmNetJobModel.h"

class BmPopAccount;

enum {
	BM_POPPER_NEEDS_PWD						= 'bmPp'
};

/*------------------------------------------------------------------------------*\
	BmPopStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmPopStatusFilter : public BmStatusFilter {
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
class IMPEXPBMDAEMON BmPopper : public BmNetJobModel {
	typedef BmNetJobModel inherited;
	
public:
	// message component definitions for additional info:
	static const char* const MSG_PWD;

	// alternate job-specifiers:
	static const int32 BM_AUTH_ONLY_JOB;
							// for authentication only (needed for SMTP-after-POP)
	static const int32 BM_CHECK_CAPABILITIES_JOB;
							// to find out about supported authentication types

	BmPopper( const BmString& name, BmPopAccount* account);
	virtual ~BmPopper();

	bool SupportsTLS() const;
	BmString SuggestAuthType() const;

	inline static int32 NextID() 			{ return ++mId; }
	inline BmString Name() const			{ return ModelName(); }

	// overrides of netjob-model base:
	void UpdateProgress( uint32 numBytes);
	void SetupAdditionalInfo( BMessage* additionalInfo);

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

private:
	// overrides of netjob-model base:
	void ExtractBase64(const BmString& text, BmString& base64);

	// internal functions:
	void StateConnect();
	void StateCapa();
	void StateStartTLS();
	void StateAuth();
	void StateCheck();
	void StateRetrieve();
	void StateDisconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdatePOPStatus( const float, const char*, bool failed=false, 
								 bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);

	static int32 mId;
							// unique message ID, this is used if a 
							// received message has no UID.
	BmRef<BmPopAccount> mPopAccount;
							// Info about our pop-account
	vector<BmString> mMsgUIDs;
							// array of unique-IDs, one for each message
	int32 mMsgCount;
							// number of msgs found on server
	int32 mCurrMailNr;
							// nr of currently handled mail (0 if none)
	int32 mNewMsgCount;
							// number of msgs to be received
	vector<int32> mNewMsgSizes;
							// sizes of msgs to be received
	int32 mNewMsgTotalSize;
							// total-size of msgs to be received
	BmString mServerTimestamp;
							// optional timestamp from Server (needed for APOP)
	BmString mSupportedAuthTypes;
							// list of auth-types the server indicates to support
	bool mServerSupportsTLS;
							// whether or not the server knows about STLS
	int32 mState;		
							// current POP3-state (refer enum below)
	enum States {
		POP_CONNECT = 0,
		POP_CAPA,
		POP_STARTTLS,
		POP_AUTH,
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
		bool skip;
		PopState( const char* t, TStateMethod f) 
			: text(t)
			, func(f) 
			, skip(false)						{ }
	};
	static PopState PopStates[POP_FINAL];

	// Hide copy-constructor and assignment:
	BmPopper( const BmPopper&);
	BmPopper operator=( const BmPopper&);

};

#endif
