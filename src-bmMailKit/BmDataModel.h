/*
	BmDataModel.h
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


#ifndef _BmDataModel_h
#define _BmDataModel_h

#include <map>
#include <memory>
#include <set>

#include <Locker.h>
#include "BmString.h"

#include "BmRefManager.h"

class BmController;

/*------------------------------------------------------------------------------*\
	message types for BmDataModel (and subclasses), all msgs are sent to 
	the handler specified via each Controllers GetControllerHandler()-method,
	so these messages are sent from a datamodel to all its controllers:
\*------------------------------------------------------------------------------*/
#define BM_JOB_DONE						'bmda'
							// the job has finished or was stopped
#define BM_JOB_UPDATE_STATE			'bmdb'
							// the job wants to update (one of) its state(s)
#define BM_LISTMODEL_ADD				'bmdc'
							// the listmodel has added a new item
#define BM_LISTMODEL_REMOVE			'bmdd'
							// the listmodel has removed an item
#define BM_LISTMODEL_UPDATE			'bmde'
							// the listmodel indicates a state-change of on of its items


/*------------------------------------------------------------------------------*\
	BmDataModel
		-	an interface for informing other objects (e.g. MVC-like 
			controllers) to be informed about state-changes of this
			object.
		-	contains functionality to add/remove controllers
\*------------------------------------------------------------------------------*/
class BmDataModel : public BmRefObj {
	typedef set< BmController*> BmControllerSet;

public:
	// c'tors & d'tor:
	BmDataModel( const BmString& name);
	virtual ~BmDataModel();

	// native methods:
	virtual void AddController( BmController* controller);
	virtual void ControllerAck( BmController* controller);
	virtual void RemoveController( BmController* controller);

	// getters:
	inline const BmString& Name() const	{ return mModelName; }
	inline const BmString& ModelName() const	{ return mModelName; }
	inline BmString ModelNameNC() const	{ return mModelName; }
	inline BLocker& ModelLocker() 		{ return mModelLocker; }

	// overrides of BmRefObj
	const BmString& RefName() const		{ return mModelName; }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL;
	static const char* const MSG_NEEDS_ACK;

protected:
	// native methods:
	virtual bool HasControllers();
	virtual void InitOutstanding();
	virtual bool ShouldContinue();
	virtual void TellControllers( BMessage* msg, bool waitForAck=false);
	virtual void WaitForAllToAck();
	virtual void WaitForAllToDetach();
	virtual void HandleError( const BmString& errString);
	inline void Freeze() 					{ mFrozenCount++; }
	inline void Thaw()						{ mFrozenCount--; }
	inline bool Frozen() 					{ return mFrozenCount > 0; }

	BLocker mModelLocker;
	BmControllerSet mControllerSet;
	BmControllerSet mOutstandingSet;
	int8 mFrozenCount;

private:
	// Hide copy-constructor and assignment:
	BmDataModel( const BmDataModel&);
	BmDataModel operator=( const BmDataModel&);

	BmString mModelName;
};

/*------------------------------------------------------------------------------*\
	BmJobModel
		-	an interface that extends a datamodel with the ability to execute a 
			specific job in its own thread and tell the controllers when it is done
		-	supports pause-, continue- and stop-functionalities
\*------------------------------------------------------------------------------*/
class BmJobModel : public BmDataModel {
	typedef BmDataModel inherited;

public:
	// c'tors & d'tor:
	BmJobModel( const BmString& name);
	virtual ~BmJobModel();

	static const int32 BM_DEFAULT_JOB;

	// native methods:
	static int32 ThreadStartFunc(  void*);
	virtual void StartJobInNewThread( int32 jobSpecifier=BM_DEFAULT_JOB);
	virtual void StartJobInThisThread( int32 jobSpecifier=BM_DEFAULT_JOB);
	virtual void PauseJob();
	virtual void ContinueJob();
	virtual void StopJob();
	inline thread_id JobThreadID() const		{ return mThreadID; }
	bool IsJobRunning() const;
	inline bool IsJobCompleted() const			{ return mJobState == JOB_COMPLETED; }
	inline int32 CurrentJobSpecifier() const	{ return mJobSpecifier; }

	//	message component definitions for status-msgs:
	static const char* const MSG_COMPLETED;
	static const char* const MSG_DOMAIN;
	static const char* const MSG_JOB_NAME;

	static const char* const MSG_JOB_SPEC;
	static const char* const MSG_JOB_THREAD;

protected:
	// native methods:
	virtual bool StartJob() 				= 0;
	virtual bool ShouldContinue();
	virtual void TellJobIsDone( bool completed=true);

	enum BmJobState { JOB_INITIALIZED = 1, 
							JOB_RUNNING, 
							JOB_PAUSED, 
							JOB_STOPPED, 
							JOB_COMPLETED};
	BmJobState mJobState;

	BmJobState JobState() const 			{ return mJobState; }

	int32 mJobSpecifier;

private:
	// Hide copy-constructor and assignment:
	BmJobModel( const BmJobModel&);
#ifndef __POWERPC__
	BmJobModel operator=( const BmJobModel&);
#endif

	virtual void doStartJob();

