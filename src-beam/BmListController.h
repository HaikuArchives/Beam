/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmListController_h
#define _BmListController_h

#include <map>

#include "CLVEasyItem.h"
#include "ColumnListView.h"

#include "BmController.h"
#include "BmDataModel.h"

using std::map;

class BmBusyView;
class BmCaption;
class BmListViewController;

/*------------------------------------------------------------------------------*\
	types of messages handled by a listview-controller:
\*------------------------------------------------------------------------------*/
enum {
	BM_LISTVIEW_SHOW_COLUMN		= 'bmca',
							// the user has chosen to show a column
	BM_LISTVIEW_HIDE_COLUMN		= 'bmcb',
							// the user has chosen to hide a column
	BM_NTFY_LISTCONTROLLER_MODIFIED = 'bmcc',
							// item has been added/removed
	BM_EXPAND_OR_COLLAPSE		= 'bmcd',
							// time to expand/collapse an item automatically
	BM_PULSED_SCROLL				= 'bmce'
							// time to scroll listview upwards/downwards
};

/*------------------------------------------------------------------------------*\
	BmListViewItem
		-	
\*------------------------------------------------------------------------------*/
class BmListViewItem : public CLVEasyItem
{
	typedef CLVEasyItem inherited;
	
	friend class BmListViewController;

protected: 
	// archival-fieldnames:
	static const char* const MSG_EXPANDED;
	static const char* const MSG_CHILDNAMES;
	static const char* const MSG_CHILDREN;

public:
	//
	BmListViewItem( ColumnListView* lv, BmListModelItem* item, 
						 bool hierarchical=false, 
						 BMessage* archive=NULL);
	virtual ~BmListViewItem();
	
	// native methods:
	void SetTextCols( int16 firstTextCol, const char** content);
	virtual void UpdateView( BmUpdFlags flags, bool redraw = true,
									 uint32 updColBitmap = 0);

	//	overrides from listitem-baseclass:
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	inline const BmString& Key() const	{ return mModelItem->Key(); }
	virtual BmListModelItem* ModelItem() const	
													{ return mModelItem.Get(); }
	
protected:
	BmRef<BmListModelItem> mModelItem;
	
	// Hide copy-constructor and assignment:
	BmListViewItem( const BmListViewItem&);
	BmListViewItem operator=( const BmListViewItem&);
};

class BMenu;
class BMessageRunner;

/*------------------------------------------------------------------------------*\
	BmViewItemManager
		-	manages the relations between listmodel-items and view items
\*------------------------------------------------------------------------------*/
class BmViewItemManager
{
	typedef map<const BmListModelItem*, BmListViewItem*> BmViewModelMap;

public:
	BmViewItemManager();
	virtual ~BmViewItemManager();
	
	virtual void Add(const BmListModelItem* modelItem, BmListViewItem* viewItem);
	virtual BmListViewItem* Remove(const BmListModelItem* modelItem);

	virtual void MakeEmpty();
	
	virtual BmListViewItem* FindViewItemFor(
		const BmListModelItem* modelItem) const;
		
protected:
	BmViewModelMap mViewModelMap;
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
	static const char* const MSG_COLUMN_NO;
	static const char* const MSG_COLUMN_POS;

public:
	//c'tors and d'tor:
	BmListViewController( BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true,
								 BmViewItemManager* viewItemManager = NULL);
	virtual ~BmListViewController();

	// native methods:
	virtual void WriteStateInfo();
	virtual void ReadStateInfo();

	// overrides of controller base:
	void AttachModel( BmDataModel* model=NULL);
	void DetachModel();
	BHandler* GetControllerHandler() 	{ return this; }
	void StartJob( BmJobModel* model = NULL, bool startInNewThread=true,
						int32 jobSpecifier=BmJobModel::BM_DEFAULT_JOB);
	status_t Archive(BMessage* archive, bool deep=true) const;

	// overrides of listview base:
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void ShowLabelViewMenu( BPoint pos);
	void AttachedToWindow();
	void MakeEmpty();
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *msg);

	// overrides of listitem-manager base
	BmListViewItem* FindViewItemFor( BmListModelItem* modelItem) const;

	// getters:
	inline BMessage* InitialStateInfo()			{ return mInitialStateInfo; }
	virtual const char* ItemNameForCaption()	{ return "item"; }

	// setters:
	void UseStateCache( bool b) 			{ mUseStateCache = b; }
	void DragBetweenItems( bool b) 		{ mDragBetweenItems = b; }

protected:
	// native methods:
	virtual void AddAllModelItems();
	virtual BmListViewItem* AddModelItem( BmListModelItem* item);
	virtual void RemoveModelItem( BmListModelItem* item);
	virtual BmListViewItem* UpdateModelItem( BmListModelItem* item, BmUpdFlags updFlags);
	virtual void UpdateModelState( BMessage* msg);
	virtual void UpdateItem( BmListViewItem* item, BmUpdFlags flags);
	virtual void UpdateCaption( const char* text=NULL);
	virtual void ItemInvoked( int32 index);
	virtual bool AcceptsDropOf( const BMessage*)	{ return false; }
	virtual void HandleDrop( BMessage* msg);
	virtual BBitmap* CreateDragImage(const vector<int>& cols, int32 maxLines=10);
	void HighlightItemAt( const BPoint& point);
	void ShowOrHideColumn( BMessage* msg);
	//
	virtual BmListViewItem* CreateListViewItem( BmListModelItem* item, 
															  BMessage* archive=NULL) 			= 0;
	//
	BMessage* GetArchiveForItemKey( const BmString&, BMessage* msg=NULL);

	// overrides of controller base:
	void JobIsDone( bool completed);

	// archival of the controller's state-info:
	virtual BmString StateInfoBasename()				= 0;
	virtual BmString StateInfoFilename( bool forRead);
	virtual BMessage* DefaultLayout()	{ return NULL; }

	virtual void PopulateLabelViewMenu( BMenu* menu);

	BmViewItemManager* mViewItemManager;
	BMessage* mInitialStateInfo;
	bool mShowCaption;
	bool mShowBusyView;
	bool mUseStateCache;
	BmListViewItem* mCurrHighlightItem;
	BMessageRunner* mExpandCollapseRunner;
	bool mSittingOnExpander;
	BMessageRunner* mPulsedScrollRunner;
	int32 mPulsedScrollStep;
	bool mDragBetweenItems;

	static const char* const MSG_HIGHITEM;
	static const char* const MSG_EXPAND;
	static const char* const MSG_SCROLL_STEP;

private:

	BmListViewItem* doAddModelItem( BmListViewItem* parent, 
											  BmListModelItem* item, bool redraw);
	void doRemoveModelItem( BmListModelItem* item);

	// Hide copy-constructor and assignment:
	BmListViewController( const BmListViewController&);
#ifndef __POWERPC__
	BmListViewController& operator=( const BmListViewController&);
#endif
};


#endif
