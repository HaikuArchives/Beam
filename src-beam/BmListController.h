/*
	BmListController.h
		$Id$
*/

#ifndef _BmListController_h
#define _BmListController_h

#include <vector>

#include "CLVEasyItem.h"
#include "ColumnListView.h"

#include "BmController.h"
#include "BmDataModel.h"

class BHandler;

#define BM_VIEW_ITEM_SELECTED				'bmCa'
#define BM_VIEW_ITEM_INVOKED				'bmCb'


class BmListViewController;

/*------------------------------------------------------------------------------*\
	BmListViewItem
		-	
\*------------------------------------------------------------------------------*/
class BmListViewItem : public CLVEasyItem
{
	typedef CLVEasyItem inherited;

	struct BmListColumn {
		const char* text;
		bool rightJustified;
	};

	class BmListViewItemInfo {
	public:
		bool isExpanded;
		
		BmListViewItemInfo();
		BmListViewItemInfo( BMessage* msg);
		virtual ~BmListViewItemInfo();
	private:
		//	message component definitions for status-msgs:
		static const char* const MSG_EXPANDED = 			"bm:expd";
	};

public:
	//
	BmListViewItem( BString& key, BmListModelItem* item,
						 bool hierarchical=false, uint32 level=0, 
						 bool superitem=false, bool expanded=false);
	~BmListViewItem();
	
	//	
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	//
	void SetTextCols( int16 firstTextCol, BmListColumn* columnVec, bool truncate=true);
	
	// getters:
	BString Key()								{ return mKey; }
	BmListModelItem* ModelItem()			{ return mModelItem; }

	//
	virtual void AddSubItemsToList( BmListViewController *view);

protected:
	BString mKey;
	BmListModelItem* mModelItem;
	int32 mFirstTextCol;
	BList* mSubItemList;
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
	BmListViewController( minimax minmax,BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true);
	~BmListViewController();

	// getters:
	CLVContainerView* ContainerView()	{ return inherited::fScrollView; }

	virtual void AttachModel( BmDataModel* model=NULL);
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
	virtual void AddModelItemsToList();
	
	virtual void MouseDown(BPoint point);

protected:
	virtual BmListModel* DataModel()		{ return dynamic_cast<BmListModel*>(BmController::DataModel()); }
	// archival of the controller's state-info:
	virtual status_t Archive(BMessage* archive, bool deep=true) const;
	virtual BString StateInfoBasename()				= 0;
	virtual void WriteStateInfo();
	virtual void ReadStateInfo();

	BList* mItemList;
	BMessage* mInitialStateInfo;
};


#endif
