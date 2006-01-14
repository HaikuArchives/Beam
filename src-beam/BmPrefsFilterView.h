/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	// c'tors and d'tor:
	BmFilterView( int32 width, int32 height);
	~BmFilterView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, 
													BMessage* archive=NULL);
	
	// overrides of controller base:
	BmString StateInfoBasename()			{ return "FilterView"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	const char* ItemNameForCaption()		{ return "filter"; }

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
