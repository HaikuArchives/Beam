/*
	BmListController.h
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
	
	friend BmListViewController;

protected: 
	// archival-fieldnames:
	static const char* const MSG_EXPANDED;
	static const char* const MSG_CHILDNAMES;
	static const char* const MSG_CHILDREN;

public:
	//
	BmListViewItem( ColumnListView* lv, const BmString& key, 
						 BmListModelItem* item, bool hierarchical=false, 
						 BMessage* archive=NULL);
	virtual ~BmListViewItem();
	
	// native methods:
	void SetTextCols( int16 firstTextCol, const char** content);
	virtual void UpdateView( BmUpdFlags flags, bool redraw = true,
									 uint32 updColBitmap = 0);

	//	overrides from listitem-baseclass:
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	inline const BmString& Key() const	{ return mKey; }
	virtual BmListModelItem* ModelItem() const	
													{ return mModelItem.Get(); }
	
protected:
	BmString mKey;
	BmRef<BmListModelItem> mModelItem;

	// Hide copy-constructor and assignment:
	BmListViewItem( const BmListViewItem&);
	BmListViewItem operator=( const BmListViewItem&);
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
							  float captionWidth=0);
	~BmCLVContainerView();
	
	// native methods:
	void SetBusy();
	void UnsetBusy();
	void PulseBusyView();

	// overrides of MView base:
	BRect layout( BRect);
	
	// setters:
	void SetCaptionText( const char* text);
	inline void SetCaptionWidth( float width) 	{ mCaptionWidth = width; }

private:
	BmCaption* mCaption;
	float mCaptionWidth;
	BmBusyView* mBusyView;

	// Hide copy-constructor and assignment:
	BmCLVContainerView( const BmCLVContainerView&);
	BmCLVContainerView operator=( const BmCLVContainerView&);
};

class BMessageRunner;
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
	BmListViewController( minimax minmax,BRect rect,
								 const char* Name = NULL,
								 list_view_type Type = B_SINGLE_SELECTION_LIST,
								 bool hierarchical = false,
								 bool showLabelView = true,
								 bool showCaption = false,
								 bool showBusyView = false);
	virtual ~BmListViewController();

	// native methods:
	BmListViewItem* FindViewItemFor( BmListModelItem* modelItem) const;
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
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void ShowLabelViewMenu( BPoint pos);
	void AttachedToWindow();
	void MakeEmpty();
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *msg);
	BmCLVContainerView* ScrollView() 	{ return dynamic_cast<BmCLVContainerView*>(fScrollView); }

	// getters:
	inline CLVContainerView* ContainerView()	{ return inherited::fScrollView; }
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
	virtual void HandleDrop( const BMessage* msg);
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
	virtual BMessage* DefaultLayout()	{ return NULL; }

	BmListViewItem* doAddModelItem( BmListViewItem* parent, 
											  BmListModelItem* item);
	void doRemoveModelItem( BmListModelItem* item);

	typedef map< BmListModelItem*, BmListViewItem*> BmViewModelMap;

	BmViewModelMap mViewModelMap;
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

	// Hide copy-constructor and assignment:
	BmListViewController( const BmListViewController&);
#ifndef __POWERPC__
	BmListViewController operator=( const BmListViewController&);
#endif
};


#endif
