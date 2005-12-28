/*
	BmStoredActionManager.cpp
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


#include <Autolock.h>
#include <File.h>
#include <Path.h>

#include "BmBasics.h"
#include "BmDataModel.h"
#include "BmStoredActionManager.h"
#include "BmLogHandler.h"
#include "BmMailMonitor.h"

//******************************************************************************
// #pragma mark -	BmStoredActionFlusher
//		-	a class that runs in its own thread and flushes any stored actions
//			to disk at an appropriate time (when the mail-monitor is idle).
//******************************************************************************
BmStoredActionFlusher* BmStoredActionFlusher::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmStoredActionFlusher* BmStoredActionFlusher::CreateInstance() {
	if (!theInstance)
		theInstance = new BmStoredActionFlusher();
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	BmStoredActionFlusher()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmStoredActionFlusher::BmStoredActionFlusher()
	:	mLocker("StoredActionFlusher")
	,	mShouldRun(false)
	,	mThreadId(-1)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	Run()
		-	
\*------------------------------------------------------------------------------*/
void BmStoredActionFlusher::Run()
{
	mShouldRun = true;
	// start new thread for worker:
	BmString tname( "StoredActionFlusher");
	mThreadId = spawn_thread( BmStoredActionFlusher::_ThreadEntry, 
									  tname.String(), B_LOW_PRIORITY, this);
	if (mThreadId < 0)
		throw BM_runtime_error("StoredActionFlusher::Run(): Could not spawn thread");
	resume_thread( mThreadId);
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	
\*------------------------------------------------------------------------------*/
void BmStoredActionFlusher::Quit()
{
	mShouldRun = false;
	status_t exitVal;
	wait_for_thread(mThreadId, &exitVal);
}

/*------------------------------------------------------------------------------*\
	_ThreadEntry()
		-	
\*------------------------------------------------------------------------------*/
int32 BmStoredActionFlusher::_ThreadEntry(void* data)
{
	BmStoredActionFlusher* worker = static_cast<BmStoredActionFlusher*>(data);
	if (worker)
		worker->_Loop();
	return B_OK;
}	

/*------------------------------------------------------------------------------*\
	_Loop()
		-	
\*------------------------------------------------------------------------------*/
void BmStoredActionFlusher::_Loop()
{
	BmRef< BmListModel> list;
	while(mShouldRun) {
		if (TheMailMonitor->IsIdle() && mLocker.Lock()) {
			ListSet::iterator iter = mListSet.begin();
			if (iter != mListSet.end()) {
				list = *iter;
				mListSet.erase(iter);
				BM_LOG2( BM_LogMailTracking, 
						   BmString("StoredActionFlusher: picked and removed "
						   	"list-model ") << list->ModelName());
			} else
				list = NULL;
			mLocker.Unlock();
		}
		if (list)
			_FlushList(list);
		else
			snooze(200*1000);
	}
}

/*------------------------------------------------------------------------------*\
	AddList( list)
		-	
\*------------------------------------------------------------------------------*/
void BmStoredActionFlusher::AddList( BmRef<BmListModel> list) {
	if (mLocker.Lock()) {
		BM_LOG( BM_LogMailTracking, 
				  BmString("StoredActionFlusher: adding list-model <")	
				  		<< list->ModelName());
		if (mListSet.insert(list).second == true) {
			BM_LOG( BM_LogMailTracking, 
					  BmString("StoredActionFlusher: added list-model <")	
					  		<< list->ModelName() 
					  		<< ">\nnumber of lists to be flushed is " 
					  		<< mListSet.size());
		}
		mLocker.Unlock();
	}
}

/*------------------------------------------------------------------------------*\
	_FlushList( list)
		-	
\*------------------------------------------------------------------------------*/
void BmStoredActionFlusher::_FlushList( BmRef<BmListModel>& list) {
	if (!list)
		return;
	try {
		BM_LOG( BM_LogMailTracking, 
				  BmString("StoredActionFlusher: flushing list-model ")	
				  		<< list->ModelName());
		list->FlushStoredActions();
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("StoredActionFlusher: ") << err.what());
	}
}



//******************************************************************************
// #pragma mark - BmStoredActionManager
// 	-	a class that manages a set of stored actions
//		-	actions are cached up to a specified maximum amount and are written
//			to disk (appended to the settings-/cache-file) once this amount
//			is exceeded.
//		-	every BmListModel delegates the writing of stored actions to its
//			own BmStoredActionManager.
//******************************************************************************

/*------------------------------------------------------------------------------*\
	BmStoredActionManager()
		-	
\*------------------------------------------------------------------------------*/
BmStoredActionManager::BmStoredActionManager(BmListModel* list)
	:	mList(list)
	,	mMaxCacheSize(1)
{
}

/*------------------------------------------------------------------------------*\
	BmStoredActionManager()
		-	
\*------------------------------------------------------------------------------*/
BmStoredActionManager::~BmStoredActionManager()
{
}

/*------------------------------------------------------------------------------*\
	StoreAction()
		-	adds given archive at end of settings-file
\*------------------------------------------------------------------------------*/
bool BmStoredActionManager::StoreAction(BMessage* action)
{
	BmAutolockCheckGlobal lock( mList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "StoreAction(): Unable to get lock");
	bool result = true;
	mActionVect.push_back(action);
	if (mActionVect.size() >= mMaxCacheSize)
		result = Flush();
	else if (TheStoredActionFlusher) {
		// add our list to the flusher, such that it will be flushed to disk
		// automatically at an appropriate time:
		TheStoredActionFlusher->AddList(mList);
	}		
	return result;
}

/*------------------------------------------------------------------------------*\
	Flush()
		-	appends all stored actions to the settings-file
\*------------------------------------------------------------------------------*/
bool BmStoredActionManager::Flush()
{
	BmAutolockCheckGlobal lock( mList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Flush(): Unable to get lock");

	if (mActionVect.empty())
		return true;

	BFile file;
	status_t err;

	try {
		BM_LOG( BM_LogMailTracking, 
				  BmString("Flushing stored actions for list-model ")
				  		<< mList->ModelName());
		BMallocIO mallocIO;
		mallocIO.SetBlockSize(mActionVect.size()*1024);
		BMessage* action;
		for( uint32 i=0; i<mActionVect.size(); ++i) {
			action = mActionVect[i];
			if ((err = action->Flatten( &mallocIO)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not flatten stored actions\n\n Result: ") 
						<< strerror(err)
				);
			delete action;
		}
		mActionVect.clear();
		BmString filename = mList->SettingsFileName();
		err = file.SetTo( filename.String(), B_WRITE_ONLY | B_OPEN_AT_END);
		if (err == B_ENTRY_NOT_FOUND) {
			// file does not exist yet, we try to create it through Store():
			mList->Store();
			if ((err = file.SetTo( 
				filename.String(), 
				B_WRITE_ONLY | B_OPEN_AT_END
			))	!= B_OK) {
				// Store() didn't create any file, so we can't append to it. This
				// is normal behaviour in case the list has not been read yet (which
				// means that the list is incomplete, so we won't write a (incomplete)
				// cache-file:
				return false;
			}
		}
		ssize_t sz = file.Write( mallocIO.Buffer(), mallocIO.BufferLength());
		if (sz < 0)
			BM_THROW_RUNTIME( BmString("Could not write to settings-file\n\t<")
									 	<< filename << ">\n\n Result: " 
									 	<< strerror(sz));
		return true;
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		return false;
	}
}
