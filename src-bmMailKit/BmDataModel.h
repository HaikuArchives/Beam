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

#include "BmMailKit.h"

#include <map>
#include <memory>
#include <set>
#include <vector>

#include <Locker.h>
#include "BmString.h"

#include "BmRefManager.h"

class BmController;

/*------------------------------------------------------------------------------*\
	message types for BmDataModel (and subclasses), all msgs are sent to 
	the handler specified via each Controllers GetControllerHandler()-method,
	so these messages are sent from a datamodel to all its controllers:
\*------------------------------------------------------------------------------*/
enum {
	BM_JOB_DONE					=	'bmda',
							// the job has finished or was stopped
	BM_JOB_UPDATE_STATE		=	'bmdb',
							// the job wants to update (one of) its state(s)
	BM_LISTMODEL_ADD			=	'bmdc',
							// the listmodel has added a new item
	BM_LISTMODEL_REMOVE		=	'bmdd',
							// the listmodel has removed an item
	BM_LISTMODEL_UPDATE		=	'bmde'
							// the listmodel indicates a state-change of one of 
							// its items
};

/*------------------------------------------------------------------------------*\
	BmDataModel
		-	an interface for informing other objects (e.g. MVC-like 
			controllers) to be informed about state-changes of this
			object.
		-	contains functionality to add/remove controllers
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmDataModel : public BmRefObj {
	typedef set< BmController*> BmControllerSet;
	
	friend class MailMonitorTest;

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
	inline const BmString& ModelName() const	
													{ return mModelName; }
	inline BmString ModelNameNC() const	{ return mModelName; }
	inline BLocker& ModelLocker() const	{ return mModelLocker; }

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
	inline bool NeedControllersToContinue()	
													{ return mNeedControllersToContinue; }

	inline void NeedControllersToContinue( bool b)	
													{ mNeedControllersToContinue = b; }

	mutable BLocker mModelLocker;
	BmControllerSet mControllerSet;
	BmControllerSet mOutstandingSet;
	int8 mFrozenCount;
	bool mNeedControllersToContinue;

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
class IMPEXPBMMAILKIT BmJobModel : public BmDataModel {
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
	inline thread_id JobThreadID() const
													{ return mThreadID; }
	bool IsJobRunning() const;
	virtual bool IsJobCompleted() const;
	inline int32 CurrentJobSpecifier() const	
													{ return mJobSpecifier; }

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
IMPEXPBMMAILKIT const BmUpdFlags UPD_EXPANDER 	= 1<<0;
IMPEXPBMMAILKIT const BmUpdFlags UPD_KEY		 	= 1<<1;
IMPEXPBMMAILKIT const BmUpdFlags UPD_ALL 			= 0xFFFFFFFFUL;

class BmListModelItem;
typedef map< BmString, BmRef<BmListModelItem> > BmModelItemMap;
/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmListModelItem : public BmRefObj, public BArchivable {
	typedef BmRefObj inherited;
	typedef BArchivable inheritedArchivable;
	friend class BmListModel;

protected:
	static const char* const MSG_VERSION;

public:
	// c'tors & d'tor:
	BmListModelItem( const BmString& key, BmListModel* model, 
						  BmListModelItem* parent);
	virtual ~BmListModelItem();

	// native methods:
	BmListModelItem* FindItemByKey( const BmString& key);
	virtual int16 ArchiveVersion() const = 0;
	virtual void ExecuteAction(BMessage* action);
	void ItemIsValid( bool b);
	virtual bool SanityCheck( BmString& complaint, BmString& fieldName) const
													{ return true; }

	struct Collector {
		virtual bool operator() (const BmListModelItem* listItem) = 0;
	};
	bool ForEachSubItem(BmListModelItem::Collector& collector) const;

	// getters:
	BmModelItemMap::const_iterator begin() const;
	BmModelItemMap::const_iterator end() const;
	size_t size() const;
	bool empty() const;

	inline const BmString& Key() const	{ return mKey; }
	virtual const BmString& DisplayKey() const		
													{ return mKey; }
	inline BmRef<BmListModelItem> Parent() const		
													{ return mParent; }
	inline bool ItemIsValid() const		{ return mItemIsValid; }
	uint32 OutlineLevel() const;
	BmRef<BmListModel> ListModel() const;

	// setters:
	inline void Parent( BmListModelItem* parent)	
													{ mParent = parent; }
	inline void Key( const BmString k)	{ mKey = k; }

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
	bool mItemIsValid;

private:

	// Hide assignment:
#ifndef __POWERPC__
	BmListModelItem( const BmListModelItem&);
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
class IMPEXPBMMAILKIT BmListModel : public BmJobModel, public BArchivable {
	typedef BmJobModel inherited;
	friend BmListModelItem;

	struct BmForeignKey {
		BmWeakRef<BmListModel> foreignListModel;
		BmString keyName;
	};
	typedef vector< BmForeignKey> BmForeignKeyVect;
	
protected:
	static const char* const MSG_VERSION;

	class StoredActionManager {
		typedef vector<BMessage*> ActionVect;
	public:
		StoredActionManager(BmListModel* list);
		~StoredActionManager();
		//
		bool StoreAction(BMessage* action);
		bool Flush();
		//
		void MaxCacheSize(uint32 maxCacheSize)
													{ mMaxCacheSize = maxCacheSize; }
	private:
		ActionVect mActionVect;
		BmListModel* mList;
		uint32 mMaxCacheSize;
	};
	
public:
	// c'tors & d'tor:
	BmListModel( const BmString& name, uint32 logTerrain);
	virtual ~BmListModel();

	// native methods:
	BmRef<BmListModelItem> FindItemByKey( const BmString& key);
	virtual bool AddItemToList( BmListModelItem* item, 
										 BmListModelItem* parent=NULL);
	virtual void RemoveItemFromList( BmListModelItem* item);
	virtual BmRef<BmListModelItem> RemoveItemByKey( const BmString& key);
	virtual void SetItemValidity(  BmListModelItem* item, bool isValid);
	//
	virtual BmString RenameItem( const BmString oldKey, const BmString& newKey);
	virtual void AddForeignKey( const char* key, BmListModel* model);
	void AdjustForeignKeys( const BmString& oldVal, const BmString& newVal);
	virtual void ForeignKeyChanged( const BmString& /*key*/, 
											  const BmString& /*oldVal*/, 
											  const BmString& /*newVal*/)	
											  		{ }
	//
	virtual bool Store();
	virtual const BmString SettingsFileName() = 0;
	virtual void InitializeItems()		{ mInitCheck = B_OK; }
	virtual void InstantiateItemsFromStream( BDataIO* dataIO, BMessage* headerMsg = NULL);
	virtual void InstantiateItems( BMessage* archive);
	virtual void InstantiateItem( BMessage* archive)
													{ }
	virtual void Cleanup();
	virtual int16 ArchiveVersion() const = 0;

	BMessage* Restore( BDataIO* dataIO);
	bool StoreAction(BMessage* action);
	bool RestoreAndExecuteActionsFrom(BDataIO* dataIO);
	virtual void ExecuteAction(BMessage* action);

	bool ForEachItem(BmListModelItem::Collector& collector) const;

	// overrides of Archivable base:
	status_t Archive( BMessage* archive, bool deep) const;

	//	message component definitions for status-msgs:
	static const char* const MSG_ITEMKEY;
	static const char* const MSG_PARENTKEY;
	static const char* const MSG_MODELITEM;
	static const char* const MSG_UPD_FLAGS;
	static const char* const MSG_OLD_KEY;

	// getters:
	BmModelItemMap::const_iterator begin() const;
	BmModelItemMap::const_iterator end() const;
	size_t size() const;
	bool empty() const;

	inline status_t InitCheck() const	{ return mInitCheck; }

	inline size_t ValidCount() const		{ return mModelItemMap.size()
																- mInvalidCount; }
	inline size_t InvalidCount() const	{ return mInvalidCount; }
	// setters:
	inline void IncInvalidCount()			{ mInvalidCount++; }
	inline void DecInvalidCount()			{ mInvalidCount--; }

protected:

	// native methods:
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item, 
												  BmUpdFlags flags=UPD_ALL,
												  const BmString oldKey="");

	// overrides of job-model base:
	void TellJobIsDone( bool completed=true);
	bool StartJob();

	status_t mInitCheck;
	bool mNeedsStore;
	BmForeignKeyVect mForeignKeyVect;
	int32 mInvalidCount;
	StoredActionManager mStoredActionManager;
	uint32 mLogTerrain;

private:
	// Hide copy-constructor and assignment:
	BmListModel( const BmListModel&);
#ifndef __POWERPC__
	BmListModel operator=( const BmListModel&);
#endif

	BmModelItemMap mModelItemMap;
};

#endif
