/*
	BmJobStatusWin.cpp
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


#include <stdio.h>

#include <Autolock.h>
#include <Beep.h>
#include <ClassInfo.h>
#include <Directory.h>
#include <Entry.h>
#include <InterfaceDefs.h>
#include <MessageRunner.h>
#include <Path.h>
#include <Roster.h>
#include <StatusBar.h>

#include <layout.h>
#include <HGroup.h>
#include <MBorder.h>
#include <MButton.h>
#include <MBViewWrapper.h>
#include <MStop.h>
#include <MStringView.h>
#include <Space.h>

#include "TextEntryAlert.h"
#include "ListSelectionAlert.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailMover.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmSmtpAccount.h"
#include "BmSmtp.h"


#define BM_MINSIZE 200

static const char* BM_BEEP_EVENT = "New E-mail";

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
	:	inheritedView( M_ETCHED_BORDER, 2, const_cast<char*>(name))
	,	inherited( name)
	,	mShowMsgRunner( NULL)
	,	mRemoveMsgRunner( NULL)
	,	mMSecsBeforeShow( 10)
	,	mMSecsBeforeRemove( 10)
	,	mIsAutoJob( false)
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
				BmAutolock lock( TheJobStatusWin);
				lock.IsLocked()				|| BM_THROW_RUNTIME( "JobStatusView(): could not lock window");
				if (!mIsAutoJob) {
					do {
						TheJobStatusWin->Minimize( false);
							TheJobStatusWin->Show();
					} while (TheJobStatusWin->IsHidden());
				}
				break;
			}
			case BM_TIME_TO_REMOVE: {
				BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been told to remove its view");
				BmAutolock lock( TheJobStatusWin);
				lock.IsLocked()				|| BM_THROW_RUNTIME( "JobStatusView(): could not lock window");
				DetachModel();
				if (!IsHidden()) {
					Hide();
					if (!mIsAutoJob)
						TheJobStatusWin->Hide();
				}
				TheJobStatusWin->RemoveJob( ControllerName());
				delete this;
				break;
			}
			case M_STOP_SELECTED: {
				// user asks us to stop, so we do it:
				StopJob();
				break;
			}
			default:
				inheritedView::MessageReceived( msg);
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
void BmJobStatusView::StartJob( BmJobModel* model, bool startInNewThread, 
										  int32 jobSpecifier) {
	delete mShowMsgRunner;
	mShowMsgRunner = NULL;
	BMessage* timerMsg = new BMessage( BM_TIME_TO_SHOW);
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> sets timer-to-show to "<<MSecsBeforeShow()<<" msecs");
	mShowMsgRunner = new BMessageRunner( BMessenger( this), timerMsg, 
													 MSecsBeforeShow(), 1);
	inherited::StartJob( model, startInNewThread, jobSpecifier);
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
	if (ThePrefs->GetBool("DynamicStatusWin") || AlwaysRemoveWhenDone()) {
		int32 timeToWait = completed 
									? (TheJobStatusWin->IsHidden() ? 1 : MSecsBeforeRemove())
									: (TheJobStatusWin->IsHidden() ? 1 : ThePrefs->GetInt("MSecsBeforeRemoveFailed", 5000*1000));
		delete mRemoveMsgRunner;
		mRemoveMsgRunner = NULL;
		BMessage* timerMsg = new BMessage( BM_TIME_TO_REMOVE);
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> sets timer-to-remove to "<<MSecsBeforeRemove()<<" msecs");
		mRemoveMsgRunner = new BMessageRunner( BMessenger( this), timerMsg, timeToWait, 1);
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
	mMSecsBeforeShow = MAX(10,ThePrefs->GetInt( "MSecsBeforeMailMoverShows"));
	BString labelText = BString("To: ") << name;
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( BRect(), name, "Moving: ", ""), true, false, false
		),
		new HGroup(
			mBottomLabel = new MStringView( labelText.String()),
			new MStop( this),
			0
		),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 8.0);
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
	BString key = FindMsgString( msg, BmJobModel::MSG_MODEL);
	BmRef<BmListModelItem> item = TheMailFolderList->FindItemByKey( key);
	BmMailFolder* folder;
	(folder = dynamic_cast<BmMailFolder*>( item.Get()))
													|| BM_THROW_INVALID( BString("Could not find BmMailFolder ") << key);
	BList* refList = new BList;
	entry_ref eref;
	for( int i=0; msg->FindRef( BmMailMover::MSG_REFS, i, &eref)==B_OK; ++i) {
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

	BmAutolock lock( BmJobStatusWin::theInstance);
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
BmPopperView* BmPopperView::CreateInstance( const char* name, bool isAutoCheck) {
	return new BmPopperView( name, isAutoCheck);
}

/*------------------------------------------------------------------------------*\
	BmPopperView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmPopperView::BmPopperView( const char* name, bool isAutoCheck)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mMailBar( NULL)
	,	mHaveBeeped( false)
{
	mIsAutoJob = isAutoCheck;
	mMSecsBeforeRemove = MAX(10,ThePrefs->GetInt( "MSecsBeforePopperRemove"));
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( BRect(), name, name, ""), true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( BRect(), name, "Mails: ", ""), true, false, false
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
	BString accName = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	BmRef<BmListModelItem> item = ThePopAccountList->FindItemByKey( accName);
	BmPopAccount* account;
	(account = dynamic_cast<BmPopAccount*>( item.Get()))
													|| BM_THROW_INVALID( BString("Could not find BmPopAccount ") << accName);
	BmPopper* popper = new BmPopper( account->Name(), account);
	popper->SetPwdAcquisitorFunc( AskUserForPwd);
	return popper;
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmPopperView::ResetController() {
	mStatBar->Reset( ControllerName(), "");
	mStatBar->SetTrailingText( "idle");
	mMailBar->Reset( "Mails: ", NULL);
	mHaveBeeped = false;
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

	BmAutolock lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			mMailBar->Update( delta, leading, trailing);
			if (mMailBar->CurrentValue()==mMailBar->MaxValue()
			&& !mHaveBeeped
			&& ThePrefs->GetBool( "BeepWhenNewMailArrived", true)) {
				// we indicate the arrival of new mail by a beep:
				system_beep( BM_BEEP_EVENT);
				mHaveBeeped = true;
			}
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
		}
	} else
		throw BM_runtime_error("BmPopperView::UpdateModelView(): could not lock window");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPopperView::AskUserForPwd( const BString accName, BString& pwd) {
	// ask user about password:
	BString text = BString( "Please enter password for POP-Account <")
					   << accName << ">:";
	TextEntryAlert* alert = new TextEntryAlert( "Info needed", text.String(),
									 						  "", "Cancel", "OK");
	alert->SetFeel( B_FLOATING_APP_WINDOW_FEEL);
	alert->SetLook( B_FLOATING_WINDOW_LOOK);
	alert->TextEntryView()->HideTyping( true);
	alert->SetShortcut( 0, B_ESCAPE);
	char buf[128];
	int32 result = alert->Go( buf, 128);
	if (result == 1) {
		pwd = buf;
		memset( buf, '*', 128);
		return true;
	} else
		return false;
}



/********************************************************************************\
	BmSmtpView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new smtp-view
\*------------------------------------------------------------------------------*/
BmSmtpView* BmSmtpView::CreateInstance( const char* name) {
	return new BmSmtpView( name);
}

