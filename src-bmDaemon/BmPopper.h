/*
	BmPopper.h

		$Id$
*/

#ifndef _BmPopper_h
#define _BmPopper_h

#include <memory>

#include <Message.h>
#include <NetAddress.h>
#include <NetEndpoint.h>

#include "BmDataModel.h"
#include "BmUtil.h"

class BmPopAccount;

/*------------------------------------------------------------------------------*\
	BmPopperInfo
		-	a BmPopper begins life with a pointer to this structure.
		-	this struct is used to push the neccessary info through 
			a thread-entry-function's data-ptr
			
\*------------------------------------------------------------------------------*/
struct BmPopperInfo {
	BmPopAccount* account;
							// the POP-account we have to deal with
	BString name;
							// name of this POP-session (used in GUI and for logging purposes)

	BmPopperInfo( BmPopAccount* a, const BString &n)
			: account(a)
			, name(n)
			{}
};

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

	BmPopper( const BString& name, BmPopAccount* account);
	virtual ~BmPopper();

	static int32 NextID() 					{ return ++mId; }

	BString Name() const						{ return ModelName(); }

	void StartJob();

private:
	static int32 mId;							// unique message ID, this is used if a 
													// received message has no UID.
	static int32 FeedbackTimeout;			// the time a BmPopper will allow to pass
													// before reacting on any user-action 
													// (like closing the window)

	static const bool SINGLE_LINE = true;
	static const bool MULTI_LINE = false;
	static const int32 NetBufSize = 16384;

	BmPtr<BmPopAccount> mPopAccount;		// Info about our pop-account

	BNetEndpoint mPopServer;				// network-connection to POP-server
	bool mConnected;							// are we connected to the server?

	BString* mMsgUIDs;						// array of unique-IDs, one for each message
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

	// private functions:
	void Connect();
	void Login();
	void Check();
	void Retrieve();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdatePOPStatus( const float, const char*, bool failed=false);
	void UpdateMailStatus( const float, const char*, int32);
	void StoreAnswer( char* );
	bool CheckForPositiveAnswer( bool SingleLineMode, int32 mailNr=0);
	bool GetAnswer( bool SingleLineMode, int32 mailNr = 0);
	int32 ReceiveBlock( char* buffer, int32 max);
	void SendCommand( BString cmd);
	void TellWeAreDone();
};

#endif
