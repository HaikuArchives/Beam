/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmImap_h
#define _BmImap_h

#include <memory>

#include <Message.h>

#include "BmDaemon.h"

#include "BmNetJobModel.h"

class BmImapAccount;

enum {
	BM_IMAP_NEEDS_PWD	= 'bmIp'
};

/*------------------------------------------------------------------------------*\
	BmImapStatusFilter
		-	
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmImapStatusFilter : public BmStatusFilter {
	typedef BmStatusFilter inherited;

public:
	BmImapStatusFilter( BmMemIBuf* input, BmNetJobModel* job, 
							  uint32 blockSize=65536);

	// overrides of BmStatusFilter base:
	bool CheckForPositiveAnswer();

protected:
	// overrides of BmMailFilter base:
	void Filter( const char* srcBuf, uint32& srcLen, 
					 char* destBuf, uint32& destLen);

	BmNetJobModel* mJob;
	BmRingBuf mLineBuf;
	BmString mLastStatusLine;
};

/*------------------------------------------------------------------------------*\
	BmImap
		-	implements the (currently rather simplistic) IMAP-client
		-	each instance represents a single connection to a specific IMAP-account
		-	in general, each BmImap is started as a thread which exits when the
			IMAP-session has ended
\*------------------------------------------------------------------------------*/
class IMPEXPBMDAEMON BmImap : public BmNetJobModel {
	typedef BmNetJobModel inherited;
	
public:
	// message component definitions for additional info:
	static const char* const MSG_PWD;

	// alternate job-specifiers:
	static const int32 BM_CHECK_CAPABILITIES_JOB;
							// to find out about supported authentication types

	BmImap( const BmString& name, BmImapAccount* account);
	virtual ~BmImap();

	bool SupportsTLS() const;
	BmString SuggestAuthType() const;

	inline static int32 NextID() 			{ return ++mId; }
	inline BmString Name() const			{ return ModelName(); }

	// overrides of netjob-model base:
	void UpdateProgress( uint32 numBytes);

	// overrides of job-model base:
	bool StartJob();
	bool ShouldContinue();

	// message components used for info-msgs (communication between
	// protocol implementation and protocol-specific status filter):
	static const char* const IMSG_NEEDED_TAG;

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
	void UpdateIMAPStatus( const float, const char*, bool failed=false, 
								 bool stopped=false);
	void UpdateMailStatus( const float, const char*, int32);
	bool CheckForTaggedPositiveAnswer( uint32 expectedSize=4096, 
												  bool update=false);
	void TagAndSendCommand( const BmString& cmd, 
									const BmString& secret=BM_DEFAULT_STRING,
									bool dotstuffEncoding=false,
									bool update=false);

	static int32 mId;
							// unique message ID, this is used if a 
							// received message has no UID.
	BmRef<BmImapAccount> mImapAccount;
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
	BmString mSupportedAuthTypes;
							// list of auth-types the server indicates to support
	bool mServerSupportsTLS;
							// whether or not the server knows about STLS
	int32 mState;		
							// current IMAP-state (refer enum below)
	enum States {
		IMAP_CONNECT = 0,
		IMAP_CAPA,
		IMAP_STARTTLS,
		IMAP_AUTH,
		IMAP_CHECK,
		IMAP_RETRIEVE,
		IMAP_DISCONNECT,
		IMAP_DONE,
		IMAP_FINAL
	};

	int32 mCurrTagNr;
							// current tag number, as required by IMAP
	BmString mCurrTag;
							// current tag identifier, (last one sent to server)

	// stuff needed for internal IMAP-state-loop:
	typedef void (BmImap::*TStateMethod)();
	struct ImapState {
		const char* text;
		TStateMethod func;
		bool skip;
		ImapState( const char* t, TStateMethod f) 
			: text(t)
			, func(f) 
			, skip(false)						{ }
	};
	static ImapState ImapStates[IMAP_FINAL];

	// Hide copy-constructor and assignment:
	BmImap( const BmImap&);
	BmImap operator=( const BmImap&);

};

#endif
