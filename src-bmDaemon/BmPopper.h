/*
	BmPopper.h

		$Id$
*/

#ifndef _BmPopper_h
#define _BmPopper_h

#include <stdio.h>

#include <Message.h>
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "BmPrefs.h"
#include "BmPopAccount.h"

// message constants for BmPopper, all msgs are sent to the looper 
// specified via BmPopperInfo-struct:
#define BM_POP_UPDATE_STATE	'bmpa'
							// a new state has been entered during POP-session
#define BM_POP_UPDATE_MAILS	'bmpb'
							// a new mail is being dealt with during POP-session
#define BM_POP_DONE				'bmpc'
							// BmPopper has finished

/*------------------------------------------------------------------------------*\
	BmPopperInfo
		-	this structure represents a BmPopper's connection to the 
			outer world.
		-	a BmPopper begins life with a pointer to this structure,
			nothing else is required
			
\*------------------------------------------------------------------------------*/
struct BmPopperInfo {
	BmPopAccount* account;
							// the POP-account we have to deal with
	BString name;
							// name of this POP-session (used in GUI and for logging purposes)
	BLooper *statusLooper;
							// the looper that should receive status-messages from a BmPopper.
							// In Beam, this is the BmConnectionWin-looper. OPTIONAL
	bool (*aliveFunc)();
							// a bool-function that returns true as long as the BmPopper
							// should continue to run. OPTIONAL

	BmPopperInfo( BmPopAccount* a, const BString &n, BLooper* sl, bool (*f)()) 
			: account(a)
			, name(n)
			, statusLooper(sl)
			, aliveFunc(f)
			{}
};

/*------------------------------------------------------------------------------*\
	BmPopper
		-	implements the POP-client
		-	each instance represents a single connection to a specific POP-account
		-	in general, each BmPopper is started as a thread which exits when the
			POP-session has ended
\*------------------------------------------------------------------------------*/
class BmPopper {
public:
	//	message component definitions for status-msgs:
	static char* const MSG_POPPER = 		"bm:popper";
	static char* const MSG_DELTA = 		"bm:delta";
	static char* const MSG_TRAILING = 	"bm:trailing";
	static char* const MSG_LEADING = 	"bm:leading";

	BmPopper( BmPopperInfo* info);
	virtual ~BmPopper();

	static int32 NewPopper( void* data);
	static int32 NextID() 					{ return ++mId; }
	void Start();

private:
	static int32 mId;							// unique message ID, this is used if a 
													// received message has no UID.
	static int32 FeedbackTimeout;			// the time a BmPopper will allow to pass
													// before reacting on any user-action 
													// (like closing the window)

	static const bool SINGLE_LINE = true;
	static const bool MULTI_LINE = false;
	static const int32 NetBufSize = 16384;

	BmPopperInfo* mPopperInfo;				// configuration-info

	BNetEndpoint mPopServer;				// network-connection to POP-server
	bool mConnected;							// are we connected to the server?

	BString* mMsgUIDs;							// array of unique-IDs, one for each message
	int32 mMsgCount;							// number of msgs to be received
	int32 mMsgSize;							// size of current msg
	int32 mMsgTotalSize;						// size of all msgs to be received
	BString mAnswer;							// holds last answer of POP-server
	BString mReplyLine;						// holds last server-reply (the answer's first line)

	int32 mState;								// current POP3-state (refer enum below)
	enum States {
		POP_CONNECT = 0,
		POP_LOGIN,
		POP_CHECK,
		POP_RETRIEVE,
		POP_UPDATE,
		POP_DISCONNECT,
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

	// private functions:
	void Connect();
	void Login();
	void Check();
	void Retrieve();
	void Update();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdatePOPStatus( const float, const char*, bool failed=false);
	void UpdateMailStatus( const float, const char*, int32);
	bool ShouldContinue();
	void StoreAnswer( char *);
	void CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr=0);
	void GetAnswer( bool SingleLineMode, int32 mailNr = 0);
	int32 ReceiveBlock( char* buffer, int32 max);
	void SendCommand( BString cmd);
};

#endif
