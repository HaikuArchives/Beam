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
	BString ModelName() 						{ return mDataModel ? mDataModel->ModelName() : "***NULL***"; }

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


class BmListViewItem;
class BmListViewController;
typedef map<const BString, BmListViewItem*> BmItemMap;

/*------------------------------------------------------------------------------*\
	BmListViewItem
		-	
\*------------------------------------------------------------------------------*/
typedef const char** BmTextVec;

class BmListViewItem : public CLVEasyItem
{
	typedef CLVEasyItem inherited;

public:
	//
	BmListViewItem( BString& key, BmListModelItem* item, BBitmap* icon, 
						 bool hierarchical=false, uint32 level=0, 
						 bool superitem=false, bool expanded=false);
	~BmListViewItem();
	
	//
	void SetTextCols( BmTextVec textVec);
	
	// getters:
	BString Key()								{ return mKey; }
	BmListModelItem* ModelItem()			{ return mModelItem; }
	
	//
	virtual const BString& GetSortKey( const BString& col)	= 0;
	
	//
	virtual void AddSubItemsToMap( BmListViewController *view);
	virtual void SortSubItemMap();
	virtual void ShowSortedSubItemMap( BmListViewController *view);

protected:
	BString mKey;
	BmListModelItem* mModelItem;
	uint32 mOffs;
	BmItemMap mSubItemMap;
	BmItemMap mSortedMap;
};

/*------------------------------------------------------------------------------*\
	BmListViewController
		-	
\*------------------------------------------------------------------------------*/
class BmListViewController : public ColumnListView, public BmJobController
{
	typedef ColumnListView inherited;
	typedef BmJobController inheritedController;

public:
	
	//Constructor and destructor
	BmListViewController( BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true);
	~BmListViewController();

	// getters:
	CLVContainerView* ContainerView()	{ return mContainerView; }

	virtual void DetachModel();

	//
	virtual BHandler* GetControllerHandler() { return this; }
	//
	virtual void MessageReceived( BMessage* msg);
	virtual void AddModelItem( BMessage* msg);
	virtual void RemoveModelItem( BMessage* msg);
	virtual void UpdateModelItem( BMessage* msg);
	virtual void UpdateModelState( BMessage* msg);
	virtual void JobIsDone( bool completed);
	//
	virtual BmListViewItem* CreateListViewItem( BmListModelItem* item, 
															  uint32 level=0) 			= 0;
	virtual BmListViewItem* TopHierarchyItem()	{ return NULL; }
	//
	virtual void AddModelItemsToMap();
	virtual void SortItemMap();
	virtual void ShowSortedItemMap();
	
	virtual void MouseDown(BPoint point);

protected:
	virtual BmListModel* DataModel()		{ 	return dynamic_cast<BmListModel*>(BmController::DataModel()); }

	CLVContainerView* mContainerView;
	BmItemMap mItemMap;
	BmItemMap mSortedMap;
};


#endif
