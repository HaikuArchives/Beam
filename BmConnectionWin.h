/*
	BmConnectionWin.h
	
		$Id$
*/

#ifndef _BmPopperWin_h
#define _BmPopperWin_h

#include <map>

#include <MBorder.h>
#include <MWindow.h>
#include <VGroup.h>

#include "BmController.h"

class BmPopAccount;
class BmPopper;

/*------------------------------------------------------------------------------*\
	types of message sent to/from a BmConnectionWin
\*------------------------------------------------------------------------------*/
#define BM_CONNWIN_FETCHPOP		'bmca'
						// sent from App BmConnectionWin in order to start pop-connection
#define BM_CONNWIN_DONE				'bmcb'
						// sent from BmConnectionWin to app when a connection has finished

/*------------------------------------------------------------------------------*\
	BmConnectionView
		-	controls a specific connection
\*------------------------------------------------------------------------------*/
class BmConnectionView : public MBorder, public BmJobController {

public:

	//
	BmConnectionView( const char* name);
	virtual ~BmConnectionView();

	virtual BmJobModel* CreateJobModel( BMessage* msg) = 0;

	BHandler* GetControllerHandler() 	{ return this; }

	virtual bool WantsToStayVisible()	{ return false; }
	virtual void ResetController()		{ }
	virtual void UpdateModelView( BMessage* msg) = 0;

	virtual void MessageReceived( BMessage* msg);
	virtual void JobIsDone( bool completed);

};

/*------------------------------------------------------------------------------*\
	BmPopperView
		-	controls a specific POP3-connection
\*------------------------------------------------------------------------------*/
class BmPopperView : public BmConnectionView {

public:
	//
	static BmPopperView* CreateInstance( const char* name);

	//
	BmPopperView( const char* name);
	~BmPopperView();

	BmJobModel* CreateJobModel( BMessage* msg);

	BHandler* GetControllerHandler() 	{ return this; }
	bool WantsToStayVisible();
	void ResetController();

	void UpdateModelView( BMessage* msg);

private:
	BStatusBar* mStatBar;				// shows current status of this connection
	BStatusBar* mMailBar;				// shows number of mails handled by this connection

};

/*------------------------------------------------------------------------------*\
	BmConnectionWin
		-	implements the connection-window, where the states of all active connections
			are displayed
		-	three different display-modes are available:
			* DYNAMIC:			Connections are only displayed as long as they are active,
									when no more connections is active, the window will close.
			* DYNAMIC_EMPTY:	Only POP-connections that actually received mail stay visible
									after the connection has ended
			* STATIC:			All connections are visible, even after connection-close
		-	in general each connection to any server starts as a request to this class (better: the
			one and only instance of this class). For every requested connection a new
			interface is generated and displayed, then the connections is executed by the
			corresponding connection-object (for instance a BmPopper).
		-	connections and their interfaces are connected via the interface defined between
			BmJobController and BmJobModel.
\*------------------------------------------------------------------------------*/
class BmConnectionWin : public MWindow {
	typedef MWindow inherited;

	friend class BmConnectionView;

	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

	typedef map<BString, BmConnectionView*> ConnectionMap;

public:

	BmConnectionWin( const char* title, BLooper* invoker=NULL);
	virtual ~BmConnectionWin();

	// standard BeOS-stuff:
	bool QuitRequested();
	void Quit();
	virtual void MessageReceived( BMessage* msg);

	//
	static const rgb_color BM_COL_STATUSBAR;

	//	
	static BmConnectionWin* Instance;

	//	message component definitions:
	static const char* const MSG_CONN_NAME = 		"bm:connname";
	static const char* const MSG_CONN_INFO = 		"bm:conninfo";

private:
	//
	void AddConnection( BMessage* msg);
	void RemoveConnection( const char* name);

	ConnectionMap mActiveConnections;	// list of known connections (some may be inactive)
	VGroup* mOuterGroup;						// the outmost view that the connection-interfaces live in
	BLooper* mInvokingLooper;				// the looper we will tell that we are finished
	uint8 mActiveConnCount;					// number of currently active connections

};

#endif
