/*
	BmDataModel.cpp
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


#include <Application.h>
#include <Messenger.h>
#include <File.h>

#include "BmBasics.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"


/********************************************************************************\
	BmDataModel
\********************************************************************************/

//	message component definitions for status-msgs:
const char* const BmDataModel::MSG_MODEL = 			"bm:model";
const char* const BmDataModel::MSG_NEEDS_ACK = 		"bm:ack";

/*------------------------------------------------------------------------------*\
	BmDataModel( name)
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmDataModel::BmDataModel( const BmString& name) 
	:	mModelName( name)
	,	mModelLocker( (BmString("beam_dm_") << name)
								.Truncate(B_OS_NAME_LENGTH).String(), 
						  false)
	,	mFrozenCount( 0)
	,	mNeedControllersToContinue( true)
{
}

/*------------------------------------------------------------------------------*\
	~BmDataModel()
		-	destructor
		-	waits for all controllers to detach, so that they don't access a stale
			pointer
\*------------------------------------------------------------------------------*/
BmDataModel::~BmDataModel() {
	BM_LOG2( BM_LogModelController, 
				BmString("DataModel <") << ModelName() << "> is being deleted");
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << "-destructor: Unable to get lock on controller-set"
		);
	if ( HasControllers()) {
		WaitForAllToDetach();
	}
	BM_LOG2( BM_LogModelController, 
				BmString("DataModel <") << ModelName() << "> is dead now");
}

/*------------------------------------------------------------------------------*\
	AddController( controller)
		-	adds a new controller to the list of interested parties
		-	trying to add a controller that is already interested does no harm, 
			the controller just won't be added again.
\*------------------------------------------------------------------------------*/
void BmDataModel::AddController( BmController* controller) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":AddController(): Unable to get lock on controller-set"
		);
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() 
					<< "> is adding controller " << controller->ControllerName());
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
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":RemoveController(): Unable to get lock on controller-set"
		);
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() 
					<< "> is removing controller " << controller->ControllerName());
	mControllerSet.erase( controller);
	mOutstandingSet.erase( controller);
}

/*------------------------------------------------------------------------------*\
	ControllerAck( controller)
		-	notes the acknowledgement of a controller (that it has reveived the
			last message that required an ack)
\*------------------------------------------------------------------------------*/
void BmDataModel::ControllerAck( BmController* controller) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":ControllerAck(): Unable to get lock on controller-set"
		);
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() 
					<< "> has received ack from controller " 
					<< controller->ControllerName());
	// add controller to set of connected controllers:
	mOutstandingSet.erase( controller);
}

/*------------------------------------------------------------------------------*\
	TellControllers( msg)
		-	tells controllers about our new state (msg holding the 
			state-description)
		-	if no interested controller exists, this does nothing
		-	if this dataModel is frozen, nothing happens
\*------------------------------------------------------------------------------*/
void BmDataModel::TellControllers( BMessage* msg, bool waitForAck) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":TellControllers(): Unable to get lock on controller-set"
		);
	BHandler* controller;
	status_t err;
	if (Frozen()) {
		BM_LOG3( BM_LogModelController, 
					BmString("Model <") << ModelName() 
					<< "> is frozen, so this msg is being dropped.");
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
		BM_LOG2( BM_LogModelController, 
					BmString("Model <") << ModelName() 
					<< "> has no controllers to talk to.");
	}
	for( ; iter != mControllerSet.end(); ++iter) {
		if (!(controller = (*iter)->GetControllerHandler()))
			BM_THROW_RUNTIME( 
				ModelNameNC() 
					<< ":TellControllers(): Controller found with NULL-handler!"
			);
		BMessenger msgr( controller);
		BM_LOG2( BM_LogModelController, 
					BmString("Talking to handler ") << (*iter)->ControllerName());
		if ((err=msgr.SendMessage( msg)) != B_OK)
			BM_THROW_RUNTIME( 
				ModelNameNC() 
					<< ":TellControllers(): SendMessage() failed!\n\n Result: " 
					<< strerror(err)
			);
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
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":HasControllers(): Unable to get lock on controller-set"
		);
	return !mControllerSet.empty();
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	checks whether this datamodel is still required to do its work
		-	this implementation returns true as long as there is at least one 
			controller interested
