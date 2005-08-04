/*
	BmPrefsFilterChainView.h
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


#ifndef _BmPrefsFilterChainView_h
#define _BmPrefsFilterChainView_h

#include "BmListController.h"
#include "BmFilterChain.h"
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmFilterChainItem
		-	
\*------------------------------------------------------------------------------*/
class BmFilterChainItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmFilterChainItem( ColumnListView* lv, BmListModelItem* item);
	~BmFilterChainItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);

private:
	// Hide copy-constructor and assignment:
	BmFilterChainItem( const BmFilterChainItem&);
	BmFilterChainItem operator=( const BmFilterChainItem&);
};



/*------------------------------------------------------------------------------*\
	BmFilterChainView
		-	
\*------------------------------------------------------------------------------*/
class BmFilterChainView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// c'tors and d'tor:
	BmFilterChainView( int32 width, int32 height);
	~BmFilterChainView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, 
													BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "FilterChainView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "filter-chain"; }

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

private:

	// Hide copy-constructor and assignment:
	BmFilterChainView( const BmFilterChainView&);
	BmFilterChainView operator=( const BmFilterChainView&);
};

enum {
	BM_ADD_CHAIN				= 'bmAS',
	BM_REMOVE_CHAIN			= 'bmRS'
};



/*------------------------------------------------------------------------------*\
	BmChainedFilterItem
		-	
\*------------------------------------------------------------------------------*/
class BmChainedFilterItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmChainedFilterItem( ColumnListView* lv, BmListModelItem* item);
	~BmChainedFilterItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
	const int32 GetNumValueForColumn( int32 column_index) const;

private:
	// Hide copy-constructor and assignment:
	BmChainedFilterItem( const BmChainedFilterItem&);
	BmChainedFilterItem operator=( const BmChainedFilterItem&);
};



/*------------------------------------------------------------------------------*\
	BmChainedFilterView
		-	
\*------------------------------------------------------------------------------*/
class BmChainedFilterView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// c'tors and d'tor:
	BmChainedFilterView( int32 width, int32 height);
	~BmChainedFilterView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, 
													BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "ChainedFilterView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "filter"; }
	bool InitiateDrag( BPoint, int32 index, bool wasSelected);
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( BMessage* msg);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

	// msg-fields:
	static const char* MSG_OLD_POS;

	// message-types:
	enum {
		BM_CHAINED_FILTER_DRAG			= 'bmCD',
		BM_NTFY_ORDER_MODIFIED 			= 'bmCE'
							// chained filters have been reordered
	};
	
private:

	// Hide copy-constructor and assignment:
	BmChainedFilterView( const BmChainedFilterView&);
	BmChainedFilterView operator=( const BmChainedFilterView&);
};



class MButton;
class MPlayFW;
class MPlayBW;
class MFFWD;
class BmTextControl;
/*------------------------------------------------------------------------------*\
	BmPrefsFilterChainView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsFilterChainView : public BmPrefsView {
	typedef BmPrefsView inherited;

	// message-types:
	enum {
		BM_CHAINED_SELECTION_CHANGED	= 'bmCC',
		BM_CHAINED_ITEM_INVOKED			= 'bmCI',
		BM_AVAILABLE_SELECTION_CHANGED= 'bmAC',
		BM_AVAILABLE_ITEM_INVOKED		= 'bmAI'
	};

public:
	// c'tors and d'tor:
	BmPrefsFilterChainView();
	virtual ~BmPrefsFilterChainView();
	
	// native methods:
	void UpdateState();
	void ChainFilter();
	void UnchainFilter();

	// overrides of BmPrefsView base:
	void Initialize();
	void Activated();
	void WriteStateInfo();
	void SaveData();
	void UndoChanges();
	bool SanityCheck();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:

	// setters:

private:
	BmListViewController* mFilterChainListView;
	BmListViewController* mChainedFilterListView;
	BmListViewController* mAvailableFilterListView;
	MButton* mAddButton;
	MButton* mRemoveButton;
	MPlayBW* mAddFilterButton;
	MPlayFW* mRemoveFilterButton;
	MFFWD* mEmptyChainButton;
	BmTextControl* mChainControl;

	BmRef<BmFilterChain> mCurrFilterChain;
	BmRef<BmChainedFilter> mCurrChainedFilter;
	BmRef<BmFilter> mCurrAvailableFilter;
	
	// Hide copy-constructor and assignment:
	BmPrefsFilterChainView( const BmPrefsFilterChainView&);
	BmPrefsFilterChainView operator=( const BmPrefsFilterChainView&);
};

#endif
