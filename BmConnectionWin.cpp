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

static rgb_color BM_COL_STATUSBAR = {128,128,128};

//---------------------------------------
bool BmConnectionWin::IsConnectionWinAlive = false;

static bool IsConnectionWinAlive() {
	return BmConnectionWin::IsConnectionWinAlive;
}

//---------------------------------------
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

	BmConnectionWin::IsConnectionWinAlive = true;
}

//---------------------------------------
BmConnectionWin::~BmConnectionWin() {
	if (mInvokingLooper) 
		mInvokingLooper->PostMessage( BM_POPWIN_DONE);
	BmConnectionWin::IsConnectionWinAlive = false;
}

//---------------------------------------
bool BmConnectionWin::QuitRequested() {
	return true;
}

//---------------------------------------
void BmConnectionWin::MessageReceived(BMessage *msg) {
	switch( msg->what) {
		case BM_POPWIN_FETCHMSGS: {
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
			RemovePopper( FindMsgString( msg, BmPopper::MSG_POPPER));
			if (mActiveConnections.empty()) {
//				Hide();
			}
			break;
		}
		case BM_POP_UPDATE_STATE:
		case BM_POP_UPDATE_MAILS: {
			UpdatePopperInterface( msg);
			break;
		}
		default:
			BWindow::MessageReceived( msg);
	}
}

//---------------------------------------
void BmConnectionWin::AddPopper( BmPopAccount *account) {
	assert( account);
	char tname[B_OS_NAME_LENGTH+1];
	const char* name = account->Name().c_str();
	BStatusBar* statBar;
	BStatusBar* mailBar;
	MView* interface;

	// ...create a new thread for the Popper...
	BmPopperInfo* popperInfo = new BmPopperInfo( account, account->Name(), this, &::IsConnectionWinAlive);
	sprintf( tname, "BmPopper%ld", BmPopper::NextID());
	thread_id t_id = spawn_thread( &BmPopper::NewPopper, tname, 
											 B_NORMAL_PRIORITY, popperInfo);
	if (t_id < 0)
		throw runtime_error("AddPopper(): Could not create new popper-thread");

	ConnectionMap::iterator popperIter = mActiveConnections.find( account->Name());
	if (popperIter == mActiveConnections.end())
	{	// account is inactive, so we create a new interface for it...
		interface = AddPopperInterface( name, statBar, mailBar);
		// ...note the thread's id and corresponding view inside the map...
		mActiveConnections[account->Name()] = new BmConnectionWinInfo(t_id, interface, statBar, mailBar);
	} else {
		// store new thread id:
		BmConnectionWinInfo *popperInfo = ((*popperIter).second);
		popperInfo->thread = t_id;
		popperInfo->statBar->Reset( "State: ", name);
		popperInfo->mailBar->Reset( "Messages: ", NULL);
	}

	// ...and activate the Popper:
	resume_thread( t_id);
}

//---------------------------------------
MView *BmConnectionWin::AddPopperInterface( const char* name, BStatusBar* &statBar, BStatusBar* &mailBar) {
	BAutolock lock( this);
	if (lock.IsLocked()) {
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
	return NULL;
}

//-------------------------------------------------
void BmConnectionWin::UpdatePopperInterface( BMessage* msg) {
	const char* name = FindMsgString( msg, BmPopper::MSG_POPPER);
	float delta = FindMsgFloat( msg, BmPopper::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmPopper::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmPopper::MSG_TRAILING, &trailing);

	ConnectionMap::iterator popperIter = mActiveConnections.find( name);
	if (popperIter == mActiveConnections.end())
		return;		// account is not active, nothing to do...
	BmConnectionWinInfo *popperInfo = (*popperIter).second;
	BAutolock lock( this);
	if (lock.IsLocked()) {
		if (msg->what == BM_POP_UPDATE_STATE) {
			popperInfo->statBar->Update( delta, leading, trailing);
		} else { 
			popperInfo->mailBar->Update( delta, leading, trailing);
		}
	} else
		throw runtime_error("UpdatePopperInterface(): could not lock window");
}

//---------------------------------------
void BmConnectionWin::RemovePopper( const char* name) {
	assert( name);

	ConnectionMap::iterator popperIter = mActiveConnections.find( name);
	if (popperIter == mActiveConnections.end())
		return;					// account is not active, nothing to do...
	
	if (Beam::Prefs->DynamicConnectionWin()) {
		BmConnectionWinInfo *popperInfo = (*popperIter).second;
		RemovePopperInterface( popperInfo);
									// account is found, so we remove its interface...
		mActiveConnections.erase( popperIter);
									// ...and remove the info about this popper from our map
		delete popperInfo;
	}
}

//---------------------------------------
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
