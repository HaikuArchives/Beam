/*
	BmDataModel.cpp
		$Id$
*/

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
	,	mModelLocker( (BString("beam_dm_") << name).String(), false)
	,	mStateVal( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmDataModel()
		-	
\*------------------------------------------------------------------------------*/
BmDataModel::~BmDataModel() {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << "-destructor: Unable to get lock on controller-set");
	if ( HasControllers()) {
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
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":AddController(): Unable to get lock on controller-set");
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is adding controller " << controller->ControllerName());
	// insert controller with state-value of 0 (indicating that the new controller 
	//	should receive state-updates the next time we get to look at it):
	mControllerMap[controller] = 0;
}

/*------------------------------------------------------------------------------*\
	RemoveController( controller)
		-	removes a controller from the list of interested parties
		-	trying to remove a controller that has not been interested before does
			no harm.
\*------------------------------------------------------------------------------*/
void BmDataModel::RemoveController( BmController* controller) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":RemoveController(): Unable to get lock on controller-set");
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is removing controller " << controller->ControllerName());
	mControllerMap.erase( controller);
}

/*------------------------------------------------------------------------------*\
	TellControllers( msg)
		-	tells controllers about our new state (msg holding the state-description)
		-	if no interested controller exists, this does nothing
\*------------------------------------------------------------------------------*/
void BmDataModel::TellControllers( BMessage* msg, bool bumpStateVal) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Unable to get lock on controller-set");
	BHandler* controller;
	if (bumpStateVal) mStateVal++;
	msg->AddString( MSG_MODEL, ModelName().String());
	BmControllerMap::iterator iter;
	for( iter = mControllerMap.begin(); iter != mControllerMap.end(); ++iter) {
		(controller = iter->first->GetControllerHandler())
													|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Controller found with NULL-handler!");
		if (iter->second < mStateVal) {
			BM_LOG2( BM_LogModelController, BString("Talking to handler ") << iter->first->ControllerName());
			controller->Looper()->PostMessage( msg, controller);
			if (bumpStateVal) {
				iter->second = mStateVal;
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	HasControllers()
		-	returns true as long as there is at least one controller
			interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::HasControllers() {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":HasControllers(): Unable to get lock on controller-set");
	return !mControllerMap.empty();
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	checks whether this datamodell is still required to do its work
		-	this implementation returns true as long as there is at least one controller
			interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::ShouldContinue() {
	return HasControllers();
}

/*------------------------------------------------------------------------------*\
	WaitForAllControllers()
		-	waits until all this model's controllers have detached
\*------------------------------------------------------------------------------*/
void BmDataModel::WaitForAllControllers() {
	// wait for all controllers to detach:
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> waits for controllers to detach");
	while( HasControllers()) {
		int lCount=0;
		while (mModelLocker.IsLocked()) {
			mModelLocker.Unlock();
			lCount++; 
		}
		snooze(200*1000);
		BM_LOG3( BM_LogModelController, BString("Job <") << ModelName() << "> is still waiting for controllers to detach");
		while(lCount--) {
			mModelLocker.Lock();
		}
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
		BAutolock lock( job->mModelLocker);
		lock.IsLocked()	 						|| BM_THROW_RUNTIME( job->ModelName() << ":ThreadStartFunc(): Unable to get lock on controller-set");
		job->doStartJob();
		BM_LOG2( BM_LogModelController, BString("Job <") << job->ModelName() << "> has finished");
		if (job->DeleteWhenDone() || job->mJobState != JOB_COMPLETED) {
			delete job;
		} else {
			job->mThreadID = 0;
		}
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
	,	mDeleteWhenDone( false)
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
	StartJobInNewThread()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInNewThread( bool deleteWhenDone) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
	if (!mThreadID) {
		mDeleteWhenDone = deleteWhenDone;
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
	StartJobInThisThread()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInThisThread() {
	doStartJob();
}

/*------------------------------------------------------------------------------*\
	doStartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::doStartJob() {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
	BM_LOG2( BM_LogModelController, BString("Job thread <") << ModelName() << "> has started");
	mJobState = JOB_RUNNING;
	StartJob();
	TellJobIsDone();
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
	if (IsJobRunning()) {
		mJobState = JOB_STOPPED;
		TellJobIsDone( false);
	}
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
	return (HasControllers() && mJobState == JOB_RUNNING);
}

/*------------------------------------------------------------------------------*\
	TellJobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::TellJobIsDone( bool completed) {
	BString name = ModelName();
	BMessage msg( BM_JOB_DONE);
	msg.AddBool( MSG_COMPLETED, completed);
	if (completed)
		mJobState = JOB_COMPLETED;
	BM_LOG2( BM_LogModelController, BString("Job <") << name << "> tells it is done");
	TellControllers( &msg);
}



/********************************************************************************\
	BmListModelItem
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	ListModelItem( key, parent)
		-	c'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::BmListModelItem( BString key, BmListModelItem* parent)
	:	mKey( key)
	,	mParent( parent)
{
}

/*------------------------------------------------------------------------------*\
	~ListModelItem()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::~BmListModelItem() {
	BmModelItemMap::iterator iter;
	for( iter = mSubItemMap.begin(); iter != mSubItemMap.end(); ++iter) {
		BmListModelItem* item = iter->second;
		delete item;
	}
	mSubItemMap.clear();
}

/*------------------------------------------------------------------------------*\
	AddSubItem()
		-	
\*------------------------------------------------------------------------------*/
void BmListModelItem::AddSubItem( BmListModelItem* subItem) {
	mSubItemMap[subItem->Key()] = subItem;
}

/*------------------------------------------------------------------------------*\
	FindItemByKey()
		-	
\*------------------------------------------------------------------------------*/
BmListModelItem* BmListModelItem::FindItemByKey( BString& key) {
	BmListModelItem* found;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); iter++) {
		BmListModelItem* item = iter->second;
		if (item->Key() == key)
			return item;
		else {
			found = item->FindItemByKey( key);
			if (found)
				return found;
		}
	}
	return NULL;
}


/********************************************************************************\
	BmListModel
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	ListModel()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmListModel::BmListModel( const BString& name)
	:  inherited( name)
{
}

/*------------------------------------------------------------------------------*\
	~ListModel()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmListModel::~BmListModel() {
	BmModelItemMap::iterator iter;
	for( iter = mModelItemMap.begin(); iter != mModelItemMap.end(); ++iter) {
		BmListModelItem* item = iter->second;
		delete item;
	}
	mModelItemMap.clear();
}

/*------------------------------------------------------------------------------*\
	RemovalNoticed()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::RemovalNoticed( BmController* controller) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":RemovalNoticed(): Unable to get lock on controller-set");
	mOpenReplySet.erase( controller); 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemAdded( BmListModelItem* item) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_ADD);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> tells about added item " << item->Key());
		TellControllers( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemRemoved( BmListModelItem* item) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_REMOVE);
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> tells about removed item " << item->Key());
		// we copy each controller into the set of open replies (where they remove themselves
		// from by calling RemovalNoticed()):
		BmControllerMap::iterator iter;
		for( iter=mControllerMap.begin(); iter != mControllerMap.end(); ++iter) {
			mOpenReplySet.insert( iter->first);
		}
		// we inform all controllers about the removal...
		TellControllers( &msg);
		// ...and wait for all controllers to reply to the item-removal:
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> waits for controllers to note deletion of item " << item->Key());
		while( !mOpenReplySet.empty()) {
			BM_LOG3( BM_LogModelController, BString("ListModel <") << ModelName() << "> is still waiting for " << mOpenReplySet.size() << " controllers to note deletion of item"  << item->Key());
			mModelLocker.Unlock();
			snooze(200*1000);
			mModelLocker.Lock();
		}
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> has informed all controllers about deletion of item " << item->Key());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemUpdated( BmListModelItem* item) {
	BAutolock lock( mModelLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemUpdated(): Unable to get lock");
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_UPDATE);
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> tells about updated item " << item->Key());
		TellControllers( &msg);
	}
}
