/*
	BmController.h
		$Id$
*/

#ifndef _BmController_h
#define _BmController_h

#include <vector>

#include "CLVEasyItem.h"
#include "ColumnListView.h"

#include "BmDataModel.h"

class BHandler;

/*------------------------------------------------------------------------------*\
	BmController
		-	a base class for a controller-part within a MVC (model-view-controller)-like
			data/view abstraction.
		-	contains functionality to set/remove the datamodel to be controlled.
		-	N.B: a Controller accesses it's DataModel directly through a pointer, 
			in contrast to the DataModel sending messages to the Controller. 
			That's because a DataModel can function perfectly happy without any 
			Controller (just doing what it is supposed to do: handle data) and emits
			messages only if any controller is interested in them.
			The Controller on the other hand sometimes requires immediate access to
			the DataModel (for instance when it needs to stop the DataModel).
\*------------------------------------------------------------------------------*/
class BmController {

public:
	//
	BmController( BString name);
	virtual ~BmController();

	// native methods:
	virtual BHandler* GetControllerHandler() = 0;
	virtual void AttachModel( BmDataModel* model=NULL);
	virtual void DetachModel();
	virtual void ResetController()		{ }

	// getters
	const char* ControllerName() const	{ return mControllerName.String(); }
	const BString ModelName() const		{ return mDataModel.Get() ? mDataModel->ModelName() : "***NULL***"; }
	virtual BmDataModel* DataModel()		{ return mDataModel.Get(); }

protected:
	//
	virtual bool IsMsgFromCurrentModel( BMessage* msg);

private:
	BmRef< BmDataModel> mDataModel;
	BString mControllerName;

	// setters
	void DataModel( BmDataModel* model);

};


/*------------------------------------------------------------------------------*\
	BmJobController
		-	a specialized controller that implements an abstract job(thread)-control.
		-	jobs can be started/stopped and have their status checked.
\*------------------------------------------------------------------------------*/
class BmJobController : public BmController {
	typedef BmController inherited;

public:
	//
	BmJobController( BString name);
	virtual ~BmJobController();

	// native methods:
	virtual void StartJob( BmJobModel* model=NULL, bool startInNewThread=true,
								  int32 jobSpecifier=BmJobModel::BM_DEFAULT_JOB);
	virtual void PauseJob( BMessage* msg);
	virtual void ContinueJob( BMessage* msg);
	virtual void StopJob();
	virtual bool IsJobRunning();
	int32 CurrentJobSpecifier();
	virtual void JobIsDone( bool completed) = 0;

protected:
	virtual BmJobModel* DataModel()		{ 	return dynamic_cast<BmJobModel*>(inherited::DataModel()); }

};


#endif
