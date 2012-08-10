/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Alert.h>
#include <MenuItem.h>
#include <PopUpMenu.h>


//#include <liblayout/LayeredGroup.h>
#include <HGroup.h>
#include <LayeredGroup.h>
#include <MPlayBW.h>
#include <MPlayFW.h>
#include <MFFWD.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BetterButton.h"
#include "BubbleHelper.h"
#include "UserResizeSplitView.h"

#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmPrefsFilterView.h"
#include "BmPrefsFilterChainView.h"
#include "BmTextControl.h"

enum {
	BM_CHAIN_FILTER		= M_PLAYBW_SELECTED,
	BM_UNCHAIN_FILTER		= M_PLAYFW_SELECTED,
	BM_EMPTY_CHAIN			= M_FFWD_SELECTED
};

/********************************************************************************\
	BmFilterChainItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterChainItem::BmFilterChainItem( ColumnListView* lv, 
												  BmListModelItem* _item)
	:	inherited( lv, _item, false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterChainItem::~BmFilterChainItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmFilterChainItem::UpdateView( BmUpdFlags flags, bool redraw, 
												uint32 updColBitmap) {
	BmFilterChain* chain	= dynamic_cast<BmFilterChain*>( ModelItem());
	if (chain && flags & UPD_ALL) {
		const char* cols[] = {
			chain->Key().String(),
			NULL
		};
		SetTextCols( 0, cols);
		updColBitmap = 0xFFFFFFFF;
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}



/********************************************************************************\
	BmFilterChainView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterChainView::BmFilterChainView( int32 width, int32 height)
	:	inherited( BRect(0,0,float(width-1),float(height-1)), "Beam_FilterView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	AddColumn( new CLVColumn( "Name", 250.0, flags, 50.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterChainView::~BmFilterChainView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmFilterChainView::CreateListViewItem( BmListModelItem* item,
																		 BMessage*) {
	return new BmFilterChainItem( this, item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmFilterChainView::AddModelItem( BmListModelItem* item) {
	BmListViewItem* viewItem = inherited::AddModelItem( item);
	if (viewItem) {
		Select( IndexOf(viewItem));
		ScrollToSelection();
	}
	return viewItem;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmFilterChainView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("FilterChainView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmChainedFilterItem
\********************************************************************************/

