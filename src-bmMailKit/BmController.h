/*
	BmController.h
		$Id$
*/

#ifndef _BmController_h
#define _BmController_h

class BHandler;

#include "BmDataModel.h"

/*------------------------------------------------------------------------------*\
	BmController
		-	
\*------------------------------------------------------------------------------*/
class BmController {

public:
	//
	BmController( BString name);
	virtual ~BmController();

	//
	virtual BHandler* GetControllerHandler() = 0;

	// setters
	void DataModel( BmDataModel* model)	{ mDataModel = model; }
	// getters
	const char* ControllerName() const	{ return mControllerName.String(); }
	BString ModelName() 						{ return mDataModel ? mDataModel->ModelName() : "***NULL***"; }

	virtual void AttachModel( BmDataModel* model=NULL);
	virtual void DetachModel();
	virtual BmDataModel* DataModel()		{ return mDataModel; }
	virtual void ResetController()		{ }

private:
	BmDataModel* mDataModel;
	BString mControllerName;

};


/*------------------------------------------------------------------------------*\
	BmJobController
		-	
\*------------------------------------------------------------------------------*/
class BmJobController : public BmController {
	typedef BmController inherited;

public:
	//
	BmJobController( BString name);
	virtual ~BmJobController();

	virtual void StartJob( BmJobModel* model = NULL);
	virtual void PauseJob( BMessage* msg);
	virtual void ContinueJob( BMessage* msg);
	virtual void StopJob();
	virtual bool IsJobRunning();

protected:
	virtual BmJobModel* DataModel()		{ 	return dynamic_cast<BmJobModel*>(inherited::DataModel()); }
};


#endif
