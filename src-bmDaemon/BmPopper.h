/*
	BmPopper.h		-	$Id$
*/

#ifndef _BmPopper_h
#define _BmPopper_h

#include <stdio.h>

#include <Message.h>
#include <NetAddress.h>
#include <NetEndpoint.h>
#include <StatusBar.h>
#include <String.h>
#include <Window.h>

#include "BmPopAccount.h"

// -----------------------------------------------
#define BM_POP_FETCHMSGS	'bmpa'
#define BM_POP_DONE			'bmpb'
#define BM_POP_UPDATE_STATE	'bmpc'
#define BM_POP_UPDATE_MAILS	'bmpd'

// -----------------------------------------------
struct BmPopperInfo {
	BmPopAccount* account;
	BString name;
	BLooper *statusLooper;
	bool (*aliveFunc)();
	BmPopperInfo( BmPopAccount* a, const BString &n, BLooper* sl, bool (*f)()) 
			: account(a)
			, name(n)
			, statusLooper(sl)
			, aliveFunc(f)
			{}
};

// -----------------------------------------------
class BmPopper {
public:
	static char* const MSG_POPPER = 			"bm:popper";
	static char* const MSG_DELTA = 			"bm:delta";
	static char* const MSG_TRAILING = 		"bm:trailing";
	static char* const MSG_LEADING = 		"bm:leading";
public:
	BmPopper( BmPopperInfo* info);
	virtual ~BmPopper();

	static int32 NewPopper( void* data);
	static int32 NextID() { return ++mId; }
	void Start();
private:
	static int32 mId;
	static int32 FeedbackTimeout;
	static int32 ReceiveTimeout;
	static int32 PasswordTimeout;

	static const bool SINGLE_LINE = true;
	static const bool MULTI_LINE = false;

	BmPopperInfo* mPopperInfo;
	BNetEndpoint mPopServer;
	int32 mState;
	char* mDataPtr;
	bool mConnected;
	BString* mMsgUIDs;
	int32 mMsgCount;
	BmLogfile log;

	void Connect();
	void Login();
	void Check();
	void Retrieve();
	void Update();
	void Disconnect();
	void Quit( bool WaitForAnswer=false);
	void UpdateStateBar( const float, const char*);
	void UpdateMailBar( const float, const char*, int32, int32);
	bool ShouldContinue();
	void StoreAnswer( char *);
	char* CheckForPositiveAnswer( bool SingleLineMode, int32 answerTimeout=ReceiveTimeout);
	char* GetAnswer( bool SingleLineMode, int32 answerTimeout=ReceiveTimeout);
	int32 ReceiveBlock( char* buffer, int32 max, int32 answerTimeout=ReceiveTimeout);
	void SendCommand( BString &cmd);
private:
	enum States {
		POP_CONNECT = 0,
		POP_LOGIN,
		POP_CHECK,
		POP_RETRIEVE,
		POP_UPDATE,
		POP_DISCONNECT,
		POP_FINAL
	};
	typedef void (BmPopper::*TStateMethod)();
	struct PopState {
		const char* text;
		TStateMethod func;
		PopState( const char* t, TStateMethod f) : text(t), func(f) { }
	};
	static PopState PopStates[POP_FINAL];
};

#endif
