/*
	BmJobStatusWin.cpp
		$Id$
*/

#include <stdio.h>

#include <Autolock.h>
#include <ClassInfo.h>
#include <InterfaceDefs.h>
#include <StatusBar.h>

#include <layout.h>
#include <MBorder.h>
#include <MButton.h>
#include <MBViewWrapper.h>
#include <Space.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"



/********************************************************************************\
	BmJobStatusView
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmJobStatusView()
		-	constructor
\*------------------------------------------------------------------------------*/
BmJobStatusView::BmJobStatusView( const char* name)
	:	MBorder( M_ETCHED_BORDER, 2, const_cast<char*>(name))
	,	BmJobController( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobStatusView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmJobStatusView::~BmJobStatusView() {
}

/*------------------------------------------------------------------------------*\
	UpdateModelView( msg)
		-	hook-method that is called when a JobModel wants to update its view
		-	parameter msg may contain any further attributes needed for update
		-	this default implementation simply does nothing
\*------------------------------------------------------------------------------*/
void BmJobStatusView::UpdateModelView( BMessage* msg) {
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmJobStatusView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOB_DONE: {
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			case BM_JOB_UPDATE_STATE: {
				// our Model wants to update its interface:
				UpdateModelView( msg);
				break;
			}
			default:
				MBorder::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("JobStatusView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	hook-method that is called when a JobModel tells us it's finished
		-	parameter msg may contain any further attributes
		-	tells the connection-window to remove this connection
		-	deletes this connection
\*------------------------------------------------------------------------------*/
void BmJobStatusView::JobIsDone( bool completed) {
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been told that job " << ModelName() << " is done");
	BmJobStatusWin::Instance->RemoveJobStatus( ControllerName());
	if (ThePrefs->DynamicJobStatusWin() == BmPrefs::CONN_WIN_DYNAMIC 
	|| (ThePrefs->DynamicJobStatusWin() == BmPrefs::CONN_WIN_DYNAMIC_EMPTY
		&& !WantsToStayVisible())) {
			delete this;
	} else {
		if (!completed)
			ResetController();
	}
}



/********************************************************************************\
	BmPopperView
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new popper-view
\*------------------------------------------------------------------------------*/
BmPopperView* BmPopperView::CreateInstance( const char* name) {
	return new BmPopperView( name);
}

/*------------------------------------------------------------------------------*\
	BmPopperView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmPopperView::BmPopperView( const char* name)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mMailBar( NULL)
{
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( BRect(), name, "State: ", name), true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( BRect(), name, "Messages: ", ""), true, false, false
		),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 8.0);
	mStatBar->SetBarColor( BmJobStatusWin::BM_COL_STATUSBAR);
	mMailBar->SetBarHeight( 8.0);
}

/*------------------------------------------------------------------------------*\
	~BmPopperView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPopperView::~BmPopperView() {
}

/*------------------------------------------------------------------------------*\
	CreateJobModel( data)
		-	creates and returns a new job-model, data may contain constructor args
\*------------------------------------------------------------------------------*/
BmJobModel* BmPopperView::CreateJobModel( BMessage* msg) {
	BArchivable* obj;
	BmPopAccount* account;
	obj = instantiate_object( msg);
	(obj && (account = dynamic_cast<BmPopAccount*>( obj)))
													|| BM_THROW_INVALID( "Could not create BmPopAccount-instance from message");
	return new BmPopper( account->Name(), account);
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmPopperView::ResetController() {
	mStatBar->Reset( "State: ", ControllerName());
	mStatBar->SetText( "idle");
	mMailBar->Reset( "Messages: ", NULL);
}

/*------------------------------------------------------------------------------*\
	WantsToStay´Visible()
		-	
\*------------------------------------------------------------------------------*/
bool BmPopperView::WantsToStayVisible() {
	return mMailBar->CurrentValue() == 0; 
}
				
/*------------------------------------------------------------------------------*\
	UpdateModelView()
		-	
\*------------------------------------------------------------------------------*/
void BmPopperView::UpdateModelView( BMessage* msg) {
	BString name = FindMsgString( msg, BmPopper::MSG_POPPER);
	BString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmPopper::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmPopper::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmPopper::MSG_TRAILING, &trailing);

	BM_LOG3( BM_LogConnWin, BString("Updating interface for ") << name);

	BAutolock lock( BmJobStatusWin::Instance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			mMailBar->Update( delta, leading, trailing);
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
		}
	} else
		throw BM_runtime_error("BmPopperView::UpdateModelView(): could not lock window");
}



/********************************************************************************\
	BmJobStatusWin
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	static members of BmJobStatusWin:
		- the color used for a status-bar's gauge:
		- pointer to the single instance
\*------------------------------------------------------------------------------*/
const rgb_color BmJobStatusWin::BM_COL_STATUSBAR = {160,160,160};
BmJobStatusWin* BmJobStatusWin::Instance = NULL;

/*------------------------------------------------------------------------------*\
	BmJobStatusWin()
		-	constructor, creates outer view that will take up the connection-interfaces
\*------------------------------------------------------------------------------*/
BmJobStatusWin::BmJobStatusWin( const char* title, BLooper* invoker)
	:	MWindow( BRect(50,50,0,0), "JobStatuss",
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					MyWinFlags)
	,	mInvokingLooper( invoker)
	,	mActiveConnCount( 0)
{ 
	mOuterGroup = 
		new VGroup(
			new Space( minimax( 300,0,1E5,1E5,1)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));

	Instance = this;
	BM_LOG2( BM_LogConnWin, "JobStatusWin has started");
}

/*------------------------------------------------------------------------------*\
	~BmJobStatusWin()
		-	destructor,	tells interested party that we are finished
		-	FIXME: needs to free memory if necessary!
\*------------------------------------------------------------------------------*/
BmJobStatusWin::~BmJobStatusWin() {
	Instance = NULL;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmJobStatusWin::QuitRequested() {
	BM_LOG2( BM_LogConnWin, BString("JobStatusWin has been asked to quit; stopping all connections"));
	JobStatusMap::iterator iter;
	for( iter = mActiveJobStatuss.begin(); iter != mActiveJobStatuss.end(); ++iter) {
		((*iter).second)->JobIsDone( false);
	}
	BM_LOG2( BM_LogConnWin, BString("JobStatusWin has stopped all connections"));
	if (mInvokingLooper)
		mInvokingLooper->PostMessage( BM_APP_CONNWIN_DONE);
	Hide();
	return bmApp->IsQuitting();
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::Quit() {
	BM_LOG2( BM_LogConnWin, BString("JobStatusWin has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles messages sent from Application (connection requests)
			and messages sent from connections (update-requests and 
			finished-triggers)
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_CONNWIN_FETCHPOP: {
				// request to start a new POP3-connection
				AddJobStatus( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("JobStatusWindow: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddJobStatus( msg)
		-	adds a new connection-interface to this window
		-	the necessary BmJobStatusView-object will be created and its 
			corresponding BmJobModel will be started (in a new thread).
		-	if connection is already active, it is left alone (nothing happens)
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::AddJobStatus( BMessage* msg) {
	BM_assert( msg);

	BString name = FindMsgString( msg, MSG_CONN_NAME);
	BmJobStatusView* controller = NULL;

	BM_LOG( BM_LogConnWin, BString("Adding connection ") << name);

	JobStatusMap::iterator interfaceIter = mActiveJobStatuss.find( name);
	if (interfaceIter != mActiveJobStatuss.end()) {
		// view found, maybe this popper is already active:
		controller = dynamic_cast<BmPopperView*>((*interfaceIter).second);
		if (controller->IsJobRunning()) {
			// job is still running, so we better don't disturb:
			BM_LOG( BM_LogConnWin, BString("JobStatus ") << name << " still active, add aborted.");
			return;
		}
	}

	if (!controller)
	{	// account is inactive, so we create a new controller for it:
		BM_LOG2( BM_LogConnWin, BString("Creating new view for ") << name);
		switch( msg->what) {
			case BM_CONNWIN_FETCHPOP:
				controller = new BmPopperView( name.String());
				break;
			default:
				break;
		}

		BAutolock lock( this);
		lock.IsLocked()						|| BM_THROW_RUNTIME("AddJobStatus(): could not lock window");

		// add the new interface to our view:
		mOuterGroup->AddChild( dynamic_cast<BView*>(controller));
		RecalcSize();
		// ...and note the interface inside the map:
		mActiveJobStatuss[name] = controller;
	} else {
		// we are in STATIC-mode, where inactive connections are shown. We just
		// have to reactivate the controller.
		// This is done by the controllers Reset()-method:
		BM_LOG2( BM_LogConnWin, BString("Reactivating view of ") << name);
		controller->ResetController();
	}

	// finally, we create the new job...
	BmJobModel* job = controller->CreateJobModel( msg);

	// ...and activate the Job via its controller:
	BM_LOG2( BM_LogConnWin, BString("Starting connection thread "));
	controller->StartJob( job);

	mActiveConnCount++;
	if (IsHidden())
		Show();
}

/*------------------------------------------------------------------------------*\
	RemoveJobStatus( name)
		-	removes a connection and its administrative info
		-	the decision about whether or not to remove the interface, too, 
			depends on the mode of the connection-window:
			*	CONN_WIN_DYNAMIC:			always remove interface
			*	CONN_WIN_DYNAMIC_EMPTY:	remove interface if no new mail was found
			*	CONN_WIN_STATIC:			never remove interface
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::RemoveJobStatus( const char* name) {
	BM_assert( name);

	BM_LOG( BM_LogConnWin, BString("Removing connection ") << name);

	JobStatusMap::iterator interfaceIter = mActiveJobStatuss.find( name);
	if (interfaceIter == mActiveJobStatuss.end())
		return;									// account is not active, nothing to do...
	
	BmJobStatusView* controller = (*interfaceIter).second;
	if (!controller)
		return;

	controller->StopJob();

	// remove interface only if mode indicates to do so:
	if (ThePrefs->DynamicJobStatusWin() == BmPrefs::CONN_WIN_DYNAMIC 
	|| (ThePrefs->DynamicJobStatusWin() == BmPrefs::CONN_WIN_DYNAMIC_EMPTY
		&& !controller->WantsToStayVisible())) {
		BM_LOG2( BM_LogConnWin, BString("Removing interface of connection ") << name);
		BAutolock lock( this);
		lock.IsLocked()						|| BM_THROW_RUNTIME( "RemoveJobStatus(): could not lock window");
		BRect rect = controller->Bounds();
		mOuterGroup->RemoveChild( controller);
		ResizeBy( 0, 0-(rect.Height()));
		RecalcSize();
		mActiveJobStatuss.erase( name);
	}
	mActiveConnCount--;
	if (mActiveJobStatuss.empty() && ThePrefs->DynamicJobStatusWin()) {
		Hide();
	}
}
