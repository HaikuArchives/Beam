/*
	BmDataModel.cpp
		$Id$
*/

#include <File.h>

#include "BmBasics.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmResources.h"
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
	,	mModelLocker( (BString("beam_dm_") << name).Truncate(B_OS_NAME_LENGTH).String(), false)
	,	mFrozenCount( 0)
{
}

/*------------------------------------------------------------------------------*\
	~BmDataModel()
		-	
\*------------------------------------------------------------------------------*/
BmDataModel::~BmDataModel() {
	BM_LOG2( BM_LogModelController, BString("DataModel <") << ModelName() << "> is being deleted");
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << "-destructor: Unable to get lock on controller-set");
	if ( HasControllers()) {
		WaitForAllToDetach();
	}
	BM_LOG2( BM_LogModelController, BString("DataModel <") << ModelName() << "> is dead now");
}

/*------------------------------------------------------------------------------*\
	AddController( controller)
		-	adds a new controller to the list of interested parties
		-	trying to add a controller that is already interested does no harm, 
			the controller just won't be added again.
\*------------------------------------------------------------------------------*/
void BmDataModel::AddController( BmController* controller) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":AddController(): Unable to get lock on controller-set");
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is adding controller " << controller->ControllerName());
	// add controller to set of connected controllers:
	mControllerSet.insert( controller);
}

/*------------------------------------------------------------------------------*\
	RemoveController( controller)
		-	removes a controller from the list of interested parties
		-	trying to remove a controller that has not been interested before does
			no harm.
\*------------------------------------------------------------------------------*/
void BmDataModel::RemoveController( BmController* controller) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":RemoveController(): Unable to get lock on controller-set");
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> is removing controller " << controller->ControllerName());
	mControllerSet.erase( controller);
	mOutstandingSet.erase( controller);
}

/*------------------------------------------------------------------------------*\
	ControllerAck( controller)
		-	notes the acknowledgement of a controller (that it has reveived the
			last message that required an ack)
\*------------------------------------------------------------------------------*/
void BmDataModel::ControllerAck( BmController* controller) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":ControllerAck(): Unable to get lock on controller-set");
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> has received ack from controller " << controller->ControllerName());
	// add controller to set of connected controllers:
	mOutstandingSet.erase( controller);
}

/*------------------------------------------------------------------------------*\
	TellControllers( msg)
		-	tells controllers about our new state (msg holding the state-description)
		-	if no interested controller exists, this does nothing
		-	if this dataModel is frozen, nothing happens
\*------------------------------------------------------------------------------*/
void BmDataModel::TellControllers( BMessage* msg, bool waitForAck) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Unable to get lock on controller-set");
	BHandler* controller;
	status_t err;
	if (Frozen()) {
		BM_LOG3( BM_LogModelController, BString("Model <") << ModelName() << "> is frozen, so this msg is being dropped.");
		return;
	}
	msg->AddString( MSG_MODEL, ModelName().String());
	if (waitForAck) {
		InitOutstanding();
		msg->AddBool( MSG_NEEDS_ACK, true);
	}
	BmControllerSet::iterator iter;
	iter = mControllerSet.begin();
	if (iter == mControllerSet.end()) {
		BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> has no controllers to talk to.");
	}
	for( ; iter != mControllerSet.end(); ++iter) {
		(controller = (*iter)->GetControllerHandler())
													|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): Controller found with NULL-handler!");
		BMessenger msgr( controller);
		BM_LOG2( BM_LogModelController, BString("Talking to handler ") << (*iter)->ControllerName());
		(err=msgr.SendMessage( msg)) == B_OK
													|| BM_THROW_RUNTIME( mModelName << ":TellControllers(): SendMessage() failed!\n\n Result: " << strerror(err));
	}
	if (waitForAck)
		WaitForAllToAck();
}

/*------------------------------------------------------------------------------*\
	HasControllers()
		-	returns true as long as there is at least one controller
			interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::HasControllers() {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( mModelName << ":HasControllers(): Unable to get lock on controller-set");
	return !mControllerSet.empty();
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	checks whether this datamodel is still required to do its work
		-	this implementation returns true as long as there is at least one controller
			interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::ShouldContinue() {
	return HasControllers();
}

/*------------------------------------------------------------------------------*\
	InitOutstanding()
		-	create list of all controllers 
\*------------------------------------------------------------------------------*/
void BmDataModel::InitOutstanding() {
	mOutstandingSet = mControllerSet;
}

