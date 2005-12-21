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

/********************************************************************************\
	BmController
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	BmController()
		-	standard contructor
\*------------------------------------------------------------------------------*/
BmController::BmController( BmString name)
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
	BM_LOG2( BM_LogModelController, 
				BmString("Controller <") << ControllerName() 
					<< "> has been destructed.");
}

/*------------------------------------------------------------------------------*\
	AttachModel( model)
		-	tell a model that we are interested in its state
		-	if the "model"-argument is specified, it will become the new model
			of interest.
		-	safe to call with a NULL-model
		-	if called with current model, nothing will happen
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
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> attaches to model " << ModelName());
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
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> detaches from model " << ModelName());
		mDataModel->RemoveController( this);
		mDataModel = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	IsMsgFromCurrentModel( msg)
		-	determines if the given message came from our current model
\*------------------------------------------------------------------------------*/
bool BmController::IsMsgFromCurrentModel( BMessage* msg) {
	BmString msgModelName = FindMsgString( msg, BmDataModel::MSG_MODEL);
	if (msgModelName != ModelName()) {
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> drops msg from model <" << msgModelName
						<< "> which is not the current one (<" 
						<< ModelName() << ">)");
		return false;
	} else
		return true;
}

/*------------------------------------------------------------------------------*\
	MsgNeedsAck( msg)
		-	checks whether given message needs to be acknowledged
		-	ACKs are necessary for item-removal-messages (for instance)
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
		-	sets the current model
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
BmJobController::BmJobController( BmString name)
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
	StartJob( model, startInNewThread, jobSpecifier)
		-	starts the given jobmodel (or the current, if model==NULL)
		-	param startInNewThread decides whether or not to spawn a new thread
			for this job
		-	jobSpecifier can specify special tasks for the model (some models may
			support more than one kind of job). This defaults to BM_DEFAULT_JOB
\*------------------------------------------------------------------------------*/
void BmJobController::StartJob( BmJobModel* model, bool startInNewThread,
										  int32 jobSpecifier) {
	if (model)
		AttachModel( model);
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel) {
		if (jobModel->IsJobCompleted() 
		&& jobSpecifier == BmJobModel::BM_DEFAULT_JOB) {
			BM_LOG2( BM_LogModelController, 
						BmString("Controller <") << ControllerName() 
							<< "> avoids already completed job " << ModelName());
			JobIsDone(true);
			return;
		}
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> starts job " << ModelName());
		if (startInNewThread)
			jobModel->StartJobInNewThread( jobSpecifier);
		else
			jobModel->StartJobInThisThread( jobSpecifier);
	}
}

/*------------------------------------------------------------------------------*\
	PauseJob( msg)
		-	hook-method that is called when the user wants this Job to pause
		-	parameter msg may contain any further attributes needed for update
		-	this default implementation simply tells its model to pause
\*------------------------------------------------------------------------------*/
void BmJobController::PauseJob( BMessage*) {
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel) {
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> pauses job " << ModelName());
		jobModel->PauseJob();
	}
}

/*------------------------------------------------------------------------------*\
	ContinueJob( msg)
		-	hook-method that is called when the user wants this Job to continue
		-	parameter msg may contain any further attributes
		-	this default implementation simply tells its model to continue
\*------------------------------------------------------------------------------*/
void BmJobController::ContinueJob( BMessage*) {
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel) {
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> continues job " << ModelName());
		jobModel->ContinueJob();
	}
}

/*------------------------------------------------------------------------------*\
	StopJob()
		-	this method is called when the user wants to stop a job
		-	this default implementation simply detaches from its model
\*------------------------------------------------------------------------------*/
void BmJobController::StopJob() {
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel && !jobModel->IsJobCompleted()) {
		BM_LOG2( BM_LogModelController, 
					BmString("Controller <") << ControllerName() 
						<< "> stops job " << ModelName());
		jobModel->StopJob();
	}
}

/*------------------------------------------------------------------------------*\
	IsJobRunning()
		-	check whether or not the current job is still running
\*------------------------------------------------------------------------------*/
bool BmJobController::IsJobRunning() {
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel)
		return jobModel->IsJobRunning();
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	CurrentJobSpecifier()
		-	returns the exact kind of the current job (usually BM_DEFAULT_JOB)
\*------------------------------------------------------------------------------*/
int32 BmJobController::CurrentJobSpecifier() {
	BmJobModel* jobModel = dynamic_cast<BmJobModel*>( DataModel().Get());
	if (jobModel)
		return jobModel->CurrentJobSpecifier();
	else
		return BmJobModel::BM_DEFAULT_JOB;
}
