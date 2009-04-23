/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
#include <MBViewWrapper.h>
#include <MStop.h>
#include <MStringView.h>
#include <Space.h>

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmFilter.h"
#include "BmImap.h"
#include "BmImapAccount.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFilter.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailMover.h"
#include "BmPopAccount.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmSmtpAccount.h"
#include "BmSmtp.h"
#include "Colors.h"

/*
 * TODO: refactor this bloody mess into something useful...
 *       currently, it's too hard to tell what's going on when (too many
 *			different concepts in one class).
 */



static const int BM_MINSIZE = 200;

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
void BmJobStatusView::UpdateModelView( BMessage*) {
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
				BM_LOG2( BM_LogModelController, 
							BmString("Controller <") << ControllerName() 
								<< "> has been told to show its view");
				BmAutolockCheckGlobal lock( TheJobStatusWin);
				if (!lock.IsLocked())
					BM_THROW_RUNTIME( "JobStatusView(): could not lock window");
				if (!mIsAutoJob) {
					TheJobStatusWin->Minimize( false);
					while (TheJobStatusWin->IsHidden())
						TheJobStatusWin->Show();
				}
				break;
			}
			case BM_TIME_TO_REMOVE: {
				BM_LOG2( BM_LogModelController, 
							BmString("Controller <") << ControllerName() 
								<< "> has been told to remove its view");
				BmAutolockCheckGlobal lock( TheJobStatusWin);
				if (!lock.IsLocked())
					BM_THROW_RUNTIME( "JobStatusView(): could not lock window");
				DetachModel();
				if (!IsHidden())
					Hide();
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
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("JobStatusView: ") << err.what());
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
	BMessage timerMsg( BM_TIME_TO_SHOW);
	BM_LOG2( BM_LogModelController, 
				BmString("Controller <") << ControllerName() 
					<< "> sets timer-to-show to "<<MSecsBeforeShow()<<" msecs");
	mShowMsgRunner = new BMessageRunner( BMessenger( this), &timerMsg, 
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
	BM_LOG2( BM_LogModelController, 
				BmString("Controller <") << ControllerName() 
					<< "> has been told that job " << ModelName() << " is done");
	int32 timeToWait 
		= completed 
			? (TheJobStatusWin->IsHidden() 
				? 1 
				: MSecsBeforeRemove())
			: (TheJobStatusWin->IsHidden() 
				? 1 
				: ThePrefs->GetInt("MSecsBeforeRemoveFailed", 5000*1000));
	delete mRemoveMsgRunner;
	mRemoveMsgRunner = NULL;
	BMessage timerMsg( BM_TIME_TO_REMOVE);
	BM_LOG2( BM_LogModelController, 
				BmString("Controller <") << ControllerName() 
					<< "> sets timer-to-remove to " << MSecsBeforeRemove() 
					<< " msecs");
	mRemoveMsgRunner = new BMessageRunner( 
		BMessenger( this), &timerMsg, timeToWait, 1
	);
	BMessage jobDoneMsg(BM_JOB_DONE);
	jobDoneMsg.AddString(BmJobModel::MSG_JOB_NAME, ControllerName());
	Looper()->PostMessage(&jobDoneMsg);
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
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( 
				BRect(), name, "Moving: ", ""
			), 
			true, false, false
		),
		new HGroup(
			mBottomLabel = new MStringView( ""),
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
	BmString key = FindMsgString( msg, BmJobModel::MSG_MODEL);
	BmRef<BmListModelItem> item = TheMailFolderList->FindItemByKey( key);
	BmMailFolder* folder;
	if (!(folder = dynamic_cast<BmMailFolder*>( item.Get())))
		BM_THROW_INVALID( BmString("Could not find BmMailFolder ") << key);
	entry_ref* refs = NULL;
	mBottomLabel->SetText( (BmString("To: ") << folder->DisplayKey()).String());
	msg->FindPointer( BmMailMover::MSG_REFS, (void**)&refs);
	int32 refCount = msg->FindInt32( BmMailMover::MSG_REF_COUNT);
	return new BmMailMover( ControllerName(), refs, refCount, folder);
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
	BmString name = FindMsgString( msg, BmMailMover::MSG_MOVER);

	float delta = FindMsgFloat( msg, BmMailMover::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmMailMover::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmMailMover::MSG_TRAILING, &trailing);

	BM_LOG3( BM_LogJobWin, BmString("Updating interface for ") << name);

	BmAutolockCheckGlobal lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		mStatBar->Update( delta, leading, trailing);
	} else
		throw BM_runtime_error( "BmMailMoverView::UpdateModelView(): could not "
										"lock window");
}



