/*
	BmJobStatusWin.cpp
		$Id$
*/

#include <stdio.h>

#include <Autolock.h>
#include <ClassInfo.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <MessageRunner.h>
#include <StatusBar.h>

#include <layout.h>
#include <MBorder.h>
#include <MButton.h>
#include <MBViewWrapper.h>
#include <MStringView.h>
#include <Space.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailMover.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"



/********************************************************************************\
	BmJobStatusView
\********************************************************************************/

static const int32 BM_TIME_TO_SHOW = 'BmZ1';
static const int32 BM_TIME_TO_REMOVE = 'BmZ2';

/*------------------------------------------------------------------------------*\
	BmJobStatusView()
		-	constructor
\*------------------------------------------------------------------------------*/
BmJobStatusView::BmJobStatusView( const char* name)
	:	MBorder( M_ETCHED_BORDER, 2, const_cast<char*>(name))
	,	BmJobController( name)
	,	mShowMsgRunner( NULL)
	,	mRemoveMsgRunner( NULL)
	,	mMSecsBeforeShow( 100)
	,	mMSecsBeforeRemove( 100)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobStatusView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmJobStatusView::~BmJobStatusView() {
	delete mRemoveMsgRunner;
	delete mShowMsgRunner;
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
			case BM_TIME_TO_SHOW: {
				BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been told to show its view");
				Show();
				TheJobStatusWin->Show();
				break;
			}
			case BM_TIME_TO_REMOVE: {
				BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been told to remove its view");
				DetachModel();
				if (!IsHidden()) {
					Hide();
					TheJobStatusWin->Hide();
				}
				TheJobStatusWin->RemoveJob( ControllerName());
				delete this;
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
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobStatusView::StartJob( BmJobModel* model, bool startInNewThread) {
	delete mShowMsgRunner;
	mShowMsgRunner = NULL;
	BMessage* timerMsg = new BMessage( BM_TIME_TO_SHOW);
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> sets timer-to-show to "<<MSecsBeforeShow()<<" msecs");
	mShowMsgRunner = new BMessageRunner( BMessenger( this), timerMsg, 
													 MSecsBeforeShow(), 1);
	inherited::StartJob( model, startInNewThread);
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	hook-method that is called when a JobModel tells us it's finished
		-	parameter msg may contain any further attributes
		-	tells the job-window to remove this job
		-	deletes this job
\*------------------------------------------------------------------------------*/
void BmJobStatusView::JobIsDone( bool completed) {
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been told that job " << ModelName() << " is done");
	if (!completed)
		ResetController();
	if (ThePrefs->GetBool("DynamicStatusWin") || AlwaysRemoveWhenDone()) {
		delete mRemoveMsgRunner;
		mRemoveMsgRunner = NULL;
		BMessage* timerMsg = new BMessage( BM_TIME_TO_REMOVE);
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> sets timer-to-remove to "<<MSecsBeforeRemove()<<" msecs");
		mRemoveMsgRunner = new BMessageRunner( BMessenger( this), timerMsg, 
															MSecsBeforeRemove(), 1);
	}
}



/********************************************************************************\
	BmMailMoverView
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new mailmover-view
\*------------------------------------------------------------------------------*/
BmMailMoverView* BmMailMoverView::CreateInstance( const char* name) {
	return new BmMailMoverView( name);
}

/*------------------------------------------------------------------------------*\
	BmMailMoverView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmMailMoverView::BmMailMoverView( const char* name)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mBottomLabel( NULL)
{
	mMSecsBeforeShow = ThePrefs->GetInt( "MSecsBeforeMailMoverShows", 250*1000);
	BString labelText = BString("To: ") << ControllerName();
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( BRect(), name, "Moving: ", ""), true, false, false
		),
		mBottomLabel = new MStringView( labelText.String()),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 12.0);
}

/*------------------------------------------------------------------------------*\
	~BmMailMoverView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmMailMoverView::~BmMailMoverView() {
}

/*------------------------------------------------------------------------------*\
	CreateJobModel( data)
		-	creates and returns a new job-model, data may contain constructor args
\*------------------------------------------------------------------------------*/
BmJobModel* BmMailMoverView::CreateJobModel( BMessage* msg) {
	BList* refList = new BList;
	BmMailFolder* folder;
	msg->FindPointer( MSG_FOLDER, (void**)&folder);
	entry_ref eref;
	for( int i=0; msg->FindRef( MSG_REFS, i, &eref)==B_OK; ++i) {
		refList->AddItem( new entry_ref( eref));
	}
	return new BmMailMover( ControllerName(), refList, folder);
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmMailMoverView::ResetController() {
	mStatBar->Reset( "Moving: ", "");
}

/*------------------------------------------------------------------------------*\
	UpdateModelView()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMoverView::UpdateModelView( BMessage* msg) {
	BString name = FindMsgString( msg, BmMailMover::MSG_MOVER);
	BString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmMailMover::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmMailMover::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmMailMover::MSG_TRAILING, &trailing);

	BM_LOG3( BM_LogJobWin, BString("Updating interface for ") << name);

	BmAutolock lock( BmJobStatusWin::Instance);
	if (lock.IsLocked()) {
		mStatBar->Update( delta, leading, trailing);
	} else
		throw BM_runtime_error("BmMailMoverView::UpdateModelView(): could not lock window");
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
	mMSecsBeforeRemove = ThePrefs->GetInt( "MSecsBeforePopperRemove", 5000*1000);
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
	BString accName = FindMsgString( msg, BmJobStatusWin::MSG_JOB_NAME);
	BmListModelItemRef item = ThePopAccountList->FindItemByKey( accName);
	BmPopAccount* account;
	(account = dynamic_cast<BmPopAccount*>( item.Get()))
													|| BM_THROW_INVALID( BString("Could not find BmPopAccount ") << accName);
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

	BM_LOG3( BM_LogJobWin, BString("Updating interface for ") << name);

	BmAutolock lock( BmJobStatusWin::Instance);
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

BmJobStatusWin* BmJobStatusWin::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmJobStatusWin* BmJobStatusWin::CreateInstance() {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmJobStatusWin( "JobStatusWin");
}

/*------------------------------------------------------------------------------*\
	BmJobStatusWin()
		-	constructor, creates outer view that will take up the job-interfaces
\*------------------------------------------------------------------------------*/
BmJobStatusWin::BmJobStatusWin( const char* title)
	:	MWindow( BRect(50,50,0,0), "Jobs",
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					MyWinFlags)
{ 
	mOuterGroup = 
		new VGroup(
			new Space( minimax( 300,0,1E5,1E5,1)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));

	Instance = this;
	BM_LOG2( BM_LogJobWin, "JobStatusWin has started");
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
	if (!bmApp->IsQuitting()) {
		return false;
	}
	BmAutolock lock( this);
	lock.IsLocked()						|| BM_THROW_RUNTIME( "QuitRequested(): could not lock window");
	BM_LOG2( BM_LogJobWin, BString("JobStatusWin has been asked to quit; stopping all jobs"));
	JobMap::iterator iter;
	for( iter = mActiveJobs.begin(); iter != mActiveJobs.end(); ++iter) {
		BmJobStatusView* jobView = (*iter).second;
		if (jobView)
			jobView->StopJob();
	}
	BM_LOG2( BM_LogJobWin, BString("JobStatusWin has stopped all jobs"));
	return bmApp->IsQuitting();
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::Quit() {
	BM_LOG2( BM_LogJobWin, BString("JobStatusWin has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles messages sent from Application (job requests)
			and messages sent from jobs (update-requests and 
			finished-triggers)
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOBWIN_FETCHPOP:
			case BM_JOBWIN_MOVEMAILS: {
				// request to start a new job
				AddJob( msg);
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
	AddJob( msg)
		-	adds a new job-interface to this window
		-	the necessary BmJobStatusView-object will be created and its 
			corresponding BmJobModel will be started (in a new thread).
		-	if job is already active, it is left alone (nothing happens)
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::AddJob( BMessage* msg) {
	BM_assert( msg);

	BString name = FindMsgString( msg, MSG_JOB_NAME);
	BmJobStatusView* controller = NULL;

	BM_LOG( BM_LogJobWin, BString("Adding job ") << name);

	BmAutolock lock( this);
	lock.IsLocked()						|| BM_THROW_RUNTIME( "AddJob(): could not lock window");

	JobMap::iterator interfaceIter = mActiveJobs.find( name);
	if (interfaceIter != mActiveJobs.end()) {
		// view found, maybe this popper is already active:
		controller = (*interfaceIter).second;
		if (controller->IsJobRunning()) {
			// job is still running, so we better don't disturb:
			BM_LOG( BM_LogJobWin, BString("JobStatus ") << name << " still active, add aborted.");
			return;
		}
	}

	if (!controller)
	{	// job is inactive, so we create a new controller for it:
		BM_LOG2( BM_LogJobWin, BString("Creating new view for ") << name);
		switch( msg->what) {
			case BM_JOBWIN_FETCHPOP:
				controller = new BmPopperView( name.String());
				break;
			case BM_JOBWIN_MOVEMAILS:
				controller = new BmMailMoverView( name.String());
				break;
			default:
				break;
		}

		BmAutolock lock( this);
		lock.IsLocked()						|| BM_THROW_RUNTIME("AddJob(): could not lock window");

		// add the new interface to our view:
		mOuterGroup->AddChild( dynamic_cast<BView*>(controller));
		RecalcSize();
		// ...and note the interface inside the map:
		mActiveJobs[name] = controller;
	} else {
		// we are in STATIC-mode, where inactive jobs are shown. We just
		// have to reactivate the controller.
		// This is done by the controllers Reset()-method:
		BM_LOG2( BM_LogJobWin, BString("Reactivating view of ") << name);
		controller->ResetController();
	}

	// finally, we create the new job...
	BmJobModel* job = controller->CreateJobModel( msg);

	// ...and activate the Job via its controller:
	BM_LOG2( BM_LogJobWin, BString("Starting job thread "));
	controller->StartJob( job);
}

/*------------------------------------------------------------------------------*\
	RemoveJob( name)
		-	removes a job and its administrative info
		-	the decision about whether or not to remove the interface, too, 
			depends on the mode of the job-window:
			*	DYNAMIC:			always remove interface
			*	STATIC:			never remove interface
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::RemoveJob( const char* name) {
	BM_assert( name);

	BM_LOG( BM_LogJobWin, BString("Removing job ") << name);

	BmAutolock lock( this);
	lock.IsLocked()						|| BM_THROW_RUNTIME( "RemoveJob(): could not lock window");

	JobMap::iterator interfaceIter = mActiveJobs.find( name);
	if (interfaceIter == mActiveJobs.end())
		return;									// account is not active, nothing to do...
	
	BmJobStatusView* controller = (*interfaceIter).second;
	if (!controller)
		return;

	BRect rect = controller->Bounds();
	mOuterGroup->RemoveChild( controller);
	ResizeBy( 0, 0-(rect.Height()));
	RecalcSize();
	mActiveJobs.erase( controller->ControllerName());
}