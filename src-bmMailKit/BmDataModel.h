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

// message constants for BmDataModel (and subclasses), all msgs are sent to 
// the handler specified via each Controllers GetControllerHandler()-method
#define BM_JOB_DONE				'bmda'
							// the job has finished or was stopped
#define BM_JOB_UPDATE_STATE	'bmdb'
							// the job wants to update (one of) its state(s)
#define BM_LISTMODEL_ADD		'bmdc'
							// the listmodel has added a new item
#define BM_LISTMODEL_REMOVE	'bmdd'
							// the listmodel has removed an item
#define BM_LISTMODEL_UPDATE	'bmde'
							// the listmodel indicates a state-change of on of its items
#define BM_LISTMODEL_BUFFER	'bmdf'
							// the listmodel has buffered a group of messages

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
	//
	BmDataModel( const BString& name);
	virtual ~BmDataModel();

	//
	virtual void AddController( BmController* controller);
	virtual void RemoveController( BmController* controller);

	// getters:
	BString ModelName() const				{ return mModelName; }
	BLocker& ModelLocker() 					{ return mModelLocker; }

	//	message component definitions for status-msgs:
	static const char* const MSG_MODEL = 			"bm:model";

protected:
	//
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
	//
	BmJobModel( const BString& name);
	virtual ~BmJobModel();

	//
	static int32 ThreadStartFunc(  void*);
	virtual void StartJobInNewThread( bool deleteWhenDone);
	virtual void StartJobInThisThread();
	
	//
	virtual void PauseJob();
	virtual void ContinueJob();
	virtual void StopJob();

	//
	thread_id JobThreadID() const 		{ return mThreadID; }
	bool DeleteWhenDone() const			{ return mDeleteWhenDone; }
	bool IsJobRunning() const;
	bool IsJobCompleted() const			{ return mJobState == JOB_COMPLETED; }

	//	message component definitions for status-msgs:
	static const char* const MSG_COMPLETED = 		"bm:completed";
	static const char* const MSG_DOMAIN = 			"bm:domain";

protected:
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
	bool mDeleteWhenDone;
};

class BmListModelItem;
typedef map< BString, BmListModelItem*> BmModelItemMap;

/*------------------------------------------------------------------------------*\
	BmListModelItem
		-	base class for the items that will be part of a BmListModel
\*------------------------------------------------------------------------------*/
class BmListModelItem : public BArchivable {

public:

	BmListModelItem( BString key, BmListModelItem* parent=NULL);
	virtual ~BmListModelItem();

	//
	inline BmModelItemMap::const_iterator begin()	const { return mSubItemMap.begin(); }
	inline BmModelItemMap::const_iterator end()		const { return mSubItemMap.end(); }

	// getters:
	BString Key() const						{ return mKey; }
	BString ParentKey() const				{ return mParent ? mParent->Key() : BString("Empty"); }
	BmListModelItem* Parent() const		{ return mParent; }

	// setters:
	void Parent( BmListModelItem* parent)	{ mParent = parent; }

	//
	virtual void AddSubItem( BmListModelItem* subItem);
	//
	virtual BmListModelItem* FindItemByKey( BString& key);

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
	//
	BmListModel( const BString& name);
	virtual ~BmListModel();

	//
	inline BmModelItemMap::const_iterator begin()	const { return mModelItemMap.begin(); }
	inline BmModelItemMap::const_iterator end()		const { return mModelItemMap.end(); }

	void RemovalNoticed( BmController* controller);

	//	message component definitions for status-msgs:
	static const char* const MSG_ITEMKEY 		=		"bm:ikey";
	static const char* const MSG_PARENTKEY 	= 		"bm:pkey";
	static const char* const MSG_MODELITEM 	= 		"bm:item";
	static const char* const MSG_NUM_CHILDREN = 		"bm:count";
	static const char* const MSG_CHILDREN 		= 		"bm:children";

protected:
	//
	virtual void TellModelItemAdded( BmListModelItem* item);
	virtual void TellModelItemRemoved( BmListModelItem* item);
	virtual void TellModelItemUpdated( BmListModelItem* item);

	BmModelItemMap mModelItemMap;
	BmOpenReplySet mOpenReplySet;
};

#endif
