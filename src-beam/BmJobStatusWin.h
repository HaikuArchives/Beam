/*
	BmJobStatusWin.h
	
		$Id$
*/

#ifndef _BmJobStatusWin_h
#define _BmJobStatusWin_h

#include <map>

#include <MBorder.h>
#include <MWindow.h>
#include <VGroup.h>

#include "BmController.h"

class BmPopAccount;
class BmPopper;

/*------------------------------------------------------------------------------*\
	BmJobStatusView
		-	controls a specific connection
\*------------------------------------------------------------------------------*/
class BmJobStatusView : public MBorder, public BmJobController {
	typedef BmJobController inherited;
	
public:
	// c'tors and d'tor:
	BmJobStatusView( const char* name);
	virtual ~BmJobStatusView();

	// native methods:
	virtual bool AlwaysRemoveWhenDone()	{ return false; }
	virtual void ResetController()		{ }
	virtual void UpdateModelView( BMessage* msg) = 0;
	virtual BmJobModel* CreateJobModel( BMessage* msg) = 0;

	// overrides of controller base:
	void StartJob( BmJobModel* model = NULL, bool startInNewThread=true);
	BHandler* GetControllerHandler() 	{ return this; }
	void JobIsDone( bool completed);

	// overrides of view base:
	void MessageReceived( BMessage* msg);

	// getters:
	int MSecsBeforeShow()					{ return mMSecsBeforeShow; }
	int MSecsBeforeRemove()					{ return mMSecsBeforeRemove; }

protected:
	int mMSecsBeforeShow;
	int mMSecsBeforeRemove;
	
private:
	BMessageRunner* mShowMsgRunner;
	BMessageRunner* mRemoveMsgRunner;
	
};

class MStringView;
/*------------------------------------------------------------------------------*\
	BmMailMoverView
		-	controls a specific mail-moving operation
\*------------------------------------------------------------------------------*/
class BmMailMoverView : public BmJobStatusView {

public:
	// creator-func, c'tors and d'tor:
	static BmMailMoverView* CreateInstance( const char* name);
	BmMailMoverView( const char* name);
	~BmMailMoverView();

	// overrides of jobstatusview base:
	bool AlwaysRemoveWhenDone()			{ return true; }
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

	//	message component definitions:
	static const char* const MSG_REFS = 		"refs";
	static const char* const MSG_NAME = 		"refs";
	static const char* const MSG_FOLDER = 		"bm:folder";

private:
	BStatusBar* mStatBar;					// shows number of mails moved during this operation
	MStringView* mBottomLabel;
};

/*------------------------------------------------------------------------------*\
	BmPopperView
		-	controls a specific POP3-connection
\*------------------------------------------------------------------------------*/
class BmPopperView : public BmJobStatusView {

public:
	// creator-func, c'tors and d'tor:
	static BmPopperView* CreateInstance( const char* name);
	BmPopperView( const char* name);
	~BmPopperView();

	// overrides of jobstatusview base:
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

private:
	BStatusBar* mStatBar;				// shows current status of this connection
	BStatusBar* mMailBar;				// shows number of mails handled by this connection

};

/*------------------------------------------------------------------------------*\
	BmJobStatusWin
		-	implements the connection-window, where the states of all active connections
			are displayed
		-	three different display-modes are available:
			* DYNAMIC:			JobStatuss are only displayed as long as they are active,
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
class BmJobStatusWin : public MWindow {
	typedef MWindow inherited;

	friend class BmJobStatusView;

	static const uint32 MyWinFlags = B_ASYNCHRONOUS_CONTROLS
												|	B_NOT_ZOOMABLE
												|	B_NOT_RESIZABLE;

	typedef map<BString, BmJobStatusView*> JobMap;

public:
	// creator-func, c'tors and d'tor:
	static BmJobStatusWin* CreateInstance();
	BmJobStatusWin( const char* title);
	virtual ~BmJobStatusWin();

	// overrides of BWindow base:
	bool QuitRequested();
	void Quit();
	void MessageReceived( BMessage* msg);

	//
	static const rgb_color BM_COL_STATUSBAR;

	//	
	static BmJobStatusWin* Instance;

	//	message component definitions:
	static const char* const MSG_JOB_NAME = 		"bm:jobname";

	static BmJobStatusWin* theInstance;

private:
	void AddJob( BMessage* msg);
	void RemoveJob( const char* name);

	JobMap mActiveJobs;						// list of known jobs (some may be inactive)
	VGroup* mOuterGroup;						// the outmost view that the connection-interfaces live in
	BLooper* mInvokingLooper;				// the looper we will tell that we are finished

};

#define TheJobStatusWin BmJobStatusWin::theInstance


#endif
