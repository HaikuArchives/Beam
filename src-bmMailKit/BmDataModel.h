/*
	BmDataModel.h
		$Id$
*/

#ifndef _BmDataModel_h
#define _BmDataModel_h

#include <map>
#include <memory>
#include <set>

#include <Autolock.h>
#include <Locker.h>
#include <Looper.h>
#include <String.h>

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
	BmDataModel( const BString& name);
	virtual ~BmDataModel();

	// native methods:
	virtual void AddController( BmController* controller);
	virtual void RemoveController( BmController* controller);

	// getters:
	BString Name() const						{ return mModelName; }
	BString ModelName() const				{ return mModelName; }
	BLocker& ModelLocker() 					{ return mModelLocker; }

	// overrides of BmRefObj
	const BString& RefName() const		{ return mModelName; }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL = 			"bm:model";

protected:
	// native methods:
	virtual bool HasControllers();
	virtual bool ShouldContinue();
	virtual void TellControllers( BMessage* msg);
	virtual void WaitForAllControllers();
	inline void Freeze() 					{ mFrozenCount++; }
	inline void Thaw()						{ mFrozenCount--; }
	inline bool Frozen() 					{ return mFrozenCount > 0; }

	BLocker mModelLocker;
	BmControllerSet mControllerSet;
	int8 mFrozenCount;

private:
	BString mModelName;
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
	BmJobModel( const BString& name);
	virtual ~BmJobModel();

	// native methods:
	static int32 ThreadStartFunc(  void*);
	virtual void StartJobInNewThread( int32 jobSpecifier=BM_DEFAULT_JOB);
	virtual void StartJobInThisThread( int32 jobSpecifier=BM_DEFAULT_JOB);
	virtual void PauseJob();
	virtual void ContinueJob();
	virtual void StopJob();
	thread_id JobThreadID() const 		{ return mThreadID; }
	bool IsJobRunning() const;
	bool IsJobCompleted() const			{ return mJobState == JOB_COMPLETED; }
	int32 CurrentJobSpecifier() const	{ return mJobSpecifier; }

	//	message component definitions for status-msgs:
	static const char* const MSG_COMPLETED = 		"bm:completed";
	static const char* const MSG_DOMAIN = 			"bm:domain";
	
	static const int32 BM_DEFAULT_JOB = 0;

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

private:
	virtual void doStartJob();

	thread_id mThreadID;
	int32 mJobSpecifier;
};

// flags indicating which parts are to be updated:
typedef uint32 BmUpdFlags;
const BmUpdFlags UPD_EXPANDER 	= 1<<0;
const BmUpdFlags UPD_ALL 			= 0xFFFFFFFF;

class BmListModelItem;
typedef map< BString, BmRef<BmListModelItem> > BmModelItemMap;
typedef BmRef< BmListModelItem> BmListModelItemRef;
/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class BmListModelItem : public BmRefObj, public BArchivable {
	friend class BmListModel;

public:
	// c'tors & d'tor:
	BmListModelItem( BString key, BmListModel* model, BmListModelItem* parent);
	virtual ~BmListModelItem();

	// native methods:
	BmListModelItem* FindItemByKey( const BString& key);

	// getters:
	BmModelItemMap::const_iterator begin() const	{ return mSubItemMap.begin(); }
	BmModelItemMap::const_iterator end() const	{ return mSubItemMap.end(); }
	size_t size() const						{ return mSubItemMap.size(); }
	bool empty() const						{ return mSubItemMap.empty(); }
	const BString& Key() const				{ return mKey; }
	const BString& ParentKey() const		{ return mParent ? mParent->Key() : nEmptyParentKey; }
	BmListModelItem* Parent() const		{ return mParent; }
	BmListModel* ListModel() const		{ return mListModel; }

	// setters:
	void Parent( BmListModelItem* parent)	{ mParent = parent; }
	void Key( const BString k)				{ mKey = k; }

	// overrides of BmRefObj
	const BString& RefName() const		{ return mKey; }

	//	message component definitions for status-msgs:
	static const char* const MSG_NUMCHILDREN  = 		"bm:count";
	static const char* const MSG_CHILDREN 		= 		"bm:chld";

protected:
	// native methods:
	bool AddSubItem( BmListModelItem* subItem);
	void RemoveSubItem( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmUpdFlags flags=UPD_ALL);

	BmListModelItem* mParent;
	BmListModel* mListModel;

private:
	BString mKey;
	BmModelItemMap mSubItemMap;

	static const BString nEmptyParentKey;

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
	
public:
	// c'tors & d'tor:
	BmListModel( const BString& name);
	virtual ~BmListModel();

	// native methods:
	BmListModelItemRef FindItemByKey( const BString& key);
	bool AddItemToList( BmListModelItem* item, BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	BmListModelItemRef RemoveItemFromList( const BString& key);
	virtual bool Store();
	virtual const BString SettingsFileName() = 0;
	static BMessage* Restore( const BString settingsFile);
	virtual void InitializeItems()		{	mInitCheck = B_OK; }
	virtual void InstantiateItems( BMessage* archive)		{ mInitCheck = B_OK; }
	virtual void Cleanup();

	// overrides of job-model base:
	bool StartJob();

	// overrides of Archivable base:
	status_t Archive( BMessage* archive, bool deep) const;

	//	message component definitions for status-msgs:
	static const char* const MSG_ITEMKEY 		=		"bm:ikey";
	static const char* const MSG_PARENTKEY 	= 		"bm:pkey";
	static const char* const MSG_MODELITEM 	= 		"bm:item";
	static const char* const MSG_UPD_FLAGS		= 		"bm:updflags";

	// getters:
	BmModelItemMap::const_iterator begin() const { return mModelItemMap.begin(); }
	BmModelItemMap::const_iterator end() const	{ return mModelItemMap.end(); }
	size_t size() const						{ return mModelItemMap.size(); }
	bool empty() const						{ return mModelItemMap.empty(); }
	status_t InitCheck() const				{ return mInitCheck; }


protected:

	// native methods:
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item, BmUpdFlags flags=UPD_ALL);

	status_t mInitCheck;

private:
	BmModelItemMap mModelItemMap;
};

#endif