/********************************************************************************\
	BmMailFilterView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new mailfilter-view
\*------------------------------------------------------------------------------*/
BmMailFilterView* BmMailFilterView::CreateInstance( const char* name) {
	return new BmMailFilterView( name);
}

/*------------------------------------------------------------------------------*\
	BmMailFilterView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmMailFilterView::BmMailFilterView( const char* name)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mBottomLabel( NULL)
{
	mMSecsBeforeShow 
		= MAX(10,ThePrefs->GetInt( "MSecsBeforeMailFilterShows", 500));
	MView* view = new HGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( 
				BRect(), name, "Filtering: ", ""
			), 
			true, false, false
		),
		new MStop( this),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 8.0);
}

/*------------------------------------------------------------------------------*\
	~BmMailFilterView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmMailFilterView::~BmMailFilterView() {
}

/*------------------------------------------------------------------------------*\
	CreateJobModel( data)
		-	creates and returns a new job-model, data may contain constructor args
\*------------------------------------------------------------------------------*/
BmJobModel* BmMailFilterView::CreateJobModel( BMessage* msg) {
	BmString filterName = msg->FindString( BmListModel::MSG_ITEMKEY);
	BmFilter* filter = NULL;
	if (filterName.Length()) {
		if (filterName == BmFilterList::LEARN_AS_SPAM_NAME)
			filter = TheFilterList->LearnAsSpamFilter().Get();
		else if (filterName == BmFilterList::LEARN_AS_TOFU_NAME)
			filter = TheFilterList->LearnAsTofuFilter().Get();
		else {
			BmRef<BmListModelItem> filterRef
				= TheFilterList->FindItemByKey( filterName);
			filter = dynamic_cast< BmFilter*>( filterRef.Get());
		}
		if (!filter)
			return NULL;
	}
	BmMailFilter* mailFilter = new BmMailFilter( ControllerName(), filter);
	BmMailRefVect* refVect = NULL;
	msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, (void**)&refVect);
	mailFilter->SetMailRefVect( refVect);
	return mailFilter;
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmMailFilterView::ResetController() {
	mStatBar->Reset( "Filtering: ", "");
}

