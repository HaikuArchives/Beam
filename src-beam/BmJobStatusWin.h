/*
	BmJobStatusWin.h
	
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


#ifndef _BmJobStatusWin_h
#define _BmJobStatusWin_h

#include <map>

#include <MBorder.h>
#include <VGroup.h>

#include "BmController.h"
#include "BmWindow.h"

/*------------------------------------------------------------------------------*\
	BmJobStatusView
		-	controls a specific connection
\*------------------------------------------------------------------------------*/
class BmJobStatusView : public MBorder, public BmJobController {
	typedef MBorder inheritedView;
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
	inline int MSecsBeforeShow()			{ return MAX(0,mMSecsBeforeShow); }
	inline int MSecsBeforeRemove()		{ return MAX(0,mMSecsBeforeRemove); }

protected:
	int mMSecsBeforeShow;
	int mMSecsBeforeRemove;
	
private:
	BMessageRunner* mShowMsgRunner;
	BMessageRunner* mRemoveMsgRunner;
	
	// Hide copy-constructor and assignment:
	BmJobStatusView( const BmJobStatusView&);
	BmJobStatusView operator=( const BmJobStatusView&);
};

class MStringView;
/*------------------------------------------------------------------------------*\
	BmMailMoverView
		-	controls a specific mail-moving operation
\*------------------------------------------------------------------------------*/
class BmMailMoverView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

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

private:
	BStatusBar* mStatBar;					// shows number of mails moved during this operation
	MStringView* mBottomLabel;

	// Hide copy-constructor and assignment:
	BmMailMoverView( const BmMailMoverView&);
	BmMailMoverView operator=( const BmMailMoverView&);
};

/*------------------------------------------------------------------------------*\
	BmPopperView
		-	controls a specific POP3-connection
\*------------------------------------------------------------------------------*/
class BmPopperView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

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

	// class-functions:
	static bool AskUserForPwd( const BString accName, BString& pwd);

private:
	BStatusBar* mStatBar;				// shows current status of this connection
	BStatusBar* mMailBar;				// shows number of mails handled by this connection

	// Hide copy-constructor and assignment:
	BmPopperView( const BmPopperView&);
	BmPopperView operator=( const BmPopperView&);
};

/*------------------------------------------------------------------------------*\
	BmSmtpView
		-	controls a specific SMTP-connection
\*------------------------------------------------------------------------------*/
class BmSmtpView : public BmJobStatusView {
	typedef BmJobStatusView inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmSmtpView* CreateInstance( const char* name);
	BmSmtpView( const char* name);
	~BmSmtpView();

	// overrides of jobstatusview base:
	void ResetController();
	void UpdateModelView( BMessage* msg);
	BmJobModel* CreateJobModel( BMessage* msg);

	// overrides of controller base:
	BHandler* GetControllerHandler() 	{ return this; }

	// class-functions:
	static bool AskUserForPwd( const BString accName, BString& pwd);

private:
	BStatusBar* mStatBar;				// shows current status of this connection
	BStatusBar* mMailBar;				// shows number of mails handled by this connection

	// Hide copy-constructor and assignment:
	BmSmtpView( const BmSmtpView&);
	BmSmtpView operator=( const BmSmtpView&);
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
class BmJobStatusWin : public BmWindow {
	typedef BmWindow inherited;

	friend class BmJobStatusView;

	typedef map<BString, BmJobStatusView*> JobMap;

public:
	// creator-func, c'tors and d'tor:
	static BmJobStatusWin* CreateInstance();
	BmJobStatusWin();
	virtual ~BmJobStatusWin();

	// overrides of BWindow base:
	bool QuitRequested();
	void Quit();
	void MessageReceived( BMessage* msg);

	//
	static const rgb_color BM_COL_STATUSBAR;

	static BmJobStatusWin* theInstance;

private:
	void AddJob( BMessage* msg);
	void RemoveJob( const char* name);

	JobMap mActiveJobs;						// list of known jobs
	VGroup* mOuterGroup;						// the outmost view that the connection-interfaces live in
	BLooper* mInvokingLooper;				// the looper we will tell that we are finished

	// Hide copy-constructor and assignment:
	BmJobStatusWin( const BmJobStatusWin&);
	BmJobStatusWin operator=( const BmJobStatusWin&);
};

#define TheJobStatusWin BmJobStatusWin::theInstance


#endif
