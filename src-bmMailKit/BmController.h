/*
	BmController.h
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


#ifndef _BmController_h
#define _BmController_h

#include <vector>

#include "BmDataModel.h"
#include "BmUtil.h"

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
	BmController( BmString name);
	virtual ~BmController();

	// native methods:
	virtual BHandler* GetControllerHandler() = 0;
	virtual void AttachModel( BmDataModel* model=NULL);
	virtual void DetachModel();
	virtual void ResetController()		{ }
	virtual bool IsMsgFromCurrentModel( BMessage* msg);

	// getters
	inline const char* ControllerName() const	{ return mControllerName.String(); }
	inline const BmString& ModelName() const	{ return mDataModel.Get() ? mDataModel->ModelName() : BM_DEFAULT_STRING; }
	inline BmString ModelNameNC() const { return ModelName(); }
	virtual BmDataModel* DataModel()		{ return mDataModel.Get(); }

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
	BmController operator=( const BmController&);
#endif
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
	virtual void JobIsDone( bool completed) = 0;

protected:
	virtual BmJobModel* DataModel()		{ 	return dynamic_cast<BmJobModel*>(inherited::DataModel()); }

	// Hide copy-constructor and assignment:
	BmJobController( const BmJobController&);
#ifndef __POWERPC__
	BmJobController operator=( const BmJobController&);
#endif
};


#endif
