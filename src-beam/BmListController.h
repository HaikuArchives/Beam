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

class BmBusyView;
class BmCaption;
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
	BmCLVContainerView
		-	
\*------------------------------------------------------------------------------*/
class BmCLVContainerView : public CLVContainerView
{
	typedef CLVContainerView inherited;
public:
	BmCLVContainerView( minimax minmax, ColumnListView* target, uint32 resizingMode, 
							  uint32 flags, bool horizontal, bool vertical,
							  bool scroll_view_corner, border_style border, 
							  bool showCaption=false, bool showBusyView=false,
							  float captionWidth=-1);
	~BmCLVContainerView();
	
	// native methods:
	void SetBusy();
	void UnsetBusy();
	void PulseBusyView();

	// overrides of MView base:
	BRect layout( BRect);
	
	// setters:
	void SetCaptionText( const char* text);
	void SetCaptionWidth( float width) 	{ mCaptionWidth = width; }

private:
	BmCaption* mCaption;
	float mCaptionWidth;
	BmBusyView* mBusyView;

};

/*------------------------------------------------------------------------------*\
	BmListViewController
		-	
\*------------------------------------------------------------------------------*/
class BmListViewController : public ColumnListView, public BmJobController
{
	typedef ColumnListView inherited;
	typedef BmJobController inheritedController;

	//	message component definitions:
	static const char* const MSG_COLUMN_NO = "bm:colno";
	static const char* const MSG_COLUMN_POS = "bm:colps";

public:
	//c'tors and d'tor:
	BmListViewController( minimax minmax,BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true,
								 bool showCaption = false,
								 bool showBusyView = false);
	virtual ~BmListViewController();

	// native methods:
	virtual void AddModelItemsToList();
	virtual void AddModelItemToList( BMessage* msg);
	void RemoveModelItem( BMessage* msg);
	void UpdateModelItem( BMessage* msg);
	void UpdateModelState( BMessage* msg);
	void UpdateItem( BmListViewItem* item, BmUpdFlags flags);
	void UpdateCaption( const char* text=NULL);
	void ShowOrHideColumn( BMessage* msg);
	//
	virtual BmListViewItem* CreateListViewItem( BmListModelItem* item, 
															  BMessage* archive=NULL) 			= 0;
	//
	BMessage* GetArchiveForItemKey( BString, BMessage* msg=NULL);

	// overrides of controller base:
	void AttachModel( BmDataModel* model=NULL);
	void DetachModel();
	BHandler* GetControllerHandler() 	{ return this; }
	void StartJob( BmJobModel* model = NULL, bool startInNewThread=true);
	void JobIsDone( bool completed);

	// overrides of listview base:
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void ShowLabelViewMenu( BPoint pos);
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);
	BmCLVContainerView* ScrollView() 	{ return dynamic_cast<BmCLVContainerView*>(fScrollView); }

	// getters:
	CLVContainerView* ContainerView()	{ return inherited::fScrollView; }
	BMessage* InitialStateInfo()			{ return mInitialStateInfo; }

	// setters:
	void UseStateCache( bool b) 			{ mUseStateCache = b; }

protected:
	virtual BmListModel* DataModel()		{ return dynamic_cast<BmListModel*>(BmController::DataModel()); }
	// archival of the controller's state-info:
	virtual status_t Archive(BMessage* archive, bool deep=true) const;
	virtual BString StateInfoBasename()				= 0;
	virtual void WriteStateInfo();
	virtual void ReadStateInfo();
	virtual BMessage* DefaultLayout()	{ return NULL; }

	BList* mItemList;
	BMessage* mInitialStateInfo;
	bool mShowCaption;
	bool mShowBusyView;
	bool mUseStateCache;

};


#endif