enum Columns2 {
	COL2_POS = 0,
	COL2_NAME
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmChainedFilterItem::BmChainedFilterItem( ColumnListView* lv, 
														BmListModelItem* _item)
	:	inherited( lv, _item, false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmChainedFilterItem::~BmChainedFilterItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmChainedFilterItem::UpdateView( BmUpdFlags flags, bool redraw, 
												  uint32 updColBitmap) {
	BmChainedFilter* filter( dynamic_cast<BmChainedFilter*>( ModelItem()));
	if (flags & UPD_ALL) {
		BmString pos;
		pos << filter->Position();
		const char* cols[] = {
			pos.String(),
			filter->Key().String(),
			NULL
		};
		SetTextCols( 0, cols);
		if (redraw)
			updColBitmap = 0xFFFFFFFF;
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const int32 
BmChainedFilterItem::GetNumValueForColumn( int32 column_index) const {
	BmChainedFilter* filter( dynamic_cast<BmChainedFilter*>( ModelItem()));
	if (column_index == COL2_POS) {
		// return numerical representation of key (the position):
		return filter->Position();
	} else {
		return 0;		// we don't know this number-column !?!
	}
}



/********************************************************************************\
	BmChainedFilterView
\********************************************************************************/

const char* BmChainedFilterView::MSG_OLD_POS = "bm:oldPos";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmChainedFilterView::BmChainedFilterView( int32 width, int32 height)
	:	inherited( BRect(0,0,float(width-1),float(height-1)), 
					  "Beam_ChainedFilterView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	AddColumn( new CLVColumn( "Pos", 20.0, 
									  flags|CLV_SORT_KEYABLE|CLV_RIGHT_JUSTIFIED
									  		 |CLV_COLDATA_NUMBER|CLV_HIDDEN,
									  20.0));
	AddColumn( new CLVColumn( "Name", 200.0, flags, 80.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL2_POS);
	
	DragBetweenItems( true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmChainedFilterView::~BmChainedFilterView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmChainedFilterView::CreateListViewItem( BmListModelItem* item,
																			BMessage*) {
	return new BmChainedFilterItem( this, item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmChainedFilterView::AddModelItem( BmListModelItem* item) {
	BmListViewItem* viewItem = inherited::AddModelItem( item);
	if (viewItem) {
		Select( IndexOf(viewItem));
		ScrollToSelection();
	}
	return viewItem;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmChainedFilterView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("ChainedFilterView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	InitiateDrag()
		-	
\*------------------------------------------------------------------------------*/
bool BmChainedFilterView::InitiateDrag( BPoint, int32 index, 
													 bool wasSelected) {
	if (!wasSelected)
		return false;
	BMessage dragMsg( BM_CHAINED_FILTER_DRAG);
	BmChainedFilterItem* filterItem 
		= dynamic_cast<BmChainedFilterItem*>(ItemAt( index));
	BmChainedFilter* filter 
		= dynamic_cast<BmChainedFilter*>( filterItem->ModelItem());
	if (filter) {
		dragMsg.AddInt32( MSG_OLD_POS, filter->Position());
		DragMessage( &dragMsg, ItemFrame( IndexOf( filterItem)));
		DeselectAll();
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	AcceptsDropOf( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmChainedFilterView::AcceptsDropOf( const BMessage* msg) {
	return (msg && msg->what == BM_CHAINED_FILTER_DRAG);
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmChainedFilterView::HandleDrop( BMessage* msg) {
	if (msg && msg->what == BM_CHAINED_FILTER_DRAG && mCurrHighlightItem) {
		BmChainedFilter* highlightedFilter 
			= dynamic_cast< BmChainedFilter*>( mCurrHighlightItem->ModelItem());
		int32 oldPos = msg->FindInt32( MSG_OLD_POS);
		int32 newPos = highlightedFilter->Position()
							+ (mCurrHighlightItem->HighlightBottom() ? 1 : -1);
		BmChainedFilterList* chainedFilters
			= dynamic_cast<BmChainedFilterList*>( DataModel().Get());
		if (chainedFilters) {
			BmAutolockCheckGlobal lock( chainedFilters->ModelLocker());
			if (!lock.IsLocked())
				BM_THROW_RUNTIME( 
					chainedFilters->ModelNameNC() << ": Unable to get lock"
				);
			BmModelItemMap::const_iterator iter;
			for( iter = chainedFilters->begin(); 
				  iter != chainedFilters->end(); ++iter) {
				BmChainedFilter* filter 
					= dynamic_cast< BmChainedFilter*>( iter->second.Get());
				if (filter && filter->Position() == oldPos) {
					filter->Position( newPos);
					chainedFilters->RenumberPos();
					BMessage msg( BM_NTFY_ORDER_MODIFIED);
					SendNotices( BM_NTFY_ORDER_MODIFIED, &msg);
					break;
				}
			}
		}
	}
	inherited::HandleDrop( msg);
}



/********************************************************************************\
	BmPrefsFilterChainView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsFilterChainView::BmPrefsFilterChainView() 
	:	inherited( "Filter-Chains")
{
	MBorder* borderl = NULL;
	MBorder* borderr = NULL;
	MView* view = 
		new UserResizeSplitView(
			new HGroup(
				minimax(400,100,1E5,1E5),
				new BetterScrollView( 
					minimax(200,100,1E5,1E5), 
					mFilterChainListView = new BmFilterChainView( 200, 150),
					BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
					| BM_SV_CAPTION,
					"99 filter-chains"
				),
				new Space( minimax(5,0,5,1E5)),
				new VGroup(
					mAddButton = new BetterButton( 
						"Add Chain", 
						new BMessage(BM_ADD_CHAIN), 
						this
					),
					mRemoveButton = new BetterButton( 
						"Remove Chain", 
						new BMessage( BM_REMOVE_CHAIN), 
						this
					),
					new Space(),
					0
				),
				new Space(),
				0
			),
			new HGroup(
				minimax( 400,250),
				borderl = new MBorder( 
					M_LABELED_BORDER, 10, (char*)"Filter-Chain Info",
					new VGroup(
						new HGroup( 
							mChainControl = new BmTextControl( "Filter-Chain name:"),
							0
						),
						new BetterScrollView( 
							minimax(200,120,1E5,1E5), 
							mChainedFilterListView = new BmChainedFilterView( 200, 200),
							BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
						),
						0
					)
				),
				new Space( minimax(10, 0, 10, 1E5)),
				new VGroup( 
					new Space(),
					mAddFilterButton = new MPlayBW( this),
					mRemoveFilterButton = new MPlayFW( this),
					mEmptyChainButton = new MFFWD( this),
					new Space(),
					0
				),
				new Space( minimax(10, 0, 10, 1E5)),
				borderr = new MBorder( 
					M_LABELED_BORDER, 10, (char*)"Available Filters",
					new BetterScrollView( 
						minimax(200,120,1E5,1E5), 
						mAvailableFilterListView = new BmFilterView( 200, 200),
						BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
					)
				),
				0
			),
			"hsplitter", 150, B_HORIZONTAL, true, true, false, false, false, 
			B_FOLLOW_NONE
		);

	float buttonWidth = StringWidth( "Remove Chain")+20;
	mAddButton->ct_mpm.mini.x = mAddButton->ct_mpm.maxi.x
		= mRemoveButton->ct_mpm.mini.x 
		= mRemoveButton->ct_mpm.maxi.x 
		= buttonWidth;

	borderl->ct_mpm = minimax(150,150);
	borderr->ct_mpm = minimax(150,200);

	mGroupView->AddChild( dynamic_cast<BView*>(view));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsFilterChainView::~BmPrefsFilterChainView() {
	TheBubbleHelper->SetHelp( mFilterChainListView, NULL);
	TheBubbleHelper->SetHelp( mChainedFilterListView, NULL);
	TheBubbleHelper->SetHelp( mAvailableFilterListView, NULL);
	TheBubbleHelper->SetHelp( mChainControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mChainControl, 
		"Here you can enter a name for this filter-chain.\n"
		"This name is used to identify this filter-chain in Beam."
	);

	mChainControl->SetTarget( this);

	mFilterChainListView
		->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mFilterChainListView->SetTarget( this);
	mFilterChainListView->StartJob( TheFilterChainList.Get());

	mChainedFilterListView
		->SetSelectionMessage( new BMessage( BM_CHAINED_SELECTION_CHANGED));
	mChainedFilterListView
		->SetInvocationMessage( new BMessage( BM_CHAINED_ITEM_INVOKED));
	mChainedFilterListView->SetTarget( this);
	mChainedFilterListView
		->StartWatching( this, BmChainedFilterView::BM_NTFY_ORDER_MODIFIED);

	mAvailableFilterListView
		->SetSelectionMessage( new BMessage( BM_AVAILABLE_SELECTION_CHANGED));
	mAvailableFilterListView->SetTarget( this);
	mAvailableFilterListView
		->SetInvocationMessage( new BMessage( BM_AVAILABLE_ITEM_INVOKED));
	mAvailableFilterListView->StartJob( TheFilterList.Get());

	UpdateState();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::Activated() {
	inherited::Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::WriteStateInfo() {
	mFilterChainListView->WriteStateInfo();
	mChainedFilterListView->WriteStateInfo();
	mAvailableFilterListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsFilterChainView::SanityCheck() {
	return true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::SaveData() {
	TheFilterChainList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::UndoChanges() {
	TheFilterChainList->Cleanup();
	TheFilterChainList->StartJobInThisThread();
	UpdateState();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 selection = mFilterChainListView->CurrentSelection( 0);
				BmFilterChainItem* filterChainItem 
					= (selection != -1)
							? dynamic_cast<BmFilterChainItem*>( 
									mFilterChainListView->ItemAt( selection))
							: NULL;
				BmFilterChain* chain 
					= filterChainItem
							? dynamic_cast<BmFilterChain*>( 
									filterChainItem->ModelItem())
							: NULL;
				if  (mCurrFilterChain != chain) {
					mCurrFilterChain = chain;
					if (mCurrFilterChain)
						mChainedFilterListView->StartJob( 
							mCurrFilterChain->ChainedFilters(), 
							false
						);
					else
						mChainedFilterListView->DetachModel();
					UpdateState();
				}
				break;
			}
			case BM_CHAINED_SELECTION_CHANGED: {
				int32 selection = mChainedFilterListView->CurrentSelection( 0);
				BmChainedFilterItem* chainedFilterItem 
					= (selection != -1)
							? dynamic_cast<BmChainedFilterItem*>( 
									mChainedFilterListView->ItemAt( selection))
							: NULL;
				mCurrChainedFilter 
					= chainedFilterItem
							? dynamic_cast<BmChainedFilter*>( 
									chainedFilterItem->ModelItem())
							: NULL;
				UpdateState();
				break;
			}
			case BM_AVAILABLE_SELECTION_CHANGED: {
				int32 selection  = mAvailableFilterListView->CurrentSelection( 0);
				BmFilterItem* availableFilterItem
					= (selection != -1)
							? dynamic_cast<BmFilterItem*>( 
											mAvailableFilterListView->ItemAt( selection))
							: NULL;
				mCurrAvailableFilter
					= availableFilterItem
					 	? dynamic_cast<BmFilter*>( availableFilterItem->ModelItem())
					 	: NULL;
				UpdateState();
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if (mCurrFilterChain && source == mChainControl) {
					// rename filter-chain:
					TheFilterChainList->RenameItem( mCurrFilterChain->Key(), 
														     mChainControl->Text());
					NoticeChange();
				}
				break;
			}
			case BM_ADD_CHAIN: {
				BmString key( "new filter-chain");
				for( int32 i=1; TheFilterChainList->FindItemByKey( key); ++i) {
					key = BmString("new filter-chain_")<<i;
				}
				TheFilterChainList->AddItemToList( 
										new BmFilterChain( key.String(), 
																 TheFilterChainList.Get()));
				mChainControl->MakeFocus( true);
				mChainControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_CHAIN: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( 
						"Remove Filter-Chain", 
						(BmString("Are you sure about removing the filter-chain <") 
							<< mCurrFilterChain->Key() << ">?").String(),
						"Remove", "Cancel", NULL, 
						B_WIDTH_AS_USUAL, B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_CHAIN), 
													 BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheFilterChainList->RemoveItemFromList( 
							mCurrFilterChain.Get()
						);
						mCurrFilterChain = NULL;
						NoticeChange();
					}
				}
				break;
			}
			case BM_CHAIN_FILTER:
			case BM_AVAILABLE_ITEM_INVOKED: {
				ChainFilter();
				NoticeChange();
				break;
			}
			case BM_UNCHAIN_FILTER:
			case BM_CHAINED_ITEM_INVOKED: {
				UnchainFilter();
				NoticeChange();
				break;
			}
			case BM_EMPTY_CHAIN: {
				BmListModel* chain = dynamic_cast< BmListModel*>(
					mCurrFilterChain->ChainedFilters()
				);
				if (chain) {
					BmAutolockCheckGlobal lock( chain->ModelLocker());
					if (!lock.IsLocked())
						BM_THROW_RUNTIME( 
							chain->ModelNameNC() << ": Unable to get lock"
						);
					BmModelItemMap::const_iterator iter;
					while( (iter = chain->begin()) != chain->end())
						chain->RemoveItemFromList( iter->second.Get());
					NoticeChange();
				}
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BmChainedFilterView::BM_NTFY_ORDER_MODIFIED: {
						// trigger redraw of chained-filter-list:
						mChainedFilterListView->SortItems();
						NoticeChange();
						break;
					}
				}
				break;
			}
			case BM_COMPLAIN_ABOUT_FIELD: {
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::ChainFilter() {
	if (!mCurrAvailableFilter || !mCurrFilterChain)
		return;
	BmChainedFilter* chainedFilter 
		= new BmChainedFilter( mCurrAvailableFilter->Key().String(), 
									  mCurrFilterChain->ChainedFilters());
	mCurrFilterChain->ChainedFilters()->AddItemToList( chainedFilter);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::UnchainFilter() {
	if (!mCurrChainedFilter || !mCurrFilterChain)
		return;
	mCurrFilterChain->ChainedFilters()->RemoveItemFromList( mCurrChainedFilter.Get());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterChainView::UpdateState() {
	bool haveChain = (mCurrFilterChain != (BmFilterChain*)NULL);
	bool haveChained = (mCurrChainedFilter != (BmChainedFilter*)NULL);
	bool haveAvailable = (mCurrAvailableFilter != (BmFilter*)NULL);

	mChainControl->SetEnabled( 
		haveChain
		&& mCurrFilterChain->Key()!=BM_DefaultItemLabel
		&& mCurrFilterChain->Key()!=BM_OutboundLabel
	);
	mRemoveButton->SetEnabled( 
		haveChain 
		&& mCurrFilterChain->Key()!=BM_DefaultItemLabel
		&& mCurrFilterChain->Key()!=BM_OutboundLabel
	);
	bool alreadyIn = false;
	if (haveAvailable && haveChain) {
		if (mCurrFilterChain->ChainedFilters()->FindItemByKey(
			mCurrAvailableFilter->Key()
		)) {
			alreadyIn = true;
		}
	}
	mAddFilterButton->SetEnabled( haveChain && haveAvailable && !alreadyIn);
	mRemoveFilterButton->SetEnabled( haveChain && haveChained);
	mEmptyChainButton->SetEnabled( haveChain);
	
	if (!haveChain) {
		mChainControl->SetTextSilently( "");
	} else {
		mChainControl->SetTextSilently( mCurrFilterChain->Name().String());
	}
}