/*------------------------------------------------------------------------------*\
	UpdateModelView()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFilterView::UpdateModelView( BMessage* msg) {
	BmString name = FindMsgString( msg, BmMailFilter::MSG_FILTER);
	BmString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmMailMover::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmMailMover::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmMailMover::MSG_TRAILING, &trailing);

	BM_LOG3( BM_LogJobWin, BmString("Updating interface for ") << name);

	BmAutolockCheckGlobal lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		mStatBar->Update( delta, leading, trailing);
	} else
		throw BM_runtime_error( "BmMailFilterView::UpdateModelView(): could not "
										"lock window");
}



/********************************************************************************\
	BmPopperView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new popper-view
\*------------------------------------------------------------------------------*/
BmPopperView* BmPopperView::CreateInstance( const char* name, 
														  bool isAutoCheck) {
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
			mStatBar = new BStatusBar( 
				BRect(), name, name, ""
			), 
			true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( 
				BRect(), name, "Mails: ", ""
			), 
			true, false, false
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
	BmString accName = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	BmRef<BmListModelItem> item = TheRecvAccountList->FindItemByKey( accName);
	BmPopAccount* account;
	if (!(account = dynamic_cast<BmPopAccount*>( item.Get())))
		BM_THROW_INVALID( BmString("Could not find BmPopAccount ") << accName);
	BmPopper* popper = new BmPopper( account->Name(), account);
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
	BmString name = FindMsgString( msg, BmPopper::MSG_MODEL);
	BmString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmPopper::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmPopper::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmPopper::MSG_TRAILING, &trailing);
	bool encrypted = false;
	msg->FindBool( BmPopper::MSG_ENCRYPTED, &encrypted);
	bool failed = false;
	msg->FindBool( BmPopper::MSG_FAILED, &failed);

	BM_LOG3( BM_LogJobWin, BmString("Updating interface for ") << name);

	BmAutolockCheckGlobal lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			if (delta < 0)
				mMailBar->Reset();
			else
				mMailBar->Update( delta, leading, trailing);
			if (mMailBar->CurrentValue()==mMailBar->MaxValue()
			&& !mHaveBeeped
			&& ThePrefs->GetBool( "BeepWhenNewMailArrived", true)) {
				// we indicate the arrival of new mail by a beep:
				system_beep( BM_BEEP_EVENT);
				mHaveBeeped = true;
			}
		} else if (domain == "mailbar.cleanup") {
			mMailBar->Update( delta, leading, trailing);
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
			rgb_color newBarColor = mStatBar->BarColor();
			if (encrypted)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_GOOD;
			if (failed)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_BAD;
			if (newBarColor != mStatBar->BarColor()) {
				mStatBar->SetBarColor(newBarColor);
				mStatBar->Invalidate();
			}
		}
	} else
		throw BM_runtime_error( "BmPopperView::UpdateModelView(): could not "
										"lock window");
}



/********************************************************************************\
	BmImapView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance( name)
		-	creates and returns a new imap-view
\*------------------------------------------------------------------------------*/
BmImapView* BmImapView::CreateInstance( const char* name, 
													 bool isAutoCheck) {
	return new BmImapView( name, isAutoCheck);
}

/*------------------------------------------------------------------------------*\
	BmImapView()
		-	standard constructor
\*------------------------------------------------------------------------------*/
BmImapView::BmImapView( const char* name, bool isAutoCheck)
	:	BmJobStatusView( name)
	,	mStatBar( NULL)
	,	mMailBar( NULL)
	,	mHaveBeeped( false)
{
	mIsAutoJob = isAutoCheck;
	mMSecsBeforeRemove = MAX(10,ThePrefs->GetInt( "MSecsBeforePopperRemove"));
	MView* view = new VGroup(
		new MBViewWrapper(
			mStatBar = new BStatusBar( 
				BRect(), name, name, ""
			), 
			true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( 
				BRect(), name, "Mails: ", ""
			), 
			true, false, false
		),
		0
	);
	AddChild( dynamic_cast<BView*>(view));
	mStatBar->SetBarHeight( 8.0);
	mStatBar->SetBarColor( BmJobStatusWin::BM_COL_STATUSBAR);
	mMailBar->SetBarHeight( 8.0);
}

/*------------------------------------------------------------------------------*\
	~BmImapView()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmImapView::~BmImapView() {
}

/*------------------------------------------------------------------------------*\
	CreateJobModel( data)
		-	creates and returns a new job-model, data may contain constructor args
\*------------------------------------------------------------------------------*/
BmJobModel* BmImapView::CreateJobModel( BMessage* msg) {
	BmString accName = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	BmRef<BmListModelItem> item = TheRecvAccountList->FindItemByKey( accName);
	BmImapAccount* account;
	if (!(account = dynamic_cast<BmImapAccount*>( item.Get())))
		BM_THROW_INVALID( BmString("Could not find BmImapAccount ") << accName);
	BmImap* imap = new BmImap( account->Name(), account);
	return imap;
}

/*------------------------------------------------------------------------------*\
	ResetController()
		-	reinitializes the view in order to start another job
\*------------------------------------------------------------------------------*/
void BmImapView::ResetController() {
	mStatBar->Reset( ControllerName(), "");
	mStatBar->SetTrailingText( "idle");
	mMailBar->Reset( "Mails: ", NULL);
	mHaveBeeped = false;
}