	thread_id mThreadID;
};

// flags indicating which parts are to be updated:
typedef uint32 BmUpdFlags;
const BmUpdFlags UPD_EXPANDER 	= 1<<0;
const BmUpdFlags UPD_KEY		 	= 1<<1;
const BmUpdFlags UPD_ALL 			= 0xFFFFFFFF;

class BmListModelItem;
typedef map< BmString, BmRef<BmListModelItem> > BmModelItemMap;
/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class BmListModelItem : public BmRefObj, public BArchivable {
	typedef BmRefObj inherited;
	typedef BArchivable inheritedArchivable;
	friend class BmListModel;

protected:
	static const char* const MSG_VERSION;

public:
	// c'tors & d'tor:
	BmListModelItem( const BmString& key, BmListModel* model, BmListModelItem* parent);
	BmListModelItem( const BmListModelItem&);
	virtual ~BmListModelItem();

	// native methods:
	BmListModelItem* FindItemByKey( const BmString& key);
	virtual int16 ArchiveVersion() const = 0;

#ifdef BM_LOGGING
	virtual int32 ObjectSize( bool addSizeofThis=true) const;
#endif

	// getters:
	inline BmModelItemMap::const_iterator begin() const	{ return mSubItemMap.begin(); }
	inline BmModelItemMap::const_iterator end() const	{ return mSubItemMap.end(); }
	inline size_t size() const						{ return mSubItemMap.size(); }
	inline bool empty() const						{ return mSubItemMap.empty(); }
	inline const BmString& Key() const			{ return mKey; }
	inline BmListModelItem* Parent() const		{ return mParent; }
	BmRef<BmListModel> ListModel() const;

	// setters:
	inline void Parent( BmListModelItem* parent)	{ mParent = parent; }
	inline void Key( const BmString k)				{ mKey = k; }

	// overrides of BmRefObj
	const BmString& RefName() const		{ return mKey; }

	// overrides of BArchivable
	status_t Archive( BMessage* archive, bool deep = true) const;

	//	message component definitions for status-msgs:
	static const char* const MSG_NUMCHILDREN;
	static const char* const MSG_CHILDREN;

protected:
	// native methods:
	bool AddSubItem( BmListModelItem* subItem);
	void RemoveSubItem( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmUpdFlags flags=UPD_ALL);

	BmListModelItem* mParent;
	BmWeakRef<BmListModel> mListModel;

private:
	// Hide assignment:
#ifndef __POWERPC__
	BmListModelItem operator=( const BmListModelItem&);
#endif
	
	BmString mKey;
	BmModelItemMap mSubItemMap;

};

/*------------------------------------------------------------------------------*\
	BmListModel
		-	an interface that extends BmJobModel with the ability to
			handle list-objects
		-	contains functionality to add/remove objects to/from the list
		-	any attaching controller automatically receives a list of all 
			objects currently contained in the list-model
\*------------------------------------------------------------------------------*/
class BmListModel : public BmJobModel, public BArchivable {
	typedef BmJobModel inherited;
	friend BmListModelItem;
	
protected:
	static const char* const MSG_VERSION;

public:
	// c'tors & d'tor:
	BmListModel( const BmString& name);
	virtual ~BmListModel();

	// native methods:
	BmRef<BmListModelItem> FindItemByKey( const BmString& key);
	bool AddItemToList( BmListModelItem* item, BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	BmString RenameItem( const BmString oldKey, const BmString newKey);
	BmRef<BmListModelItem> RemoveItemFromList( const BmString& key);
	virtual bool Store();
	virtual const BmString SettingsFileName() = 0;
	static BMessage* Restore( const BmString settingsFile);
	virtual void InitializeItems()								{	mInitCheck = B_OK; }
	virtual void InstantiateItems( BMessage*)					{ mInitCheck = B_OK; }
	virtual void Cleanup();
	virtual int16 ArchiveVersion() const = 0;

	// overrides of Archivable base:
	status_t Archive( BMessage* archive, bool deep) const;

	//	message component definitions for status-msgs:
	static const char* const MSG_ITEMKEY;
	static const char* const MSG_PARENTKEY;
	static const char* const MSG_MODELITEM;
	static const char* const MSG_UPD_FLAGS;
	static const char* const MSG_OLD_KEY;

	// getters:
	inline BmModelItemMap::const_iterator begin() const 	
															{ return mModelItemMap.begin(); }
	inline BmModelItemMap::const_iterator end() const		
															{ return mModelItemMap.end(); }
	inline size_t size() const						{ return mModelItemMap.size(); }
	inline bool empty() const						{ return mModelItemMap.empty(); }
	inline status_t InitCheck() const			{ return mInitCheck; }

protected:

	// native methods:
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item, BmUpdFlags flags=UPD_ALL,
												  const BmString oldKey="");

	// overrides of job-model base:
	bool StartJob();

	status_t mInitCheck;
	bool mNeedsStore;

private:
	// Hide copy-constructor and assignment:
	BmListModel( const BmListModel&);
#ifndef __POWERPC__
	BmListModel operator=( const BmListModel&);
#endif

	BmModelItemMap mModelItemMap;
};

#endif