\*------------------------------------------------------------------------------*/
bool BmDataModel::ShouldContinue() {
	return HasControllers() || !mNeedControllersToContinue;
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
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() 
					<< "> waits for controllers to ack");
	while( ShouldContinue() && mOutstandingSet.size()) {
		int lCount=0;
		while (mModelLocker.IsLocked()) {
			mModelLocker.Unlock();
			lCount++; 
		}
		snooze(50*1000);
		BM_LOG3( BM_LogModelController, 
					BmString("Model <") << ModelName() 
						<< "> is still waiting for some controllers to ack:");
		BmControllerSet::iterator iter;
		for(  iter = mOutstandingSet.begin(); 
				iter != mOutstandingSet.end(); ++iter) {
			BM_LOG3( BM_LogModelController, 
						BmString("... <") << (*iter)->ControllerName() 
							<< "> has still not ack'd!");
		}
		while(lCount--) {
			mModelLocker.Lock();
		}
	}
	BM_LOG2( BM_LogModelController, 
				BmString("All controllers of model <") << ModelName() 
					<< "> have ack'd");
}

/*------------------------------------------------------------------------------*\
	WaitForAllToDetach()
		-	waits until all this model's controllers have detached
\*------------------------------------------------------------------------------*/
void BmDataModel::WaitForAllToDetach() {
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() 
					<< "> waits for controllers to detach");
	while( HasControllers()) {
		int lCount=0;
		while (mModelLocker.IsLocked()) {
			mModelLocker.Unlock();
			lCount++; 
		}
		snooze(200*1000);
		BM_LOG3( BM_LogModelController, 
					BmString("Model <") << ModelName() 
						<< "> is still waiting for some controllers to detach:");
		BmControllerSet::iterator iter;
		for(  iter = mControllerSet.begin(); 
				iter != mControllerSet.end(); ++iter) {
			BM_LOG3( BM_LogModelController, 
						BmString("... <") << (*iter)->ControllerName() 
							<< "> is still attached!");
		}
		while(lCount--) {
			mModelLocker.Lock();
		}
	}
	BM_LOG2( BM_LogModelController, 
				BmString("Model <") << ModelName() << "> has no more controllers");
}

/*------------------------------------------------------------------------------*\
	HandleError( errStr)
		-	handles the given error by logging it and showing it to the
			user if the dataModel should currently continue
\*------------------------------------------------------------------------------*/
void BmDataModel::HandleError( const BmString& errStr) {
	if (ShouldContinue() && ThePrefs->GetBool( "ShowAlertForErrors", false)) {
		BM_SHOWERR( errStr);
	} else {
		BM_LOGERR( errStr);
		snooze(200*1000);
	}
}



/********************************************************************************\
	BmJobModel
\********************************************************************************/

//	message component definitions for status-msgs:
const char* const BmJobModel::MSG_COMPLETED = 	"bm:completed";
const char* const BmJobModel::MSG_DOMAIN = 		"bm:domain";
const char* const BmJobModel::MSG_JOB_NAME = 	"bm:jobname";

const int32 BmJobModel::BM_DEFAULT_JOB = 	0;

const char* const BmJobModel::MSG_JOB_SPEC = 	"bm:jobspec";
const char* const BmJobModel::MSG_JOB_THREAD = 	"bm:jobthread";