/*------------------------------------------------------------------------------*\
	UpdateModelView()
		-	
\*------------------------------------------------------------------------------*/
void BmImapView::UpdateModelView( BMessage* msg) {
	BmString name = FindMsgString( msg, BmImap::MSG_MODEL);
	BmString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmImap::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmImap::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmImap::MSG_TRAILING, &trailing);
	bool encrypted = false;
	msg->FindBool( BmImap::MSG_ENCRYPTED, &encrypted);
	bool failed = false;
	msg->FindBool( BmImap::MSG_FAILED, &failed);

	BM_LOG3( BM_LogJobWin, BmString("Updating interface for ") << name);

	BmAutolockCheckGlobal lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			if (delta < 0)
				mMailBar->Reset();
			else
				mMailBar->Update( delta, leading, trailing);
			if (mMailBar->CurrentValue()==mMailBar->MaxValue()
			&& !mHaveBeeped
			&& ThePrefs->GetBool( "BeepWhenNewMailArrived", true)) {
				// we indicate the arrival of new mail by a beep:
				system_beep( BM_BEEP_EVENT);
				mHaveBeeped = true;
			}
		} else if (domain == "mailbar.cleanup") {
			mMailBar->Update( delta, leading, trailing);
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
			rgb_color newBarColor = mStatBar->BarColor();
			if (encrypted)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_GOOD;
			if (failed)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_BAD;
			if (newBarColor != mStatBar->BarColor()) {
				mStatBar->SetBarColor(newBarColor);
				mStatBar->Invalidate();
			}
		}
	} else
		throw BM_runtime_error( "BmImapView::UpdateModelView(): could not "
										"lock window");
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
			mStatBar = new BStatusBar( 
				BRect(), name, (BmString("SMTP: ")<<name).String(), ""
			), 
			true, false, false
		),
		new MBViewWrapper(
			mMailBar = new BStatusBar( 
				BRect(), name, "Mails: ", ""
			), 
			true, false, false
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
	BmString accName = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	BmRef<BmListModelItem> item = TheSmtpAccountList->FindItemByKey( accName);
	BmSmtpAccount* account;
	if (!(account = dynamic_cast<BmSmtpAccount*>( item.Get())))
		BM_THROW_INVALID( BmString("Could not find BmSmtpAccount ") << accName);
	BmSmtp* smtp = new BmSmtp( account->Name(), account);
	entry_ref eref;
	for( int i=0; msg->FindRef( BmSmtpAccount::MSG_REF, i, &eref)==B_OK; ++i)
		smtp->QueueMail( eref);
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
	BmString name = FindMsgString( msg, BmSmtp::MSG_MODEL);
	BmString domain = FindMsgString( msg, BmJobModel::MSG_DOMAIN);

	float delta = FindMsgFloat( msg, BmSmtp::MSG_DELTA);
	const char* leading = NULL;
	msg->FindString( BmSmtp::MSG_LEADING, &leading);
	const char* trailing = NULL;
	msg->FindString( BmSmtp::MSG_TRAILING, &trailing);
	bool encrypted = false;
	msg->FindBool( BmSmtp::MSG_ENCRYPTED, &encrypted);
	bool failed = false;
	msg->FindBool( BmSmtp::MSG_FAILED, &failed);

	BM_LOG3( BM_LogJobWin, BmString("Updating interface for ") << name);

	BmAutolockCheckGlobal lock( BmJobStatusWin::theInstance);
	if (lock.IsLocked()) {
		if (domain == "mailbar") {
			mMailBar->Update( delta, leading, trailing);
		} else { 
			// domain == "statbar"
			mStatBar->Update( delta, leading, trailing);
			rgb_color newBarColor = mStatBar->BarColor();
			if (encrypted)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_GOOD;
			if (failed)
				newBarColor = BmJobStatusWin::BM_COL_STATUSBAR_BAD;
			if (newBarColor != mStatBar->BarColor()) {
				mStatBar->SetBarColor(newBarColor);
				mStatBar->Invalidate();
			}
		}
	} else
		throw BM_runtime_error( "BmSmtpView::UpdateModelView(): could not "
										"lock window");
}



