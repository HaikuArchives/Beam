/*
	BmPrefsFilterView.h
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


#ifndef _BmPrefsFilterView_h
#define _BmPrefsFilterView_h

#include "BmListController.h"
#include "BmFilter.h"
#include "BmPrefsView.h"

/*------------------------------------------------------------------------------*\
	BmFilterItem
		-	
\*------------------------------------------------------------------------------*/
class BmFilterItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmFilterItem( ColumnListView* lv, BmListModelItem* item);
	~BmFilterItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);

private:
	// Hide copy-constructor and assignment:
	BmFilterItem( const BmFilterItem&);
	BmFilterItem operator=( const BmFilterItem&);
};



/*------------------------------------------------------------------------------*\
	BmFilterView
		-	
\*------------------------------------------------------------------------------*/
class BmFilterView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmFilterView* CreateInstance( minimax minmax, int32 width, 
													 int32 height,
													 bool showCaption=true);
	BmFilterView(  minimax minmax, int32 width, int32 height, 
						bool showCaption=true);
	~BmFilterView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, 
													BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "FilterView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "filter"; }
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, 
												  border_style border, 
												  uint32 ResizingMode, 
												  uint32 flags);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);

private:

	// Hide copy-constructor and assignment:
	BmFilterView( const BmFilterView&);
	BmFilterView operator=( const BmFilterView&);
};



class MButton;
class MPopup;
class MStringView;
class VGroup;
class LayeredGroup;
class BmCheckControl;
class BmTextControl;
class BmMenuControl;

/*------------------------------------------------------------------------------*\
	BmPrefsFilterView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsFilterView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_ADD_FILTER			   = 'bmAF',
		BM_REMOVE_FILTER		   = 'bmRF',
		BM_ADD_TO_CHAIN_CHANGED	= 'bmAC'
	};

public:
	// c'tors and d'tor:
	BmPrefsFilterView();
	virtual ~BmPrefsFilterView();
	
	// native methods:
	void ShowFilter( int32 selection);

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
	CLVContainerView* CreateFilterListView( minimax minmax, int32 width, 
														 int32 height);

	BmListViewController* mFilterListView;
	MPopup* mAddPopup;
	MButton* mRemoveButton;
	BmCheckControl* mAddToChainControl;

	MStringView* mInfoLabel;

	LayeredGroup* mLayeredAddonGroup;
	BmTextControl* mFilterControl;

	BmRef<BmFilter> mCurrFilter;
	BmFilterAddonPrefsView* mCurrAddonView;
	
	// Hide copy-constructor and assignment:
	BmPrefsFilterView( const BmPrefsFilterView&);
	BmPrefsFilterView operator=( const BmPrefsFilterView&);
};

#endif
