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

#define BM_VIEW_ITEM_SELECTED				'bmCa'
#define BM_VIEW_ITEM_INVOKED				'bmCb'

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
	const BString ModelName() const		{ return mDataModel ? mDataModel->ModelName() : "***NULL***"; }

	virtual void AttachModel( BmDataModel* model=NULL);
	virtual void DetachModel();
	virtual BmDataModel* DataModel()		{ return mDataModel; }
	virtual void ResetController()		{ }

protected:
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

	virtual void StartJob( BmJobModel* model = NULL, bool startInNewThread=true, bool deleteWhenDone=true);
	virtual void PauseJob( BMessage* msg);
	virtual void ContinueJob( BMessage* msg);
	virtual void StopJob();
	virtual bool IsJobRunning();

protected:
	virtual BmJobModel* DataModel()		{ 	return dynamic_cast<BmJobModel*>(inherited::DataModel()); }

};


#endif
