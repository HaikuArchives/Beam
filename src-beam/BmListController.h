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
#define BM_LISTVIEW_SHOW_COLUMN		'bmca'
							// the user has chosen to show a column
#define BM_LISTVIEW_HIDE_COLUMN		'bmcb'
							// the user has chosen to hide a column
#define BM_NTFY_LISTCONTROLLER_MODIFIED 'bmcc'
							// item has been added/removed

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
	static const char* const MSG_CHILDNAMES = "bm:chldnm";
	static const char* const MSG_CHILDREN = 	"bm:chldrn";

public:
	//
	BmListViewItem( BString& key, BmListModelItem* item, bool hierarchical=false, 
						 BMessage* archive=NULL);
	virtual ~BmListViewItem();
	
	// native methods:
	void SetTextCols( int16 firstTextCol, BmListColumn* columnVec, bool truncate=true);
	virtual void UpdateView( BmUpdFlags flags);

	//	overrides from listitem-baseclass:
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	inline const BString Key() const		{ return mKey; }
	virtual BmListModelItem* ModelItem() const	{ return mModelItem.Get(); }
	
protected:
	BString mKey;
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
	virtual void AddAllModelItems();
	virtual BmListViewItem* AddModelItem( BmListModelItem* item);
	virtual void RemoveModelItem( BmListModelItem* item);
	virtual BmListViewItem* UpdateModelItem( BmListModelItem* item, BmUpdFlags updFlags);
	virtual void UpdateModelState( BMessage* msg);
	virtual void UpdateItem( BmListViewItem* item, BmUpdFlags flags);
	virtual void UpdateCaption( const char* text=NULL);
	BmListViewItem* FindViewItemFor( BmListModelItem* modelItem);
	virtual void ItemInvoked( int32 index);
	virtual bool AcceptsDropOf( const BMessage* msg)	{ return false; }
	virtual void HandleDrop( const BMessage* msg);
	void ShowOrHideColumn( BMessage* msg);
	//
	virtual BmListViewItem* CreateListViewItem( BmListModelItem* item, 
															  BMessage* archive=NULL) 			= 0;
	//
	BMessage* GetArchiveForItemKey( BString, BMessage* msg=NULL);
	//
	virtual void WriteStateInfo();
	virtual void ReadStateInfo();

	// overrides of controller base:
	void AttachModel( BmDataModel* model=NULL);
	void DetachModel();
	BHandler* GetControllerHandler() 	{ return this; }
	void StartJob( BmJobModel* model = NULL, bool startInNewThread=true);
	void JobIsDone( bool completed);
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

protected:
	virtual BmListModel* DataModel()		{ return dynamic_cast<BmListModel*>(BmController::DataModel()); }
	// archival of the controller's state-info:
	virtual BString StateInfoBasename()				= 0;
	virtual BMessage* DefaultLayout()	{ return NULL; }

	BMessage* mInitialStateInfo;
	bool mShowCaption;
	bool mShowBusyView;
	bool mUseStateCache;
	BmListViewItem* mCurrHighlightItem;
	BMessageRunner* mUpdatePulseRunner;
	BList mCachedMessages;
	bool mSittingOnExpander;

private:
	BmListViewItem* doAddModelItem( BmListViewItem* parent, BmListModelItem* item);

	// Hide copy-constructor and assignment:
	BmListViewController( const BmListViewController&);
	BmListViewController operator=( const BmListViewController&);
};


#endif