/********************************************************************************\
	BmJobStatusWin
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	static members of BmJobStatusWin:
		- the color used for a status-bar's gauge:
		- pointer to the single instance
\*------------------------------------------------------------------------------*/
#ifdef B_BEOS_VERSION_DANO
const rgb_color BmJobStatusWin::BM_COL_STATUSBAR = {216,216,216};
#else
const rgb_color BmJobStatusWin::BM_COL_STATUSBAR = {160,160,160};
#endif
const rgb_color BmJobStatusWin::BM_COL_STATUSBAR_GOOD = {0,160,0};
const rgb_color BmJobStatusWin::BM_COL_STATUSBAR_BAD = {208,0,0};
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
					BRect(beamApp->ScreenFrame().right-BM_MINSIZE-4,20,0,0), 
					"Jobs",
					B_FLOATING_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
					B_ASYNCHRONOUS_CONTROLS	|	B_NOT_ZOOMABLE	|	B_NOT_V_RESIZABLE
					| B_NO_WORKSPACE_ACTIVATION 
					| (mouse_mode()==B_NORMAL_MOUSE ? B_AVOID_FOCUS : 0))
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
	BmAutolockCheckGlobal lock( this);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "QuitRequested(): could not lock window");
	while( !IsHidden())
		Hide();
	BM_LOG2( BM_LogJobWin, 
				"JobStatusWin has been asked to quit; stopping all jobs");
	JobMap::iterator iter;
	for( iter = mActiveJobs.begin(); iter != mActiveJobs.end(); ++iter) {
		BmJobStatusView* jobView = iter->second;
		if (jobView)
			jobView->StopJob();
	}
	snooze( ThePrefs->GetInt( "FeedbackTimeout", 200)*1500);
							// give jobs a chance to stop
	BM_LOG2( BM_LogJobWin, BmString("JobStatusWin has stopped all jobs"));
	return beamApp->IsQuitting();
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::Quit() {
	inherited::Quit();
	BM_LOG2( BM_LogJobWin, BmString("JobStatusWin has quit"));
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
			case BM_JOBWIN_SMTP:
			case BM_JOBWIN_POP:
			case BM_JOBWIN_IMAP: {
				if (mActiveJobs.empty()
				|| !ThePrefs->GetBool("QueueNetworkJobs", true))
					AddJob( msg);
				else
					QueueJob( msg);
				break;
			}
			case BM_JOBWIN_FILTER:
			case BM_JOBWIN_MOVEMAILS: {
				AddJob( msg);
				break;
			}
			case BM_JOB_DONE: {
				BmString name = msg->FindString(BmJobModel::MSG_JOB_NAME);
				JobMap::iterator pos = mActiveJobs.find(name);
				if (pos != mActiveJobs.end()) {
					// move job from active to done map:
					JobMap::iterator donePos = mDoneJobs.find(name);
					if (donePos != mDoneJobs.end()) {
						// job with this name is still in done queue, we need
						// to remove it now, since we are going to clobber
						// its place in the map next:
						RemoveJob(name.String());
					}
					mDoneJobs[name] = pos->second;
					mActiveJobs.erase(pos);
				}
				if (mActiveJobs.empty() && !mQueuedJobs.empty()) {
					BMessage* msg = mQueuedJobs[0];
					mQueuedJobs.pop_front();
					AddJob( msg);
					delete msg;
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("JobStatusWindow: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QueueJob( msg)
		-	queues a new job-interface for later execution
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::QueueJob( BMessage* msg) {
	BM_ASSERT( msg);

	BmString name = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);

	BM_LOG( BM_LogJobWin, BmString("Queueing job ") << name << " for addition");

	BmAutolockCheckGlobal lock( this);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "AddJob(): could not lock window");
	DetachCurrentMessage();
	mQueuedJobs.push_back(msg);
}

/*------------------------------------------------------------------------------*\
	AddJob( msg)
		-	adds a new job-interface to this window
		-	the necessary BmJobStatusView-object will be created and its 
			corresponding BmJobModel will be started (in a new thread).
		-	if job is already active, it is left alone (nothing happens)
\*------------------------------------------------------------------------------*/
void BmJobStatusWin::AddJob( BMessage* msg) {
	BM_ASSERT( msg);

	BmString name = FindMsgString( msg, BmJobModel::MSG_JOB_NAME);
	int32 jobSpecifier;
	if (msg->FindInt32( BmJobModel::MSG_JOB_SPEC, &jobSpecifier) != B_OK)
		jobSpecifier = BmJobModel::BM_DEFAULT_JOB;
	bool inNewThread;
	if (msg->FindBool( BmJobModel::MSG_JOB_THREAD, &inNewThread) != B_OK)
		inNewThread = true;
	BmJobStatusView* controller = NULL;

	BM_LOG( BM_LogJobWin, BmString("Adding job ") << name);

	BmAutolockCheckGlobal lock( this);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "AddJob(): could not lock window");

	JobMap::iterator interfaceIter = mActiveJobs.find( name);
	if (interfaceIter != mActiveJobs.end()) {
		// job is already active, so we better don't disturb:
		return;
	}

	// job is inactive, so we create a new controller for it:
	BM_LOG2( BM_LogJobWin, BmString("Creating new view for ") << name);
	switch( msg->what) {
		case BM_JOBWIN_POP: {
			bool autoMode = msg->FindBool( BmRecvAccountList::MSG_AUTOCHECK);
			controller = new BmPopperView( name.String(), autoMode);
			break;
		}
		case BM_JOBWIN_IMAP: {
			bool autoMode = msg->FindBool( BmRecvAccountList::MSG_AUTOCHECK);
			controller = new BmImapView( name.String(), autoMode);
			break;
		}
		case BM_JOBWIN_SMTP:
			controller = new BmSmtpView( name.String());
			break;
		case BM_JOBWIN_FILTER:
			controller = new BmMailFilterView( name.String());
			break;
		case BM_JOBWIN_MOVEMAILS:
			controller = new BmMailMoverView( name.String());
			break;
		default:
			break;
	}

	// add the new interface to our view:
	mOuterGroup->AddChild( dynamic_cast<BView*>(controller));
	BRect frame = controller->Frame();
	float height = frame.bottom+2;
	ResizeTo( Frame().Width(), height);
	RecalcSize();
	// ...and add the interface to the map:
	mActiveJobs[name] = controller;

	// finally, we create the new job...
	BmJobModel* job = controller->CreateJobModel( msg);

	// ...and activate the Job via its controller:
	BM_LOG2( BM_LogJobWin, BmString("Starting job thread "));
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
	BM_ASSERT( name);

	BM_LOG( BM_LogJobWin, BmString("Removing job ") << name);

	BmAutolockCheckGlobal lock( this);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "RemoveJob(): could not lock window");

	JobMap::iterator interfaceIter = mDoneJobs.find(name);
	if (interfaceIter == mDoneJobs.end())
		return;
	
	BmJobStatusView* controller = (*interfaceIter).second;
	if (controller) {
		BRect rect = controller->Bounds();
		mOuterGroup->RemoveChild( controller);
		ResizeBy( 0, -1-(rect.Height()));
		RecalcSize();
		mDoneJobs.erase( controller->ControllerName());

		if (mActiveJobs.empty()) {
			if (!mQueuedJobs.empty()) {
				BMessage* msg = mQueuedJobs[0];
				mQueuedJobs.pop_front();
				AddJob( msg);
				delete msg;
			} else {
				if (mDoneJobs.empty()) {
					while( !IsHidden())
						Hide();
				}
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	HasActiveJobs()
		-	determines whether or not there are still any jobs active
\*------------------------------------------------------------------------------*/
bool BmJobStatusWin::HasActiveJobs() {
	BmAutolockCheckGlobal lock( this);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "HasActiveJobs(): could not lock window");
	JobMap::iterator iter;
	for( iter = mActiveJobs.begin(); iter != mActiveJobs.end(); ++iter) {
		BmJobStatusView* jobView = iter->second;
		if (jobView && jobView->IsJobRunning())
			return true;
	}
	return false;
}
