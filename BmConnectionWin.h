/*
	BmConnectionWin.h
	
		$Id$
*/

#ifndef _BmPopperWin_h
#define _BmPopperWin_h

#include <map>

#include <liblayout/MWindow.h>
#include <liblayout/VGroup.h>

#include "BmPopper.h"

/*------------------------------------------------------------------------------*\
	types of message sent to/from a BmConnectionWin
\*------------------------------------------------------------------------------*/
#define BM_POPWIN_FETCHMSGS		'bmca'
						// sent from App BmConnectionWin in order to start connection
#define BM_POPWIN_DONE				'bmcb'
						// sent from BmConnectionWin to app when a connection has finished

/*------------------------------------------------------------------------------*\
	BmConnectionWin
		-	implements the connection-window, where the states of all active connections
			are displayed
		-	three different display-modes are available:
			* DYNAMIC:			Connections are only displayed as long as they are active,
									when no more connections is active, the window will close.
			* DYNAMIC_EMPTY:	Only POP-connections that actually received mail stay visible
									after the connection has ended
			* STATIC:			All connections are visible, even after close
		-	in general each connection to any server starts as a request to this class (better: the
			one and only instance of this class). For every requested connection a new
			interface is generated and displayed, then the connections is executed by the
			corresponding connection-object (for insance a BmPopper).
		-	connections will be executed inside a seperate thread.
\*------------------------------------------------------------------------------*/
class BmConnectionWin : public MWindow {
	typedef MWindow inherited;
	
	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

	static const rgb_color BM_COL_STATUSBAR;
	
	/*------------------------------------------------------------------------------*\
		BmConnectionWinInfo
			-	holds information about a specific connection
	\*------------------------------------------------------------------------------*/
	struct BmConnectionWinInfo {
		MView* interface;						// the interface for this connection
		thread_id thread;						// the thread the connection is running in
		BStatusBar* statBar;					// shows current status of this connection
		BStatusBar* mailBar;					// shows number of mails handled by this connection
		BmConnectionWinInfo( thread_id t, MView* i, BStatusBar* sb, BStatusBar* mb)
			: interface(i)
			, thread(t)
			, statBar(sb)
			, mailBar(mb)
			{}
	};
	typedef map<BString, BmConnectionWinInfo*> ConnectionMap;

	// the following flag indicates, whether the user has requested a termination of
	// the connections. All active connections check this flag periodically in order to
	// find out if they should continue or not:
	static bool ConnectionWinAlive;
public:
	static bool IsConnectionWinAlive();	// function used by connections for alive-check:

	BmConnectionWin( const char* title, BLooper *invoker=NULL);
	virtual ~BmConnectionWin();

	// standard BeOS-stuff:
	bool QuitRequested();
	void Quit();
	virtual void MessageReceived(BMessage *message);

private:
	ConnectionMap mActiveConnections;	// list of known connections (some may be inactive)
	VGroup *mOuterGroup;						// the outmost view that the connection-interfaces live in
	BLooper *mInvokingLooper;				// the looper we will tell that we are finished
	uint8 mActiveConnCount;					// number of currently active connections

	void AddPopper( BmPopAccount* popAccount);
	MView *AddPopperInterface( const char* name, BStatusBar*&, BStatusBar*&);
	void UpdatePopperInterface( BMessage* msg);
	void RemovePopper( const char* name);
	void RemovePopperInterface( BmConnectionWinInfo* interface);
};

#endif
