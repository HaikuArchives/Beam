/*
	BmConnectionWin.cpp
		$Id$
*/

#include <stdio.h>

#include <Alert.h>
#include <Application.h>
#include <Autolock.h>
#include <ClassInfo.h>
#include <InterfaceDefs.h>
#include <StatusBar.h>

#include <layout.h>
#include <MBorder.h>
#include <MButton.h>
#include <MBViewWrapper.h>
#include <Space.h>

#include "BmConnectionWin.h"

/*------------------------------------------------------------------------------*\
	the color used for status-bar's gauge:
\*------------------------------------------------------------------------------*/
const rgb_color BmConnectionWin::BM_COL_STATUSBAR = {128,128,128};

/*------------------------------------------------------------------------------*\
	flag and access-function that indicate a user's request-to-stop:
\*------------------------------------------------------------------------------*/
bool BmConnectionWin::ConnectionWinAlive = false;
bool BmConnectionWin::IsConnectionWinAlive() {
	return BmConnectionWin::IsConnectionWinAlive;
}

/*------------------------------------------------------------------------------*\
	constructor
		-	creates outer view that will take up the connection-interfaces
\*------------------------------------------------------------------------------*/
BmConnectionWin::BmConnectionWin( const char* title, BLooper *invoker)
	: MWindow( BRect(50,50,0,0), title,
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					MyWinFlags)
	, mInvokingLooper( invoker)
{ 
	mOuterGroup = 
		new VGroup(
			new Space( minimax( 300,0,1E5,1E5,1)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));

	BmConnectionWin::ConnectionWinAlive = true;
}

