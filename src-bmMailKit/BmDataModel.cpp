/*
	BmDataModel.cpp
		$Id$
*/

#include <memory>

#include <Autolock.h>

#include "BmApp.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmUtil.h"

/********************************************************************************\
	BmDataModel
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmDataModel( name)
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmDataModel::BmDataModel( const BString& name) 
	:	mModelName( name)
	,	mModelLocker( (BString("beam_dm_") << name).String())
{
}

/*------------------------------------------------------------------------------*\
	~BmDataModel()
		-	
\*------------------------------------------------------------------------------*/
BmDataModel::~BmDataModel() {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << "-destructor: Unable to get lock on controller-set");
	if ( !mControllerSet.empty()) {
		WaitForAllControllers();
	}
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> is dead now");
}

/*------------------------------------------------------------------------------*\
	AddController( controller)
		-	adds a new controller to the list of interested parties
		-	trying to add a controller that is already interested does no harm, 
			the controller just won't be added again.
\*------------------------------------------------------------------------------*/
void BmDataModel::AddController( BmController* controller) {
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is adding controller " << controller->ControllerName());
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":AddController(): Unable to get lock on controller-set");
	mControllerSet.insert( controller);
}

/*------------------------------------------------------------------------------*\
	RemoveController( controller)
		-	removes a controller from the list of interested parties
		-	trying to remove a controller that has not been interested before does
			no harm.
\*------------------------------------------------------------------------------*/
void BmDataModel::RemoveController( BmController* controller) {
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is removing controller " << controller->ControllerName());
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":RemoveController(): Unable to get lock on controller-set");
	mControllerSet.erase( controller);
}

/*------------------------------------------------------------------------------*\
	TellControllers( msg)
		-	tells controllers about our new state (msg holding the state-description)
		-	if no interested controller exists, this does nothing
\*------------------------------------------------------------------------------*/
void BmDataModel::TellControllers( BMessage* msg) {
	BHandler* controller;
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Unable to get lock on controller-set");
	BmControllerSet::iterator iter;
	for( iter=mControllerSet.begin(); iter != mControllerSet.end(); ++iter) {
		(controller = (*iter)->GetControllerHandler())
													|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Controller found with NULL-handler!");
		BM_LOG2( BM_LogModelController, BString("Talking to handler ") << (*iter)->ControllerName());
		controller->Looper()->PostMessage( msg, controller);
	}
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	checks whether this datamodell is still required to do its work
		-	this implementation returns true as long as there is at least one controller
			interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::ShouldContinue() {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":ShouldContinue(): Unable to get lock on controller-set");
	return !mControllerSet.empty();
}

/*------------------------------------------------------------------------------*\
	WaitForAllControllers()
		-	waits until all this model's controllers have detached
\*------------------------------------------------------------------------------*/
void BmDataModel::WaitForAllControllers() {
	// wait for all controllers to detach:
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> waits for controllers to detach");
	while( !mControllerSet.empty()) {
		mModelLocker.Unlock();
		snooze(200*1000);
		BM_LOG3( BM_LogModelController, BString("Job <") << ModelName() << "> is still waiting for controllers to detach");
		mModelLocker.Lock();
	}
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> has no more controllers");
}


/********************************************************************************\
	BmJobModel
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	StartJobThread()
		-	
\*------------------------------------------------------------------------------*/
int32 BmJobModel::ThreadStartFunc( void* data) {
	BmJobModel* job = static_cast<BmJobModel*>( data);
	if (job) {
		job->doStartJob();
		BM_LOG2( BM_LogModelController, BString("Job <") << job->ModelName() << "> has finished");
		job->WaitForAllControllers();
		delete job;
		return 0;
	} else {
		BM_LOGERR("StartJobThread(): Data-pointer is no BmJobModel");
		return 10;
	}
}

/*------------------------------------------------------------------------------*\
	BmJobModel( name)
		-	contructor
\*------------------------------------------------------------------------------*/
BmJobModel::BmJobModel( const BString& name)
	:	BmDataModel( name)
	,	mJobState( JOB_INITIALIZED)
	,	mThreadID( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobModel()
		-	destructor
\*------------------------------------------------------------------------------*/
BmJobModel::~BmJobModel() {
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> is being deleted");
}

/*------------------------------------------------------------------------------*\
	StartJobInThread()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInThread() {
	if (!mThreadID) {
		// we create a new thread for this job...
		thread_id t_id = spawn_thread( &BmJobModel::ThreadStartFunc, ModelName().String(),
												 B_NORMAL_PRIORITY, this);
		if (t_id < 0)
			throw BM_runtime_error("StartJob(): Could not spawn thread");

		mThreadID = t_id;

		// finally, we activate the job:
		BM_LOG2( BM_LogModelController, BString("Starting job thread ") << t_id);
		resume_thread( t_id);
	} else {
		BM_LOGERR( BString("Trying to start a job that is already running"));
	}
}

/*------------------------------------------------------------------------------*\
	doStartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::doStartJob() {
	BM_LOG2( BM_LogModelController, BString("Job thread <") << ModelName() << "> has started");
	mJobState = JOB_RUNNING;
	StartJob();
}

/*------------------------------------------------------------------------------*\
	PauseJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::PauseJob() {
	mJobState = JOB_PAUSED;
}

/*------------------------------------------------------------------------------*\
	ContinueJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::ContinueJob() {
	mJobState = JOB_RUNNING;
}

/*------------------------------------------------------------------------------*\
	StopJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StopJob() {
	mJobState = JOB_STOPPED;
	TellJobIsDone( false);
}

/*------------------------------------------------------------------------------*\
	IsJobRunning()
		-	
\*------------------------------------------------------------------------------*/
bool BmJobModel::IsJobRunning() const {
	return (mJobState == JOB_RUNNING || mJobState == JOB_PAUSED);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	
\*------------------------------------------------------------------------------*/
bool BmJobModel::ShouldContinue() {
	// check if we are in pause mode, if so we wait till we snap out of it:
	while( mJobState == JOB_PAUSED) {
		snooze(200*1000);
	}
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":ShouldContinue(): Unable to get lock on controller-set");
	return (!mControllerSet.empty() && mJobState == JOB_RUNNING);
}

/*------------------------------------------------------------------------------*\
	TellJobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::TellJobIsDone( bool completed) {
	BString name = ModelName();
	auto_ptr<BMessage> msg( new BMessage( BM_JOB_DONE));
	msg->AddBool( MSG_COMPLETED, completed);
	mJobState = JOB_DYING;
	BM_LOG2( BM_LogModelController, BString("Job <") << name << "> tells it is done");
	TellControllers( msg.get());
}
