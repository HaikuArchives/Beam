/*
	BmDataModel.h
		$Id$
*/

#ifndef _BmDataModel_h
#define _BmDataModel_h

#include <set>

#include <Locker.h>
#include <Looper.h>
#include <String.h>

class BmController;

// message constants for BmDataModel (and subclasses), all msgs are sent to 
// the handler specified via each Controllers GetControllerHandler()-method
#define BM_JOB_DONE				'bmja'
							// the job has finished or was stopped
#define BM_JOB_UPDATE_STATE	'bmjb'
							// the job wants to update (one of) its state(s)

/*------------------------------------------------------------------------------*\
	BmDataModel
		-	an interface for informing other objects (e.g. MVC-like 
			controllers) to be informed about state-changes of this
			object.
		-	contains functionality to add/remove controllers
\*------------------------------------------------------------------------------*/
class BmDataModel {
	typedef set<BmController*> BmControllerSet;

public:
	//
	BmDataModel( const BString& name);
	virtual ~BmDataModel();

	//
	virtual void AddController( BmController* controller);
	virtual void RemoveController( BmController* controller);

	// getters:
	BString ModelName() const				{ return mModelName; }

protected:
	//
	virtual bool ShouldContinue();
	virtual void TellControllers( BMessage* msg);
	virtual void WaitForAllControllers();

	BLocker mModelLocker;
	BmControllerSet mControllerSet;

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
	virtual void StartJobInThread();
	
	//
	virtual void PauseJob();
	virtual void ContinueJob();
	virtual void StopJob();

	//
	thread_id JobThreadID() const 		{ return mThreadID; }
	bool IsJobRunning() const;

	//	message component definitions for status-msgs:
	static const char* const MSG_COMPLETED = 		"bm:completed";
	static const char* const MSG_DOMAIN = 			"bm:domain";

protected:
	virtual void StartJob() 				= 0;
	virtual bool ShouldContinue();
	virtual void TellJobIsDone( bool completed=true);

	enum BmJobState { JOB_INITIALIZED, JOB_RUNNING, JOB_PAUSED, JOB_STOPPED, JOB_DYING };
	BmJobState mJobState;

	BmJobState JobState() const 			{ return mJobState; }

private:
	virtual void doStartJob();

	thread_id mThreadID;
};

#endif
