/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmController_h
#define _BmController_h

#include "BmMailKit.h"

#include "BmDataModel.h"
#include "BmUtil.h"

class BHandler;

/*------------------------------------------------------------------------------*\
	BmController
		-	a base class for a controller-part within a 
			MVC(model-view-controller)-like data/view abstraction.
		-	contains functionality to set/remove the datamodel to be controlled.
		-	N.B: a Controller accesses it's DataModel directly through a pointer, 
			in contrast to the DataModel sending messages to the Controller. 
			That's because a DataModel can function perfectly happy without any 
			Controller (just doing what it is supposed to do: handle data) and 
			emits messages only if any controller is interested in them.
			The Controller on the other hand sometimes requires immediate access
			to the DataModel (for instance when it needs to stop the DataModel).
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmController {

public:
	//
	BmController( BmString name);
	virtual ~BmController();

	// native methods:
	virtual BHandler* GetControllerHandler() = 0;
	virtual void AttachModel( BmDataModel* model=NULL);
	virtual void DetachModel();
	virtual void ResetController()		{ }
	virtual bool IsMsgFromCurrentModel( BMessage* msg);

	// getters
	inline const char* ControllerName() const	
													{ return mControllerName.String(); }
	inline const BmString& ModelName() const	
													{ return mDataModel.Get() 
																	? mDataModel->ModelName() 
																	: BM_DEFAULT_STRING; }
	inline BmString ModelNameNC() const { return ModelName(); }
	virtual const BmRef<BmDataModel>& DataModel()
													{ return mDataModel; }

protected:
	//
	virtual bool MsgNeedsAck( BMessage* msg);

private:
	BmRef< BmDataModel> mDataModel;
	BmString mControllerName;

	// setters
	void DataModel( BmDataModel* model);

	// Hide copy-constructor and assignment:
	BmController( const BmController&);
#ifndef __POWERPC__
	BmController& operator=( const BmController&);
#endif
};


/*------------------------------------------------------------------------------*\
	BmJobController
		-	a specialized controller that implements an abstract 
			job(thread)-control.
		-	jobs can be started/stopped and have their status checked.
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmJobController : public BmController {
	typedef BmController inherited;

public:
	//
	BmJobController( BmString name);
	virtual ~BmJobController();

	// native methods:
	virtual void StartJob( BmJobModel* model=NULL, bool startInNewThread=true,
								  int32 jobSpecifier=BmJobModel::BM_DEFAULT_JOB);
	virtual void PauseJob( BMessage* msg);
	virtual void ContinueJob( BMessage* msg);
	virtual void StopJob();
	virtual bool IsJobRunning();
	int32 CurrentJobSpecifier();

protected:

	virtual void JobIsDone( bool completed) = 0;

	// Hide copy-constructor and assignment:
	BmJobController( const BmJobController&);
#ifndef __POWERPC__
	BmJobController& operator=( const BmJobController&);
#endif
};


#endif
