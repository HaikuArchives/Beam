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
	BString RefName() const					{ return mModelName; }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL = 			"bm:model";

	static BmRefManager<BmDataModel> RefManager;

protected:
	// native methods:
	virtual bool HasControllers();
	virtual bool ShouldContinue();
	virtual void TellControllers( BMessage* msg);
	virtual void WaitForAllControllers();
	inline void Freeze() 					{ mFrozenCount++; }
	inline void Thaw()						{ mFrozenCount--; }
	inline bool Frozen() 					{ return mFrozenCount!=0; }

	BLocker mModelLocker;
	BmControllerSet mControllerSet;
	uint8 mFrozenCount;

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
	virtual void StartJobInNewThread();
	virtual void StartJobInThisThread();
	virtual void PauseJob();
	virtual void ContinueJob();
	virtual void StopJob();
	thread_id JobThreadID() const 		{ return mThreadID; }
	bool IsJobRunning() const;
	bool IsJobCompleted() const			{ return mJobState == JOB_COMPLETED; }

	//	message component definitions for status-msgs:
	static const char* const MSG_COMPLETED = 		"bm:completed";
	static const char* const MSG_DOMAIN = 			"bm:domain";

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
};

// flags indicating which parts are to be updated:
typedef uint32 BmUpdFlags;
const BmUpdFlags UPD_EXPANDER 	= 1<<0;
const BmUpdFlags UPD_ALL 			= 0xFFFFFFFF;

class BmListModelItem;
typedef map< BString, BmRef<BmListModelItem> > BmModelItemMap;

/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class BmListModelItem : public BmRefObj, public BArchivable {

public:
	// c'tors & d'tor:
	BmListModelItem( BString key, BmListModelItem* parent=NULL);
	virtual ~BmListModelItem();

	// native methods:
	void AddSubItem( BmListModelItem* subItem);
	void RemoveSubItem( BmListModelItem* item);
	BmListModelItem* FindItemByKey( BString& key);

	// getters:
	BmModelItemMap::const_iterator begin() const	{ return mSubItemMap.begin(); }
	BmModelItemMap::const_iterator end() const	{ return mSubItemMap.end(); }
	size_t size() const						{ return mSubItemMap.size(); }
	bool empty() const						{ return mSubItemMap.empty(); }
	BString Key() const						{ return mKey; }
	BString ParentKey() const				{ return mParent ? mParent->Key() : BString("Empty"); }
	BmListModelItem* Parent() const		{ return mParent; }

	// setters:
	void Parent( BmListModelItem* parent)	{ mParent = parent; }

	// overrides of BmRefObj
	BString RefName() const					{ return mKey; }

	//	message component definitions for status-msgs:
	static const char* const MSG_NUMCHILDREN  = 		"bm:count";
	static const char* const MSG_CHILDREN 		= 		"bm:chld";

	static BmRefManager<BmListModelItem> RefManager;

protected:
	BString mKey;
	BmListModelItem* mParent;
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
	
public:
	// c'tors & d'tor:
	BmListModel( const BString& name);
	virtual ~BmListModel();

	// native methods:
	BmListModelItem* FindItemByKey( BString& key);
	void AddItemToList( BmListModelItem* item, BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	void RemoveItemFromList( BString key);
	virtual bool Store();
	virtual const BString SettingsFileName() = 0;
	static BMessage* Restore( const BString settingsFile);
	virtual void InitializeItems()		{	mInitCheck = B_OK; }
	virtual void InstantiateItems( BMessage* archive)		{ mInitCheck = B_OK; }

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
	status_t InitCheck()						{ return mInitCheck; }


protected:

	// native methods:
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item, BmUpdFlags flags=UPD_ALL);

	BmModelItemMap mModelItemMap;
	status_t mInitCheck;
};

#endif