/*------------------------------------------------------------------------------*\
	BmSmtpView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmSmtpView::BmSmtpView( const char* name)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mMailBar( NULL)
{
	mMSecsBeforeRemove = MAX(10,ThePrefs->GetInt( "MSecsBeforeSmtpRemove"));
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( BRect(), name, name, ""), true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( BRect(), name, "Mails: ", ""), true, false, false
		),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 8.0);
	mStatBar->SetBarColor( BmJobStatusWin::BM_COL_STATUSBAR);
	mMailBar->SetBarHeight( 8.0);
}

/*------------------------------------------------------------------------------*\
	~BmSmtpView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmSmtpView::~BmSmtpView() {
}

/*------------------------------------------------------------------------------*\
	CreateJobModel( data)
		-	creates and returns a new job-model, data may contain constructor args
\*------------------------------------------------------------------------------*/
BmJobModel* BmSmtpView::CreateJobModel( BMessage* msg) {
	BString accName = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	BmRef<BmListModelItem> item = TheSmtpAccountList->FindItemByKey( accName);
	BmSmtpAccount* account;
	(account = dynamic_cast<BmSmtpAccount*>( item.Get()))
													|| BM_THROW_INVALID( BString("Could not find BmSmtpAccount ") << accName);
	BmSmtp* smtp = new BmSmtp( account->Name(), account);
	smtp->SetPwdAcquisitorFunc( AskUserForPwd);
	smtp->SetPopAccAcquisitorFunc( AskUserForPopAcc);
	return smtp;
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmSmtpView::ResetController() {
	mStatBar->Reset( ControllerName(), "");
	mStatBar->SetTrailingText( "idle");
	mMailBar->Reset( "Mails: ", NULL);
}

/*------------------------------------------------------------------------------*\
	UpdateModelView()
		-	
\*------------------------------------------------------------------------------*/
void BmSmtpView::UpdateModelView( BMessage* msg) {
	BString name = FindMsgString( msg, BmSmtp::MSG_SMTP);
	BString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmSmtp::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmSmtp::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmSmtp::MSG_TRAILING, &trailing);

	BM_LOG3( BM_LogJobWin, BString("Updating interface for ") << name);

	BmAutolock lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			mMailBar->Update( delta, leading, trailing);
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
		}
	} else
		throw BM_runtime_error("BmSmtpView::UpdateModelView(): could not lock window");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmSmtpView::AskUserForPwd( const BString accName, BString& pwd) {
	// ask user about password:
   BString text = BString( "Please enter password for SMTP-account <")
   				   << accName << ">:";
	TextEntryAlert* alert = new TextEntryAlert( "Info needed", text.String(),
									 						  "", "Cancel", "OK");
	alert->SetFeel( B_FLOATING_APP_WINDOW_FEEL);
	alert->SetLook( B_FLOATING_WINDOW_LOOK);
	alert->TextEntryView()->HideTyping( true);
	alert->SetShortcut( 0, B_ESCAPE);
	char buf[128];
	int32 result = alert->Go( buf, 128);
	if (result == 1) {
		pwd = buf;
		memset( buf, '*', 128);
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmSmtpView::AskUserForPopAcc( const BString accName, BString& popAccName) {
	// ask user about password:
   BString text = BString( "Please select the POP3-account\nto be used in authentication\nfor SMTP-account <")
   				   << accName << ">:";
	BList list;
	BmModelItemMap::const_iterator iter;
	for (iter = ThePopAccountList->begin(); iter != ThePopAccountList->end(); ++iter) {
		BmPopAccount* acc = dynamic_cast<BmPopAccount*>( iter->second.Get());
		list.AddItem( (void*)acc->Name().String());
	}
	ListSelectionAlert* alert 
		= new ListSelectionAlert( "Pop-Account", text.String(), list, "", "Cancel", "OK");
	alert->SetFeel( B_FLOATING_APP_WINDOW_FEEL);
	alert->SetLook( B_FLOATING_WINDOW_LOOK);
	alert->SetShortcut( 0, B_ESCAPE);
	char buf[128];
	int32 result = alert->Go( buf, 128);
	if (result == 1) {
		popAccName = buf;
		return popAccName.Length() > 0;
	} else
		return false;
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
BmJobStatusWin* BmJobStatusWin::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmJobStatusWin* BmJobStatusWin::CreateInstance() {
	if (!theInstance) {
		theInstance = new BmJobStatusWin;
		theInstance->ReadStateInfo();
	}
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	BmJobStatusWin()
		-	constructor, creates outer view that will take up the job-interfaces
\*------------------------------------------------------------------------------*/
BmJobStatusWin::BmJobStatusWin()
	:	BmWindow( "JobStatusWindow", 
					BRect(bmApp->ScreenFrame().right-BM_MINSIZE-4,20,0,0), 
					"Jobs",
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS	|	B_NOT_ZOOMABLE	|	B_NOT_V_RESIZABLE
					| B_NO_WORKSPACE_ACTIVATION)
{ 
	mOuterGroup = 
		new VGroup(
			new Space( minimax( BM_MINSIZE,0,1E5,1E5,1)),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));

	theInstance = this;
	BM_LOG2( BM_LogJobWin, "JobStatusWin has started");
}

/*------------------------------------------------------------------------------*\
	~BmJobStatusWin()
		-	destructor,	tells interested party that we are finished
		-	FIXME: needs to free memory if necessary!
\*------------------------------------------------------------------------------*/
BmJobStatusWin::~BmJobStatusWin() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmJobStatusWin::QuitRequested() {
	BmAutolock lock( this);
	lock.IsLocked()						|| BM_THROW_RUNTIME( "QuitRequested(): could not lock window");
	while( !IsHidden())
		Hide();
	BM_LOG2( BM_LogJobWin, BString("JobStatusWin has been asked to quit; stopping all jobs"));
	JobMap::iterator iter;
	for( iter = mActiveJobs.begin(); iter != mActiveJobs.end(); ++iter) {
		BmJobStatusView* jobView = iter->second;
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
	inherited::Quit();
	BM_LOG2( BM_LogJobWin, BString("JobStatusWin has quit"));
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
			case B_QUIT_REQUESTED:
				break;
			case BM_JOBWIN_SMTP:
			case BM_JOBWIN_POP:
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

	BString name = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	int32 jobSpecifier;
	if (msg->FindInt32( BmJobModel::MSG_JOB_SPEC, &jobSpecifier) != B_OK)
		jobSpecifier = BmJobModel::BM_DEFAULT_JOB;
	bool inNewThread;
	if (msg->FindBool( BmJobModel::MSG_JOB_THREAD, &inNewThread) != B_OK)
		inNewThread = true;
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
			case BM_JOBWIN_POP: {
				bool autoMode = msg->FindBool( BmPopAccountList::MSG_AUTOCHECK);
				controller = new BmPopperView( name.String(), autoMode);
				break;
			}
			case BM_JOBWIN_SMTP:
				controller = new BmSmtpView( name.String());
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
		BRect frame = controller->Frame();
		float height = frame.bottom+2;
		ResizeTo( Frame().Width(), height);
		RecalcSize();
		// ...and add the interface to the map:
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
	controller->StartJob( job, inNewThread, jobSpecifier);
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
	ResizeBy( 0, -1-(rect.Height()));
	RecalcSize();
	mActiveJobs.erase( controller->ControllerName());
	if (mActiveJobs.empty()) {
		if (dynamic_cast<BmPopperView*>( controller)) {
			// [suggested by Rainer Riedl]:
			// 	temporary hack: Execute possibly existing 'filter'-program to do 
			//		mail filtering (this will be removed in Beam 0.92):
			app_info appInfo;
			bmApp->GetAppInfo( &appInfo); 
			node_ref nref;
			nref.device = appInfo.ref.device;
			nref.node = appInfo.ref.directory;
			BDirectory appDir( &nref);
			BEntry appDirEntry;
			appDir.GetEntry( &appDirEntry);
			BPath appPath;
			appDirEntry.GetPath( &appPath);
			BString filterCmd;
			filterCmd << appPath.Path() << "/filter";
			system( filterCmd.String());
		}
		while( !IsHidden())
			Hide();
	}
}