/*------------------------------------------------------------------------------*\
	StartJobThread( data)
		-	thread-entry function for every job-model that runs in its own thread
		-	data is a pointer to a BmJobModel that contains further info
\*------------------------------------------------------------------------------*/
int32 BmJobModel::ThreadStartFunc( void* data) {
	BmJobModel* job = static_cast<BmJobModel*>( data);
	if (job) {
		{
			BmAutolockCheckGlobal lock( job->mModelLocker);
			if (!lock.IsLocked()) {
				BM_LOGERR( 
					job->ModelNameNC() 
						<< ":ThreadStartFunc(): Unable to get lock, thread stops!"
				);
				return 10;
			}
			BM_LOG2( BM_LogModelController, 
						BmString("Thread is started for job <") << job->ModelName() 
							<< ">");
			job->doStartJob();
			BM_LOG2( BM_LogModelController, 
						BmString("Job <") << job->ModelName() << "> has finished");
			job->mThreadID = 0;
		}
		job->RemoveRef();
							// indicate that this thread has no more interest
							// in jobmodel
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
BmJobModel::BmJobModel( const BmString& name)
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
		-	spawns a new thread for this job and runs the job in that thread
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInNewThread( int32 jobSpecifier) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	if (mJobState == JOB_RUNNING)
		return; 			// job is already running, we won't disturb
	mJobSpecifier = jobSpecifier;
	if (!mThreadID) {
		// we create a new thread for this job...
		BmString tname = ModelName();
		tname.Truncate( B_OS_NAME_LENGTH);
		thread_id t_id = spawn_thread( &BmJobModel::ThreadStartFunc, 
												 tname.String(),
												 B_NORMAL_PRIORITY, this);
		if (t_id < 0)
			throw BM_runtime_error("StartJob(): Could not spawn thread");

		mThreadID = t_id;

		// we add another ref to ourselves (which belongs to the thread that
		// we spawned above:
		AddRef();

		// finally, we activate the job:
		BM_LOG2( BM_LogModelController, 
					BmString("Starting job thread ") << t_id);
		mJobState = JOB_RUNNING;
		resume_thread( t_id);
	} else {
		BM_LOG2( BM_LogModelController, 
					BmString("Trying to start a job that is already running"));
	}
}

/*------------------------------------------------------------------------------*\
	StartJobInThisThread()
		-	starts this job in the current thread
		-	if the job is still running, this method synchronizes to it, i.e.
			it blocks until the job has finished (or was stopped).
\*------------------------------------------------------------------------------*/
void BmJobModel::StartJobInThisThread( int32 jobSpecifier) {
	bool isRunning;
	{	// scope for autolock
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
		isRunning = (mJobState == JOB_RUNNING);
	}
	if (isRunning) {
		// job is already running, we need to wait till job has finished...
		while( isRunning) {
			snooze(200*1000);
			{
				BmAutolockCheckGlobal lock( mModelLocker);
				if (!lock.IsLocked())
					BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
				isRunning = (mJobState == JOB_RUNNING);
			}
 		}
	} else {
		// start job:
		mJobSpecifier = jobSpecifier;
		doStartJob();
	}
}

/*------------------------------------------------------------------------------*\
	doStartJob()
		-	executes the real job by calling the job's StartJob()-method
		-	ensures that the job-lock is free during execution of the actual job
\*------------------------------------------------------------------------------*/
void BmJobModel::doStartJob() {
	BM_LOG2( BM_LogModelController, 
				BmString("Job <") << ModelName() << "> has started");
	mJobState = JOB_RUNNING;
	int lCount=0;
	while (mModelLocker.IsLocked()) {
		mModelLocker.Unlock();
		lCount++; 
	}
	try {
		bool result = StartJob();
		TellJobIsDone( result && mJobState != JOB_STOPPED);
	} catch( BM_error& e) {
		BM_SHOWERR( e.what());
	}
	
	while(lCount--) {
		mModelLocker.Lock();
	}
}

/*------------------------------------------------------------------------------*\
	PauseJob()
		-	pauses this job, causing it to wait indefinitely, until it is
			explicitly stopped or continued
\*------------------------------------------------------------------------------*/
void BmJobModel::PauseJob() {
	mJobState = JOB_PAUSED;
}

/*------------------------------------------------------------------------------*\
	ContinueJob()
		-	continues this job
\*------------------------------------------------------------------------------*/
void BmJobModel::ContinueJob() {
	mJobState = JOB_RUNNING;
}

/*------------------------------------------------------------------------------*\
	StopJob()
		-	stops the current job
\*------------------------------------------------------------------------------*/
void BmJobModel::StopJob() {
	if (IsJobRunning()) {
		mJobState = JOB_STOPPED;
	}
}

/*------------------------------------------------------------------------------*\
	IsJobRunning()
		-	checks if this job is currently running (or paused)
\*------------------------------------------------------------------------------*/
bool BmJobModel::IsJobRunning() const {
	return (mJobState == JOB_RUNNING || mJobState == JOB_PAUSED);
}

/*------------------------------------------------------------------------------*\
	ShouldContinue()
		-	determines whether this job should continue
		-	this implementation requires a job to have interested controllers
			in order to let it continue
\*------------------------------------------------------------------------------*/
bool BmJobModel::ShouldContinue() {
	// check if we are in pause mode, if so we wait till we snap out of it:
	while( mJobState == JOB_PAUSED) {
		snooze(200*1000);
	}
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() 
				<< ":ShouldContinue(): Unable to get lock on controller-set"
		);
	return ((HasControllers() || !mNeedControllersToContinue) 
			 && mJobState == JOB_RUNNING);
}

/*------------------------------------------------------------------------------*\
	TellJobIsDone()
		-	tells all controllers about that the job has finished
		-	param completed indicates if the job has finished normally (=true)
			or was stopped (=false)
\*------------------------------------------------------------------------------*/
void BmJobModel::TellJobIsDone( bool completed) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":TellJobIsDone(): Unable to get lock"
		);
	BmString name = ModelName();
	BMessage msg( BM_JOB_DONE);
	msg.AddBool( MSG_COMPLETED, completed);
	if (completed)
		mJobState = JOB_COMPLETED;
	BM_LOG2( BM_LogModelController, 
				BmString("Job <") << name << "> tells it is done");
	TellControllers( &msg, false);
}



/********************************************************************************\
	BmListModelItem
\********************************************************************************/

const char* const BmListModelItem::MSG_VERSION  = 		"bm:version";
const char* const BmListModelItem::MSG_NUMCHILDREN = 	"bm:count";
const char* const BmListModelItem::MSG_CHILDREN = 		"bm:chld";

