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
		-	
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
	const BString ModelName() const		{ return mDataModel ? mDataModel->ModelName() : "***NULL***"; }
	virtual BmDataModel* DataModel()		{ return mDataModel; }

	// setters
	void DataModel( BmDataModel* model)	{ mDataModel = model; }

protected:
	//
	virtual bool IsMsgFromCurrentModel( BMessage* msg);

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

	// native methods:
	virtual void StartJob( BmJobModel* model = NULL, bool startInNewThread=true, bool deleteWhenDone=true);
	virtual void PauseJob( BMessage* msg);
	virtual void ContinueJob( BMessage* msg);
	virtual void StopJob();
	virtual bool IsJobRunning();

protected:
	virtual BmJobModel* DataModel()		{ 	return dynamic_cast<BmJobModel*>(inherited::DataModel()); }

};


#endif