/*------------------------------------------------------------------------------*\
	destructor
		-	FIXME: needs to free memory if necessary!
		-	tells interested party that we are finished
\*------------------------------------------------------------------------------*/
BmConnectionWin::~BmConnectionWin() {
	if (mInvokingLooper) 
		mInvokingLooper->PostMessage( BM_POPWIN_DONE);
	BmConnectionWin::ConnectionWinAlive = false;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmConnectionWin::QuitRequested() {
	BmConnectionWin::ConnectionWinAlive = false;
	return true;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles messages sent from Application (connection requests)
			and messages sent from connections (update-requests and 
			finished-triggers)
\*------------------------------------------------------------------------------*/
void BmConnectionWin::MessageReceived(BMessage *msg) {
	switch( msg->what) {
		case BM_POPWIN_FETCHMSGS: {
			// request to start a new POP3-connection
			BArchivable *obj;
			BmPopAccount *account;
			obj = instantiate_object( msg);
			if (obj && (account = cast_as( obj, BmPopAccount))) {
				AddPopper( account);
				if (IsHidden())
					Show();
			} else {
				throw invalid_argument( "Could not create BmPopAccount-instance from message of type MSG_FETCHMSGS");
							// Illegal message data !?!
			}
			break;
		}
		case BM_POP_DONE: {
			// a POP3-connection tells us that it has finished its job
			RemovePopper( FindMsgString( msg, BmPopper::MSG_POPPER));
			if (mActiveConnections.empty()) {
				Hide();
			}
			break;
		}
		case BM_POP_UPDATE_STATE:
		case BM_POP_UPDATE_MAILS: {
			// a POP3-connection wants to update its interface
			UpdatePopperInterface( msg);
			break;
		}
		default:
			BWindow::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	AddPopper( account)
		-	adds a new pop3-connection-interface to this window
		-	the necessary BmPopper-object will be created and started
			in a new thread
		-	if connection is already active, it is left alone (nothing happens)
\*------------------------------------------------------------------------------*/
void BmConnectionWin::AddPopper( BmPopAccount *account) {
	assert( account);
	char tname[B_OS_NAME_LENGTH+1];
	const char* name = account->Name().String();
	BStatusBar* statBar;
	BStatusBar* mailBar;
	MView* interface;

	ConnectionMap::iterator interfaceIter = mActiveConnections.find( account->Name());
	if (interfaceIter != mActiveConnections.end()) {
		// interface-info found, maybe this popper is already active:
		BmConnectionWinInfo *interfaceInfo = ((*interfaceIter).second);
		thread_info ti;
		int32 res;
		if ((res=get_thread_info( interfaceInfo->thread, &ti)) == B_OK) {
			// thread is still running, so we better don't disturb:
			return;
		} else {
		}
	}

	// we create a new thread for the Popper...
	BmPopperInfo* popperInfo = new BmPopperInfo( account, account->Name(), this, &IsConnectionWinAlive);
	sprintf( tname, "BmPopper%ld", BmPopper::NextID());
	thread_id t_id = spawn_thread( &BmPopper::NewPopper, tname, 
											 B_NORMAL_PRIORITY, popperInfo);
	if (t_id < 0)
		throw runtime_error("AddPopper(): Could not create new popper-thread");

	if (interfaceIter == mActiveConnections.end())
	{	// account is inactive, so we create a new interface for it...
		interface = AddPopperInterface( name, statBar, mailBar);
		// ...note the thread's id and corresponding view inside the map...
		mActiveConnections[account->Name()] = new BmConnectionWinInfo(t_id, interface, statBar, mailBar);
	} else {
		// we are in STATIC-mode, where inactive connections are shown. We just
		// have to reactivate the interface.
		// thus, we just store the new thread id...
		BmConnectionWinInfo *interfaceInfo = ((*interfaceIter).second);
		interfaceInfo->thread = t_id;
		// ...and reinitialize the BStatusBars:
		interfaceInfo->statBar->Reset( "State: ", name);
		interfaceInfo->mailBar->Reset( "Messages: ", NULL);
	}

	// finally, we activate the Popper:
	resume_thread( t_id);
}

/*------------------------------------------------------------------------------*\
	AddPopperInterface( name, statBar, mailBar)
		-	create a new interface for POP3-connections
		-	the statusBar and the mailBar are returned to the caller
\*------------------------------------------------------------------------------*/
MView *BmConnectionWin::AddPopperInterface( const char* name, BStatusBar* &statBar, BStatusBar* &mailBar) {
	BAutolock lock( this);
	if (lock.IsLocked()) {					// lock window for change of layout
		MView* newView = 
		new MBorder
		(
			M_ETCHED_BORDER, 3, (char*)(name),
			new VGroup(
				new MBViewWrapper(
					statBar = new BStatusBar( BRect(), name, "State: ", name), true, false, false
				),
				new MBViewWrapper(
					mailBar = new BStatusBar( BRect(), name, "Messages: ", ""), true, false, false
				),
				0
			)
		);
		statBar->SetBarHeight( 8.0);
		statBar->SetBarColor( BM_COL_STATUSBAR);
		mailBar->SetBarHeight( 8.0);
		mOuterGroup->AddChild( dynamic_cast<BView*>(newView));
		RecalcSize();
		return newView;
	}
	throw runtime_error("AddPopperInterface(): could not lock window");
}

/*------------------------------------------------------------------------------*\
	UpdatePopperInterface( msg)
		-	updates the popper-interface with new state-info received from BmPopper
		-	only one of statusBar/mailBar will be updated
\*------------------------------------------------------------------------------*/
void BmConnectionWin::UpdatePopperInterface( BMessage* msg) {
	const char* name = FindMsgString( msg, BmPopper::MSG_POPPER);
	float delta = FindMsgFloat( msg, BmPopper::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmPopper::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmPopper::MSG_TRAILING, &trailing);

	ConnectionMap::iterator interfaceIter = mActiveConnections.find( name);
	if (interfaceIter == mActiveConnections.end())
		return;		// account is not active, nothing to do...
	BmConnectionWinInfo *interfaceInfo = (*interfaceIter).second;
	BAutolock lock( this);
	if (lock.IsLocked()) {
		if (msg->what == BM_POP_UPDATE_STATE) {
			interfaceInfo->statBar->Update( delta, leading, trailing);
		} else { 
			// msg->what == BM_POP_UPDATE_MAILS
			interfaceInfo->mailBar->Update( delta, leading, trailing);
		}
	} else
		throw runtime_error("UpdatePopperInterface(): could not lock window");
}

/*------------------------------------------------------------------------------*\
	RemovePopper( name)
		-	removes a popper and its administrative info
		-	the decision about whether or not to remove the popper's interface, too, 
			depends on the mode of the connection-window:
			*	CONN_WIN_DYNAMIC:			always remove interface
			*	CONN_WIN_DYNAMIC_EMPTY:	remove interface if no new mail was found
			*	CONN_WIN_STATIC:			never remove interface
\*------------------------------------------------------------------------------*/
void BmConnectionWin::RemovePopper( const char* name) {
	assert( name);

	ConnectionMap::iterator interfaceIter = mActiveConnections.find( name);
	if (interfaceIter == mActiveConnections.end())
		return;									// account is not active, nothing to do...
	
	BmConnectionWinInfo *interfaceInfo = (*interfaceIter).second;
	if (!interfaceInfo)
		return;

	// remove interface only if mode indicates to do so:
	if (Beam::Prefs->DynamicConnectionWin() == BmPrefs::CONN_WIN_DYNAMIC 
	|| (Beam::Prefs->DynamicConnectionWin() == BmPrefs::CONN_WIN_DYNAMIC_EMPTY
		&& interfaceInfo->mailBar->CurrentValue()==0)) {
		RemovePopperInterface( interfaceInfo);
							// account is found, so we remove its interface...
		mActiveConnections.erase( interfaceIter);
							// ...and remove the info about this popper from our map
		delete interfaceInfo;
	}
}

/*------------------------------------------------------------------------------*\
	RemovePopperInterface( popperInfo)
		-	removes the popper's interface from this window
\*------------------------------------------------------------------------------*/
void BmConnectionWin::RemovePopperInterface( BmConnectionWinInfo* popperInfo) {
	BAutolock lock( this);
	if (lock.IsLocked()) {
		BView *view = dynamic_cast<BView*>(popperInfo->interface);
		if (view) {
			BRect rect = view->Bounds();
			mOuterGroup->RemoveChild( view);
			ResizeBy( 0, 0-(rect.Height()));
			RecalcSize();
			delete view;
			return;
		}
		throw invalid_argument("RemovePopperInterface(): received view of incorrect type");
	}
	throw runtime_error("RemovePopperInterface(): could not lock window");
}