/*------------------------------------------------------------------------------*\
	ListModelItem( key, parent)
		-	c'tor
\*------------------------------------------------------------------------------*/
BmListModelItem::BmListModelItem( const BmString& key, BmListModel* model, 
											 BmListModelItem* parent)
	:	mKey( key)
	,	mListModel( model)
	,	mParent( parent)
	,	mItemIsValid( true)
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
	IntegrateAppendedArchive( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmListModelItem::IntegrateAppendedArchive( BMessage* /*archive*/) {
}

/*------------------------------------------------------------------------------*\
	AddSubItem( subItem)
		-	adds the given item to this listmodel-item
		-	N.B.: The ListModel this item lives in must be locked when calling
			this method, otherwise "bad things"(TM) happen!
\*------------------------------------------------------------------------------*/
bool BmListModelItem::AddSubItem( BmListModelItem* subItem) {
#ifdef BM_REF_DEBUGGING
	BmRef<BmListModel> listModel( ListModel());
	BM_ASSERT( !listModel || listModel->ModelLocker().IsLocked());
#endif
	BmModelItemMap::iterator iter = mSubItemMap.find( subItem->Key());
	if (iter == mSubItemMap.end()) {
		mSubItemMap[subItem->Key()] = subItem;
		subItem->Parent( this);
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	RemoveSubItem( subItem)
		-	removes the given subItem from this listmodel-item
		-	N.B.: The ListModel this item lives in must be locked when calling
			this method, otherwise "bad things"(TM) happen!
\*------------------------------------------------------------------------------*/
void BmListModelItem::RemoveSubItem( BmListModelItem* subItem) {
#ifdef BM_REF_DEBUGGING
	BmRef<BmListModel> listModel( ListModel());
	BM_ASSERT( listModel && listModel->ModelLocker().IsLocked());
#endif
	mSubItemMap.erase( subItem->Key());
}

/*------------------------------------------------------------------------------*\
	TellModelItemUpdated( updFlags)
		-	tells all controllers of this item's listmodel that this item has
			changed
		-	if listmodel is not known (usually because this item has not yet been
			added to a listmodel) nothing happens
		-	param updFlags indicates which part of item has been updated
\*------------------------------------------------------------------------------*/
void BmListModelItem::TellModelItemUpdated( BmUpdFlags flags) {
	BmRef<BmListModel> listModel( ListModel());
	if (listModel)
		listModel->TellModelItemUpdated( this, flags);
}

/*------------------------------------------------------------------------------*\
	FindItemByKey( key)
		-	finds the item with the given key within the sub-hierarchy of this
			listmodel-item
		-	N.B.: The ListModel this item lives in must be locked when calling
			this method, otherwise "bad things"(TM) happen!
\*------------------------------------------------------------------------------*/
BmListModelItem* BmListModelItem::FindItemByKey( const BmString& key) {
#ifdef BM_REF_DEBUGGING
	BmRef<BmListModel> listModel( ListModel());
	BM_ASSERT( listModel && listModel->ModelLocker().IsLocked());
#endif
	BmListModelItem* found = NULL;
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); !found && iter != end(); ++iter) {
		BmListModelItem* item = iter->second.Get();
		if (item->Key() == key)
			return item;
		else if (!item->empty())
			found = item->FindItemByKey( key);
	}
	return found;
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
BmModelItemMap::const_iterator BmListModelItem::begin() const	
{ 
#ifdef BM_REF_DEBUGGING
	BmRef<BmListModel> listModel( ListModel());
	if (listModel)
		BM_ASSERT( listModel->ModelLocker().IsLocked());
#endif
	return mSubItemMap.begin();
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
BmModelItemMap::const_iterator BmListModelItem::end() const	
{ 
#ifdef BM_REF_DEBUGGING
	BmRef<BmListModel> listModel( ListModel());
	if (listModel)
		BM_ASSERT( listModel->ModelLocker().IsLocked());
#endif
	return mSubItemMap.end(); 
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
size_t BmListModelItem::size() const				
{ 
	BmRef<BmListModel> listModel( ListModel());
	if (listModel) {
		BmAutolockCheckGlobal lock( listModel->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				listModel->ModelNameNC() << "size(): Unable to get lock"
			);
		return mSubItemMap.size(); 
	} else
		return 0;
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
bool BmListModelItem::empty() const				
{ 
	BmRef<BmListModel> listModel( ListModel());
	if (listModel) {
		BmAutolockCheckGlobal lock( listModel->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				listModel->ModelNameNC() << "empty(): Unable to get lock"
			);
		return mSubItemMap.empty(); 
	} else
		return true;
}

/*------------------------------------------------------------------------------*\
	ListModel()
		-	dereferences the weak-reference to the listmodel (may be NULL)
\*------------------------------------------------------------------------------*/
BmRef<BmListModel> BmListModelItem::ListModel() const	{ 
	return mListModel.Get(); 
}

/*------------------------------------------------------------------------------*\
	OutlineLevel()
		-	returns the number of parents this item has
\*------------------------------------------------------------------------------*/
uint32 BmListModelItem::OutlineLevel() const	{
	uint32 level = 0;
 	for( BmListModelItem* parent = mParent; parent; parent = parent->mParent)
 		level++;
	return level;
}

/*------------------------------------------------------------------------------*\
	ItemIsValid( b)
		-	
\*------------------------------------------------------------------------------*/
void BmListModelItem::ItemIsValid( bool _itemIsValid) {
	if (mItemIsValid != _itemIsValid) {
		BmRef<BmListModel> listModel( ListModel());
		if (listModel)
			// we have a listmodel, so we delegate the validity change to it,
			// since it will deal with required updates, too:
			listModel->SetItemValidity( this, _itemIsValid);
		else
			// no listmodel, we just change the validity:
			mItemIsValid = _itemIsValid;
	}
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
bool BmListModelItem
::ForEachSubItem(BmListModelItem::Collector& collector) const
{
	BmModelItemMap::const_iterator iter;
	for( iter=begin(); iter != end(); ++iter) {
		if (!iter->second)
			continue;
		if (!collector(iter->second.Get())
		|| !iter->second->ForEachSubItem( collector))
			return false;
	}
	return true;
}




/********************************************************************************\
	BmListModel
\********************************************************************************/

const char* const BmListModel::MSG_VERSION = "bm:version";

const char* const BmListModel::MSG_ITEMKEY 		=	"bm:ikey";
const char* const BmListModel::MSG_PARENTKEY 	= 	"bm:pkey";
const char* const BmListModel::MSG_MODELITEM 	= 	"bm:item";
const char* const BmListModel::MSG_UPD_FLAGS		= 	"bm:updflags";
const char* const BmListModel::MSG_OLD_KEY		= 	"bm:oldkey";

/*------------------------------------------------------------------------------*\
	ListModel()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmListModel::BmListModel( const BmString& name)
	:  inherited( name)
	,	mInitCheck( B_NO_INIT)
	,	mNeedsStore( false)
	,	mInvalidCount( 0)
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
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ":Cleanup(): Unable to get lock");
	mModelItemMap.clear();
	mNeedsStore = false;
	mInitCheck = B_NO_INIT;
	mInvalidCount = 0;
}

/*------------------------------------------------------------------------------*\
	AddItemToList( item, parent)
		-	adds given item to given parent-item
		-	if parent==NULL the item is added to the listmodel itself
\*------------------------------------------------------------------------------*/
bool BmListModel::AddItemToList( BmListModelItem* item, 
											BmListModelItem* parent) {
	if (item) {
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":AddItemToList(): Unable to get lock"
			);
		if (parent) {
			if (parent->AddSubItem( item)) {
				item->mListModel = this;
				if (!item->mItemIsValid)
					IncInvalidCount();
				mNeedsStore = true;
				TellModelItemAdded( item);
				if (parent->size() == 1) {
					// the parent has just become a superitem (and thus 
					// needs to be updated):
					TellModelItemUpdated( parent, UPD_EXPANDER);
				}
				return true;
			}
		} else {
			BmModelItemMap::iterator iter = mModelItemMap.find( item->Key());
			if (iter == mModelItemMap.end()) {
				mModelItemMap[item->Key()] = item;
				item->Parent( NULL);
				item->mListModel = this;
				if (!item->mItemIsValid)
					IncInvalidCount();
				mNeedsStore = true;
				TellModelItemAdded( item);
				return true;
			}
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList( item)
		-	removes the given item from this listmodel
		-	safe to call with empty item
\*------------------------------------------------------------------------------*/
void BmListModel::RemoveItemFromList( BmListModelItem* item) {
	if (item) {
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":RemoveItemFromList(): Unable to get lock"
			);
		BmRef<BmListModelItem> parent = item->Parent();
		mNeedsStore = true;
		if (parent) {
			parent->RemoveSubItem( item);
			if (parent->size() == 0) {
				// the parent has just become a non-superitem (and thus 
				// needs to be updated):
				TellModelItemUpdated( parent.Get(), UPD_EXPANDER);
			}
		} else {
			mModelItemMap.erase( item->Key());
		}
		item->mListModel = NULL;
		if (!item->mItemIsValid)
			DecInvalidCount();
		TellModelItemRemoved( item);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveItemByKey( key)
		-	removes item with given key from this listmodel's item-hierarchy
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmListModel::RemoveItemByKey( const BmString& key) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":RemoveItemFromList(): Unable to get lock"
		);
	BmRef<BmListModelItem> item( FindItemByKey( key));
	RemoveItemFromList( item.Get());
	return item;
}

/*------------------------------------------------------------------------------*\
	SetItemValidity( item, b)
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::SetItemValidity(  BmListModelItem* item, bool isValid) {
	if (item && item->mItemIsValid != isValid) {
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":SetItemValidity(): Unable to get lock"
			);
		if (isValid) {
			// item has changed from invalid to valid, we set to valid and then
			// tell controllers about its addition:
			item->mItemIsValid = true;
			DecInvalidCount();
			TellModelItemAdded( item);
		} else {
			// item has changed from valid to invalid, we first
			// tell controllers about its removal and then set item to invalid
			// (otherwise, TellModelItemRemoved would ignore our request,
			// since this item is invalid):
			TellModelItemRemoved( item);
			IncInvalidCount();
			item->mItemIsValid = false;
		}
	}
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::AddForeignKey( const char* key, BmListModel* model) {
	BmForeignKey foreignKey;
	foreignKey.keyName = key;
	foreignKey.foreignListModel = model;
	mForeignKeyVect.push_back( foreignKey);
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::AdjustForeignKeys( const BmString& oldVal, 
												 const BmString& newVal) {
	// adjust all foreign-keys (if any):
	for( uint32 i=0; i<mForeignKeyVect.size(); ++i) {
		BmRef<BmListModel> foreignList( 
			mForeignKeyVect[i].foreignListModel.Get());
		if (foreignList) {
			foreignList->ForeignKeyChanged( mForeignKeyVect[i].keyName,
													  oldVal, newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
BmModelItemMap::const_iterator BmListModel::begin() const
{ 
#ifdef BM_REF_DEBUGGING
	BM_ASSERT( mModelLocker.IsLocked());
#endif
	return mModelItemMap.begin(); 
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
BmModelItemMap::const_iterator BmListModel::end() const
{
#ifdef BM_REF_DEBUGGING
	BM_ASSERT( mModelLocker.IsLocked());
#endif
	return mModelItemMap.end(); 
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
size_t BmListModel::size() const
{ 
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << "size(): Unable to get lock");
	return mModelItemMap.size(); 
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
bool BmListModel::empty() const
{
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << "empty(): Unable to get lock");
	return mModelItemMap.empty(); 
}

/*------------------------------------------------------------------------------*\
		-	
\*------------------------------------------------------------------------------*/
bool BmListModel::ForEachItem(BmListModelItem::Collector& collector) const
{
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << "ForEachItem(): Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter=begin(); iter != end(); ++iter) {
		if (!iter->second)
			continue;
		if (!collector(iter->second.Get())
		|| !iter->second->ForEachSubItem( collector))
			return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	RenameItem( oldKey, suggestedNewKey)
		-	renames the item that has the given key oldKey into suggestedNewKey
		-	if suggestedNewKey is not unique, a unique new key is generated
		-	the real new key of the item is returned
		-	N.B.: oldKey is passed in by-value so that it does not change during 
			the rename (so that old value is available afterwards).
\*------------------------------------------------------------------------------*/
BmString BmListModel::RenameItem( const BmString oldKey, 
											 const BmString& suggestedNewKey) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ":RenameItem(): Unable to get lock");
	// find unique key for item:
	BmString newKey=suggestedNewKey;
	BmRef<BmListModelItem> doubleItem;
	if (oldKey != newKey) {
		// if key has actually changed, we check if the new key already exists
		for( int32 i = 1; (doubleItem=FindItemByKey( newKey)); ++i) {
			newKey = suggestedNewKey+"("<<i<<")";
							// find a unique new key
		}
	}
	// now find item under old key...
	BmRef<BmListModelItem> item( FindItemByKey( oldKey));
	if (!item)
		return oldKey;
	// ...and rename it:
	mModelItemMap.erase( oldKey);
	item->RenameRef( newKey.String());
	item->Key( newKey);
	mModelItemMap[newKey] = item.Get();
	TellModelItemUpdated( item.Get(), UPD_KEY, oldKey);
	// ...determine path to item (needed for handling of foreign-keys):
	BmString path;
	BmListModelItem* curr = item.Get();
	while( (curr = curr->Parent().Get()) != NULL) {
		if (!path.Length())
			path.Prepend( curr->DisplayKey());
		else		
			path.Prepend( curr->DisplayKey() + "/");
	}
	// now adjust foreign keys:
	AdjustForeignKeys( path.Length() ? path + "/" + oldKey : oldKey,
							 path.Length() ? path + "/" + newKey : newKey);
	// return new key (might differ from the one suggested):
	return newKey;
}

/*------------------------------------------------------------------------------*\
	FindItemByKey( key)
		-	finds and returns the item that has the given key
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmListModel::FindItemByKey( const BmString& key) {
	BmListModelItem* found = NULL;
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":FindItemByKey(): Unable to get lock"
		);
	BmModelItemMap::const_iterator iter = mModelItemMap.find( key);
	if (iter != end()) {
		found = iter->second.Get();
	} else {
		for( iter = begin(); !found && iter != end(); ++iter) {
			BmListModelItem* item = iter->second.Get();
			if (!item->empty())
				found = item->FindItemByKey( key);
		}
	}
	return found;
}

/*------------------------------------------------------------------------------*\
	TellModelItemAdded( item)
		-	tells all controllers that the given item has been added to hierarchy
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemAdded( BmListModelItem* item) {
	if (Frozen() || !item->ItemIsValid())
		return;
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":TellModelItemRemoved(): Unable to get lock"
		);
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_ADD);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		// since each message will reference the item, we add
		// as many refs to the item as we have controllers:
		for( uint32 i=0; i<mControllerSet.size(); ++i)
			item->AddRef();
		BM_LOG2( BM_LogModelController, 
					BmString("ListModel <") << ModelName() 
						<< "> tells about added item " << item->Key());
		TellControllers( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	TellModelItemRemoved( item)
		-	tells all controllers that the given item has been removed from 
			hierarchy
		-	the controllers are requested to acknowledge the removal, so that
			they won't access the removed item (stale pointer)
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemRemoved( BmListModelItem* item) {
	if (Frozen() || !item->ItemIsValid())
		return;
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":TellModelItemRemoved(): Unable to get lock"
		);
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_REMOVE);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		// since each message will reference the item, we add
		// as many refs to the item as we have controllers:
		for( uint32 i=0; i<mControllerSet.size(); ++i)
			item->AddRef();
		BM_LOG2( BM_LogModelController, 
					BmString("ListModel <") << ModelName() 
						<< "> tells about removed item " << item->Key());
		TellControllers( &msg, true);
	}
}

/*------------------------------------------------------------------------------*\
	TellModelItemUpdated( item, updFlags, oldKey)
		-	tells all controllers that the given item has been updated
		-	param updFlags indicates which part of the item has been changed
		-	param oldKey contains the old key of the item if the item was renamed
\*------------------------------------------------------------------------------*/
void BmListModel::TellModelItemUpdated( BmListModelItem* item, 
													 BmUpdFlags flags,
													 const BmString oldKey) {
	if (Frozen() || !item->ItemIsValid())
		return;
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":TellModelItemUpdated(): Unable to get lock"
		);
	if (HasControllers()) {
		BMessage msg( BM_LISTMODEL_UPDATE);
		msg.AddPointer( MSG_MODELITEM, static_cast<void*>(item));
		// since each message will reference the item, we add
		// as many refs to the item as we have controllers:
		for( uint32 i=0; i<mControllerSet.size(); ++i)
			item->AddRef();
		msg.AddInt32( MSG_UPD_FLAGS, flags);
		if (oldKey.Length())
			// item has been renamed, we include old key:
			msg.AddString( MSG_OLD_KEY, oldKey.String());
		BM_LOG2( BM_LogModelController, 
					BmString("ListModel <") << ModelName() 
						<< "> tells about updated item " << item->Key());
		TellControllers( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	TellJobIsDone()
		-	extends job-model method with updating of menu-controllers
\*------------------------------------------------------------------------------*/
void BmListModel::TellJobIsDone( bool completed) {
	inherited::TellJobIsDone( completed);
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	archives the listmodel with all items into the given message
\*------------------------------------------------------------------------------*/
status_t BmListModel::Archive( BMessage* archive, bool deep) const {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":Archive(): Unable to get lock"
		);
	status_t ret = BArchivable::Archive( archive, deep);
	if (ret == B_OK) {
		ret = archive->AddInt32( BmListModelItem::MSG_NUMCHILDREN, size());
		ret = archive->AddInt16( BmListModelItem::MSG_VERSION, ArchiveVersion());
	}
	if (deep && ret == B_OK) {
		BM_LOG( BM_LogModelController, 
				  BmString("ListModel <") << ModelName() << "> begins to archive");
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); iter != end() && ret == B_OK; ++iter) {
			BMessage msg;
			ret = iter->second->Archive( &msg, deep)
					|| archive->AddMessage( BmListModelItem::MSG_CHILDREN, &msg);
		}
		BM_LOG( BM_LogModelController, 
				  BmString("ListModel <") << ModelName() 
				  	<< "> finished with archive");
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores List inside Settings-dir
\*------------------------------------------------------------------------------*/
bool BmListModel::Store() {
	BMessage archive;
	BmBackedFile arcFile;
	status_t err;

	if (mInitCheck != B_OK)
		return true;
	try {
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( ModelNameNC() << ":Store(): Unable to get lock");
		BmString filename = SettingsFileName();
		if (this->Archive( &archive, true) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Unable to archive list-model ")<<ModelName()
			);
		if ((err = arcFile.SetTo( filename.String())) != B_OK) {
			BM_THROW_RUNTIME( BmString("Could not create settings-file\n\t<") 
										<< filename << ">\n\n Result: " << strerror(err));
		}
		if ((err = archive.Flatten( &arcFile.File())) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not store settings into file\n\t<") 
										<< filename << ">\n\n Result: " << strerror(err));
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	Restore()
		-	restores List from Settings-file (if it exists)
\*------------------------------------------------------------------------------*/
BMessage* BmListModel::Restore( const BmString filename, 
										  BList& appendedArchives) {
	status_t err;
	BFile file;
	BMessage* archive = NULL;

	// try to open settings-file...
	if ((err = file.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, settings file found, we fetch our data from it:
		try {
			archive = new BMessage;
			if ((err = archive->Unflatten( &file)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not fetch settings from file\n\t<") 
						<< filename << ">\n\n Result: " << strerror(err)
				);
			FetchAppendedArchives( &file, &appendedArchives);
		} catch (BM_error &e) {
			BM_SHOWERR( e.what());
			delete archive;
			archive = NULL;
		}
	}
	return archive;
}

/*------------------------------------------------------------------------------*\
	FetchAppendedArchives()
		-	fetches appended archives from given file and puts them into given list
\*------------------------------------------------------------------------------*/
void BmListModel::FetchAppendedArchives( BDataIO* dataIO, BList* appendedArchives) {
	BMessage* appendedArchive = new BMessage;
	while (appendedArchive->Unflatten( dataIO) == B_OK) {
		appendedArchives->AddItem( appendedArchive);
		appendedArchive = new BMessage;
	}
	delete appendedArchive;
}

/*------------------------------------------------------------------------------*\
	AppendArchive()
		-	adds given archive at end of settings-file
\*------------------------------------------------------------------------------*/
bool BmListModel::AppendArchive( BMessage* archive) {
	BFile file;
	status_t err;

	try {
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":AppendArchive(): Unable to get lock"
			);
		BmString filename = SettingsFileName();
		err = file.SetTo( filename.String(), B_WRITE_ONLY | B_OPEN_AT_END);
		if (err == B_ENTRY_NOT_FOUND) {
			// file does not exist yet, we try to create it through Store():
			Store();
			if ((err = file.SetTo( 
				filename.String(), 
				B_WRITE_ONLY | B_OPEN_AT_END | B_CREATE_FILE
			))	!= B_OK)
				BM_THROW_RUNTIME( BmString("Could not append to settings-file\n\t<")
										 	<< filename << ">\n\n Result: " 
										 	<< strerror(err));
		}
		if ((err = archive->Flatten( &file)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not append settings to file\n\t<") 
										<< filename << ">\n\n Result: " << strerror(err));
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	IntegrateAppendedArchives( list)
		-	
\*------------------------------------------------------------------------------*/
void BmListModel::IntegrateAppendedArchives( BList& appendedArchives) {
	int32 count = appendedArchives.CountItems();
	BmString key;
	BMessage* archive;
	for( int i=0; i<count; ++i) {
		archive = static_cast< BMessage*>( appendedArchives.ItemAt( i));
		if (archive) {
			key = archive->FindString( MSG_ITEMKEY);
			BmRef<BmListModelItem> item( FindItemByKey( key));
			if (item)
				item->IntegrateAppendedArchive( archive);
			delete archive;
		}
	}
	appendedArchives.MakeEmpty();
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	executes the default job for a listmodel
		-	tries to read the listmodel-hierarchy from a cache-file 
			via InstantiateItems()
		-	if no cache-file exists, list-model is initialized afresh via
			InitializeItems()
\*------------------------------------------------------------------------------*/
bool BmListModel::StartJob() {
	// try to open cache-file (if any) or else initialize the long way
	if (InitCheck() == B_OK) {
		return true;
	}

	Freeze();
	try {
		BList appendedArchives(100);
		BMessage* archive = Restore( SettingsFileName(), appendedArchives);
		if (archive) {
			InstantiateItems( archive);
			delete archive;
		} else {
			// ...no cache file found, we fetch the existing items by hand...
			InitializeItems();
		}
		if (appendedArchives.CountItems() > 0)
			IntegrateAppendedArchives( appendedArchives);
	} catch (BM_error &e) {
		BM_SHOWERR( e.what());
	}

	Thaw();
	return InitCheck() == B_OK;
}
