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

typedef uint32 BmUpdFlags;

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

	friend BmListViewController;

protected: 
	// archival-fieldnames:
	static const char* const MSG_EXPANDED = 	"bm:expnd";
	static const char* const MSG_CHILDNAMES = 	"bm:chldnm";
	static const char* const MSG_CHILDREN = 		"bm:chldrn";

	// flags indicating which parts are to be updated
	static const BmUpdFlags UPD_EXPANDER 	= 1<<0;
	static const BmUpdFlags UPD_ALL 			= 0xFFFFFFFF;

public:
	//
	BmListViewItem( BString& key, BmListModelItem* item, bool hierarchical=false, 
						 BMessage* archive=NULL);
	virtual ~BmListViewItem();
	
	// native methods:
	void SetTextCols( int16 firstTextCol, BmListColumn* columnVec, bool truncate=true);
	void AddSubItemsToList( BmListViewController *view);
	virtual void UpdateView( BmUpdFlags flags);

	//	overrides from listitem-baseclass:
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const BString Key() const				{ return mKey; }
	BmListModelItem* ModelItem() 			{ return mModelItem; }
	
protected:
	BString mKey;
	BmListModelItem* mModelItem;
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
	//
	BmListViewController( minimax minmax,BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true);
	virtual ~BmListViewController();

	// native methods:
	void AddModelItemsToList();
	void AddModelItem( BMessage* msg);
	void RemoveModelItem( BMessage* msg);
	void UpdateModelItem( BMessage* msg);
	void UpdateModelState( BMessage* msg);
	void UpdateItem( BmListViewItem* item, BmUpdFlags flags);
	//
	virtual BmListViewItem* CreateListViewItem( BmListModelItem* item, 
															  BMessage* archive=NULL) 			= 0;
	//
	BMessage* GetArchiveForItemKey( BString);

	// overrides of controller-baseclass:
	void AttachModel( BmDataModel* model=NULL);
	void DetachModel();
	BHandler* GetControllerHandler() 	{ return this; }
	void JobIsDone( bool completed);

	// overrides of listview-baseclass:
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);

	// getters:
	CLVContainerView* ContainerView()	{ return inherited::fScrollView; }
	BMessage* InitialStateInfo()			{ return mInitialStateInfo; }

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
