/*
	BmController.cpp
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

#include "BmBasics.h"
#include "BmController.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
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
	BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> has been destructed.");
}

/*------------------------------------------------------------------------------*\
	AttachModel( model)
		-	tell a model that we are interested in its state
		-	if the "model"-argument is specified, it will become the new model
			of interest.
		-	safe to call with a NULL-model
\*------------------------------------------------------------------------------*/
void BmController::AttachModel( BmDataModel* model) {
	if (mDataModel.Get() == model)
		return;
	if (mDataModel) {
		// detach current model, since we shall control a new one:
		DetachModel();
	}
	DataModel( model);
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
	if (msgModelName != ModelName()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> drops msg from model <"<<msgModelName<<"> which is not the current one (<"<<ModelName()<<">)");
		return false;
	} else
		return true;
}

/*------------------------------------------------------------------------------*\
	MsgNeedsAck( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmController::MsgNeedsAck( BMessage* msg) {
	bool needsAck;
	if (!msg || msg->FindBool( BmDataModel::MSG_NEEDS_ACK, &needsAck) != B_OK)
		return false;
	else
		return needsAck;
}

/*------------------------------------------------------------------------------*\
	DataModel( model)
		-	
\*------------------------------------------------------------------------------*/
void BmController::DataModel( BmDataModel* model) {
	mDataModel = model;
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
										  int32 jobSpecifier) {
	AttachModel( model);
	if (DataModel()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> starts job " << ModelName());
		if (startInNewThread)
			DataModel()->StartJobInNewThread( jobSpecifier);
		else
			DataModel()->StartJobInThisThread( jobSpecifier);
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
	if (job && !job->IsJobCompleted()) {
		BM_LOG2( BM_LogModelController, BString("Controller <") << ControllerName() << "> stops job " << ModelName());
		job->StopJob();
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

/*------------------------------------------------------------------------------*\
	CurrentJobSpecifier()
		-	
\*------------------------------------------------------------------------------*/
int32 BmJobController::CurrentJobSpecifier() {
	BmJobModel* model = DataModel();
	if (model)
		return model->CurrentJobSpecifier();
	else
		return BmJobModel::BM_DEFAULT_JOB;
}
