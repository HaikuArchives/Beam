/*
	BmController.cpp
		$Id$
*/

#include <Autolock.h>

#include "BmApp.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

//#include <Profile.h>

/********************************************************************************\
	BmController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmController::BmController( BString name)
	:	mDataModel( NULL)
	,	mControllerName( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmController()
		-	destructor that ensures a detach from the model (if any)
\*------------------------------------------------------------------------------*/
BmController::~BmController() {
	DetachModel();
}

/*------------------------------------------------------------------------------*\
	AttachModel( model)
		-	tell a model that we are interested in its state
		-	if the "model"-argument is specified, it will become the new model
			of interest.
		-	safe to call with a NULL-model
\*------------------------------------------------------------------------------*/
void BmController::AttachModel( BmDataModel* model) {
	if (model) {
		if (mDataModel) {
			// detach current model, since we shall control a new one:
			DetachModel();
		}
		DataModel( model);
	}
	if (mDataModel) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> attaches to model " << ModelName());
		mDataModel->AddController( this);
	}
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	tell a model that we are no longer interested
		-	safe to call if there is no attached model
\*------------------------------------------------------------------------------*/
void BmController::DetachModel() {
	if (mDataModel) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> detaches from model " << ModelName());
		mDataModel->RemoveController( this);
		mDataModel = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	IsMsgFromCurrentModel( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmController::IsMsgFromCurrentModel( BMessage* msg) {
	BString msgModelName = FindMsgString( msg, BmDataModel::MSG_MODEL);
	return msgModelName == ModelName();
}



/********************************************************************************\
	BmJobController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmJobController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmJobController::BmJobController( BString name)
	:	BmController( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmJobController()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmJobController::~BmJobController() {
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmJobController::StartJob( BmJobModel* model, bool startInNewThread, 
										  bool deleteWhenDone) {
	if (model) {
		AttachModel( model);
	}
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> starts job " << ModelName());
		if (startInNewThread)
			DataModel()->StartJobInNewThread( deleteWhenDone);
		else
			DataModel()->StartJobInThisThread();
	}
}

/*------------------------------------------------------------------------------*\
	PauseJob( msg)
		-	hook-method that is called when the user wants this Job to pause
		-	parameter msg may contain any further attributes needed for update
		-	this default implementation simply tells its model to pause
\*------------------------------------------------------------------------------*/
void BmJobController::PauseJob( BMessage* msg) {
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> pauses job " << ModelName());
		DataModel()->PauseJob();
	}
}

/*------------------------------------------------------------------------------*\
	ContinueJob( msg)
		-	hook-method that is called when the user wants this Job to continue
		-	parameter msg may contain any further attributes
		-	this default implementation simply tells its model to continue
\*------------------------------------------------------------------------------*/
void BmJobController::ContinueJob( BMessage* msg) {
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> continues job " << ModelName());
		DataModel()->ContinueJob();
	}
}

/*------------------------------------------------------------------------------*\
	StopJob()
		-	this method is called when the user wants to stop a job
		-	this default implementation simply detaches from its model
\*------------------------------------------------------------------------------*/
void BmJobController::StopJob() {
	BmJobModel* job = DataModel();
	if (job) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> stops job " << ModelName());
		job->StopJob();
		DetachModel();
	}
}

/*------------------------------------------------------------------------------*\
	IsJobRunning()
		-	
\*------------------------------------------------------------------------------*/
bool BmJobController::IsJobRunning() {
	BmJobModel* model = DataModel();
	if (model) {
		return model->IsJobRunning();
	} else {
		return false;
	}
}

