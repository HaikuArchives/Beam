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

class BmController;

/*------------------------------------------------------------------------------*\
	BmDataModel
		-	an interface for informing other objects (e.g. MVC-like 
			controllers) to be informed about state-changes of this
			object.
		-	contains functionality to add/remove controllers
\*------------------------------------------------------------------------------*/
class BmDataModel {
	typedef map< BmController*, uint32> BmControllerMap;

public:
	// c'tors & d'tor:
	BmDataModel( const BString& name);
	virtual ~BmDataModel();

	// native methods:
	virtual void AddController( BmController* controller);
	virtual void RemoveController( BmController* controller);

	// getters:
	BString ModelName() const				{ return mModelName; }
	BLocker& ModelLocker() 					{ return mModelLocker; }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL = 			"bm:model";

protected:
	// native methods:
	virtual bool HasControllers();
	virtual bool ShouldContinue();
	virtual void TellControllers( BMessage* msg, bool bumpStateVal=true);
	virtual void WaitForAllControllers();

	BLocker mModelLocker;
	BmControllerMap mControllerMap;
	uint32 mStateVal;

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
	virtual void StartJob() 				{}
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

class BmListModelItem;
typedef map< BString, BmListModelItem*> BmModelItemMap;

/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class BmListModelItem : public BArchivable {

public:
	// c'tors & d'tor:
	BmListModelItem( BString key, BmListModelItem* parent=NULL);
	virtual ~BmListModelItem();

	// native methods:
	virtual void AddSubItem( BmListModelItem* subItem);
	virtual BmListModelItem* FindItemByKey( BString& key);

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
class BmListModel : public BmJobModel {
	typedef BmJobModel inherited;
	
	typedef set< BmController*> BmOpenReplySet;

public:
	// c'tors & d'tor:
	BmListModel( const BString& name);
	virtual ~BmListModel();

	// native methods:
	virtual void RemovalNoticed( BmController* controller);

	//	message component definitions for status-msgs:
	static const char* const MSG_ITEMKEY 		=		"bm:ikey";
	static const char* const MSG_PARENTKEY 	= 		"bm:pkey";
	static const char* const MSG_MODELITEM 	= 		"bm:item";
	static const char* const MSG_NUM_CHILDREN = 		"bm:count";
	static const char* const MSG_CHILDREN 		= 		"bm:children";

	// getters:
	BmModelItemMap::const_iterator begin() const { return mModelItemMap.begin(); }
	BmModelItemMap::const_iterator end() const	{ return mModelItemMap.end(); }
	size_t size() const						{ return mModelItemMap.size(); }
	bool empty() const						{ return mModelItemMap.empty(); }

protected:
	// native methods:
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item);

	BmModelItemMap mModelItemMap;
	BmOpenReplySet mOpenReplySet;
};



/*------------------------------------------------------------------------------*\
	BmDataModelManager
		-	manages all instances of DataModels (esp their deletion)
\*------------------------------------------------------------------------------*/
class BmDataModelManager {
	typedef map< BmDataModel*, int32> BmRefMap;

public:
	// creator-func, c'tors & d'tor:
	static BmDataModelManager* CreateInstance();
	BmDataModelManager();
	~BmDataModelManager();
	
	// native methods:
	void AddRef( BmDataModel* model);
	void RemoveRef( BmDataModel* model);

	static BmDataModelManager* theInstance;

private:
	BmRefMap mRefMap;
	BLocker mLocker;
};

#define TheModelManager BmDataModelManager::theInstance


#endif