/*------------------------------------------------------------------------------*\
	WaitForAllToAck()
		-	waits until all this model's controllers have acknowledged a message
			that required so (for instance the removal of an item from a list)
\*------------------------------------------------------------------------------*/
void BmDataModel::WaitForAllToAck() {
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> waits for controllers to ack");
	while( ShouldContinue() && mOutstandingSet.size()) {
		int lCount=0;
		while (mModelLocker.IsLocked()) {
			mModelLocker.Unlock();
			lCount++; 
		}
		snooze(50*1000);
		BM_LOG3( BM_LogModelController, BString("Model <") << ModelName() << "> is still waiting for some controllers to ack:");
		BmControllerSet::iterator iter;
		for( iter = mOutstandingSet.begin(); iter != mOutstandingSet.end(); ++iter) {
			BM_LOG3( BM_LogModelController, BString("... <") << (*iter)->ControllerName() << "> has still not ack'd!");
		}
		while(lCount--) {
			mModelLocker.Lock();
		}
	}
	BM_LOG2( BM_LogModelController, BString("All controllers of model <") << ModelName() << "> have ack'd");
}

/*------------------------------------------------------------------------------*\
	WaitForAllToDetach()
		-	waits until all this model's controllers have detached
\*------------------------------------------------------------------------------*/
void BmDataModel::WaitForAllToDetach() {
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> waits for controllers to detach");
	while( HasControllers()) {
		int lCount=0;
		while (mModelLocker.IsLocked()) {
			mModelLocker.Unlock();
			lCount++; 
		}
		snooze(200*1000);
		BM_LOG3( BM_LogModelController, BString("Model <") << ModelName() << "> is still waiting for some controllers to detach:");
		BmControllerSet::iterator iter;
		for( iter = mControllerSet.begin(); iter != mControllerSet.end(); ++iter) {
			BM_LOG3( BM_LogModelController, BString("... <") << (*iter)->ControllerName() << "> is still attached!");
		}
		while(lCount--) {
			mModelLocker.Lock();
		}
	}
	BM_LOG2( BM_LogModelController, BString("Model <") << ModelName() << "> has no more controllers");
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
		{
			BmAutolock lock( job->mModelLocker);
			lock.IsLocked()	 						|| BM_THROW_RUNTIME( job->ModelName() << ":ThreadStartFunc(): Unable to get lock on controller-set");
			BM_LOG2( BM_LogModelController, BString("Thread is started for job <") << job->ModelName() << ">");
			job->doStartJob();
			BM_LOG2( BM_LogModelController, BString("Job <") << job->ModelName() << "> has finished");
			job->mThreadID = 0;
		}
		job->RemoveRef();
							// indicate that this thread has no more interest in jobmodel
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
	,	mJobSpecifier( BM_DEFAULT_JOB)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobModel()
		-	destructor
\*------------------------------------------------------------------------------*/
BmJobModel::~BmJobModel() {
}

/*------------------------------------------------------------------------------*\
	StartJobInNewThread()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInNewThread( int32 jobSpecifier) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ": Unable to get lock");
	mJobSpecifier = jobSpecifier;
	if (!mThreadID) {
		// we create a new thread for this job...
		BString tname = ModelName();
		tname.Truncate( B_OS_NAME_LENGTH);
		thread_id t_id = spawn_thread( &BmJobModel::ThreadStartFunc, tname.String(),
												 B_NORMAL_PRIORITY, this);
		if (t_id < 0)
			throw BM_runtime_error("StartJob(): Could not spawn thread");

		mThreadID = t_id;

		// we add another ref to ourselves (which belongs to the thread that
		// we spawned above:
		AddRef();

		// finally, we activate the job:
		BM_LOG2( BM_LogModelController, BString("Starting job thread ") << t_id);
		mJobState = JOB_RUNNING;
		resume_thread( t_id);
	} else {
		BM_LOGERR( BString("Trying to start a job that is already running"));
	}
}

/*------------------------------------------------------------------------------*\
	StartJobInThisThread()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInThisThread( int32 jobSpecifier) {
	mJobSpecifier = jobSpecifier;
	doStartJob();
}

/*------------------------------------------------------------------------------*\
	doStartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::doStartJob() {
	BM_LOG2( BM_LogModelController, BString("Job <") << ModelName() << "> has started");
	mJobState = JOB_RUNNING;
	int lCount=0;
	while (mModelLocker.IsLocked()) {
		mModelLocker.Unlock();
		lCount++; 
	}
	try {
		bool result = StartJob();
		TellJobIsDone( result && mJobState != JOB_STOPPED);
	} catch( exception& e) {
		BM_SHOWERR( e.what());
	}
	while(lCount--) {
		mModelLocker.Lock();
	}
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
		BmAutolock lock( mModelLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":StopJob(): Unable to get lock");
		mJobState = JOB_STOPPED;
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
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":ShouldContinue(): Unable to get lock on controller-set");
	return (HasControllers() && mJobState == JOB_RUNNING);
}

/*------------------------------------------------------------------------------*\
	TellJobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmJobModel::TellJobIsDone( bool completed) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":TellJobIsDone(): Unable to get lock");
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

const BString BmListModelItem::nEmptyParentKey = "empty";

/*------------------------------------------------------------------------------*\
	ListModelItem( key, parent)
		-	c'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::BmListModelItem( BString key, BmListModel* model, BmListModelItem* parent)
	:	mKey( key)
	,	mListModel( model)
	,	mParent( parent)
{
}

/*------------------------------------------------------------------------------*\
	ListModelItem( item)
		-	copy c'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::BmListModelItem( const BmListModelItem& item)
	:	mKey( item.Key())
	,	mListModel( NULL)
	,	mParent( NULL)
{
}

/*------------------------------------------------------------------------------*\
	~ListModelItem()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::~BmListModelItem() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes version of item-format into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmListModelItem::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inheritedArchivable::Archive( archive, deep)
		||	archive->AddInt16( MSG_VERSION, ArchiveVersion()));
	return ret;
}

/*------------------------------------------------------------------------------*\
	AddSubItem()
		-	
\*------------------------------------------------------------------------------*/
bool BmListModelItem::AddSubItem( BmListModelItem* subItem) {
	BmModelItemMap::iterator iter = mSubItemMap.find( subItem->Key());
	if (iter == mSubItemMap.end()) {
		mSubItemMap[subItem->Key()] = subItem;
		subItem->Parent( this);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	AddSubItem()
		-	
\*------------------------------------------------------------------------------*/
void BmListModelItem::RemoveSubItem( BmListModelItem* subItem) {
	mSubItemMap.erase( subItem->Key());
}

/*------------------------------------------------------------------------------*\
	TellModelItemUpdated()
		-	
\*------------------------------------------------------------------------------*/
void BmListModelItem::TellModelItemUpdated( BmUpdFlags flags=UPD_ALL) {
	BmRef<BmListModel> listModel( ListModel());
	if (listModel)
		listModel->TellModelItemUpdated( this, flags);
}

/*------------------------------------------------------------------------------*\
	FindItemByKey()
		-	N.B.: The ListModel this item lives in must be locked when calling
			this method, otherwise "bad things"(TM) happen!
\*------------------------------------------------------------------------------*/
BmListModelItem* BmListModelItem::FindItemByKey( const BString& key) {
	BmListModelItem* found = NULL;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); !found && iter != end(); iter++) {
		BmListModelItem* item = iter->second.Get();
		if (item->Key() == key)
			return item;
		else if (!item->empty())
			found = item->FindItemByKey( key);
	}
	return found;
}

/*------------------------------------------------------------------------------*\
	ListModel()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmListModel> BmListModelItem::ListModel() const	{ 
	return mListModel.Get(); 
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
	,	mInitCheck( B_NO_INIT)
{
}

/*------------------------------------------------------------------------------*\
	~ListModel()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmListModel::~BmListModel() {
}

/*------------------------------------------------------------------------------*\
	Cleanup()
		-	frees occupied memory
\*------------------------------------------------------------------------------*/
void BmListModel::Cleanup() {
	BmAutolock lock( mModelLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ":Cleanup(): Unable to get lock");
	mModelItemMap.clear();
	mInitCheck = B_NO_INIT;
}

/*------------------------------------------------------------------------------*\
	AddItemToList()
		-	
\*------------------------------------------------------------------------------*/
bool BmListModel::AddItemToList( BmListModelItem* item, BmListModelItem* parent) {
	if (item) {
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":AddItemToList(): Unable to get lock");
		if (parent) {
			if (parent->AddSubItem( item)) {
				TellModelItemAdded( item);
				if (parent->size() == 1) {
					// the parent has just become a superitem (and thus needs to be updated):
					TellModelItemUpdated( parent, UPD_EXPANDER);
				}
				return true;
			}
		}
		else {
			BmModelItemMap::iterator iter = mModelItemMap.find( item->Key());
			if (iter == mModelItemMap.end()) {
				mModelItemMap[item->Key()] = item;
				item->Parent( NULL);
				TellModelItemAdded( item);
				return true;
			}
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::RemoveItemFromList( BmListModelItem* item) {
	if (item) {
		BmAutolock lock( mModelLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelName() << ":RemoveItemFromList(): Unable to get lock");
		BmRef<BmListModelItem> parent = item->Parent();
		TellModelItemRemoved( item);
		if (parent) {
			parent->RemoveSubItem( item);
			if (parent->size() == 0) {
				// the parent has just become a non-superitem (and thus needs to be updated):
				TellModelItemUpdated( parent.Get(), UPD_EXPANDER);
			}
		} else {
			mModelItemMap.erase( item->Key());
		}
	}
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmListModel::RemoveItemFromList( const BString& key) {
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":RemoveItemFromList(): Unable to get lock");
	BmRef<BmListModelItem> item( FindItemByKey( key));
	RemoveItemFromList( item.Get());
	return item;
}

/*------------------------------------------------------------------------------*\
	FindItemByKey()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmListModel::FindItemByKey( const BString& key) {
	BmListModelItem* found = NULL;
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":FindItemByKey(): Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); !found && iter != end(); iter++) {
		BmListModelItem* item = iter->second.Get();
		if (item->Key() == key)
			return BmRef<BmListModelItem>( item);
		else if (!item->empty())
			found = item->FindItemByKey( key);
	}
	return BmRef<BmListModelItem>( found);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemAdded( BmListModelItem* item) {
	if (Frozen())
		return;
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
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
	if (Frozen())
		return;
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemRemoved(): Unable to get lock");
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_REMOVE);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> tells about removed item " << item->Key());
		TellControllers( &msg, true);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemUpdated( BmListModelItem* item, BmUpdFlags flags) {
	if (Frozen())
		return;
	BmAutolock lock( mModelLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( ModelName() << ":TellModelItemUpdated(): Unable to get lock");
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_UPDATE);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		msg.AddInt32( MSG_UPD_FLAGS, flags);
		BM_LOG2( BM_LogModelController, BString("ListModel <") << ModelName() << "> tells about updated item " << item->Key());
		TellControllers( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmListModel::Archive( BMessage* archive, bool deep) const {
	status_t ret = BArchivable::Archive( archive, deep);
	if (ret == B_OK) {
		ret = archive->AddInt32( BmListModelItem::MSG_NUMCHILDREN, size());
	}
	if (deep && ret == B_OK) {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); iter != end() && ret == B_OK; ++iter) {
			BMessage msg;
			ret = iter->second->Archive( &msg, deep)
					|| archive->AddMessage( BmListModelItem::MSG_CHILDREN, &msg);
		}
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores List inside Settings-dir:
\*------------------------------------------------------------------------------*/
bool BmListModel::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	if (mInitCheck != B_OK) return true;
	try {
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":Store(): Unable to get lock");
		BString filename = SettingsFileName();
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME(BString("Unable to archive list-model ")<<ModelName());
		(err = cacheFile.SetTo( filename.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create settings-file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	Restore()
		-	restores List from Settings-file (if it exists):
\*------------------------------------------------------------------------------*/
BMessage* BmListModel::Restore( const BString filename) {
	status_t err;
	BFile file;
	BMessage* archive = NULL;

	// try to open settings-file...
	if ((err = file.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, settings file found, we fetch our data from it:
		try {
			archive = new BMessage;
			(err = archive->Unflatten( &file)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch settings from file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		} catch (exception &e) {
			BM_SHOWERR( e.what());
			delete archive;
			archive = NULL;
		}
	}
	return archive;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmListModel::StartJob() {
	// try to open cache-file (if any) or else initialize the long way
	if (InitCheck() == B_OK) {
		return true;
	}

	Freeze();
	try {
		BMessage* archive = Restore( SettingsFileName());
		if (archive) {
			InstantiateItems( archive);
			delete archive;
		} else {
			// ...no cache file found, we fetch the existing items by hand...
			InitializeItems();
		}
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	Thaw();
	return InitCheck() == B_OK;
}
