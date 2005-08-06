/*
	BmPrefsFilterView.cpp
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

#include <Alert.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <LayeredGroup.h>
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "UserResizeSplitView.h"

#include "BeamApp.h"
#include "BmCheckControl.h"
#include "BmFilter.h"
#include "BmFilterAddonPrefs.h"
#include "BmFilterChain.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailRef.h"
#include "BmMsgTypes.h"
#include "BmMultiLineTextControl.h"
#include "BmPrefs.h"
#include "BmPrefsFilterView.h"
#include "BmPrefsWin.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmFilterItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_STATE
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterItem::BmFilterItem( ColumnListView* lv, BmListModelItem* _item)
	:	inherited( lv, _item, false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterItem::~BmFilterItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmFilterItem::UpdateView( BmUpdFlags flags, bool redraw, 
										 uint32 updColBitmap) {
	BmFilter* filter( dynamic_cast<BmFilter*>( ModelItem()));
	if (flags & UPD_ALL) {
		const char* cols[] = {
			filter->Key().String(),
			filter->IsDisabled() ? "DISABLED" : "",
			NULL
		};
		SetTextCols( 0, cols);
		updColBitmap = 0xFFFFFFFF;
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}



/********************************************************************************\
	BmFilterView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterView::BmFilterView( int32 width, int32 height)
	:	inherited( BRect(0,0,width-1,height-1), "Beam_FilterView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	AddColumn( new CLVColumn( "Name", 200.0, flags|CLV_SORT_KEYABLE, 50.0));
	AddColumn( new CLVColumn( "State", 80.0, flags, 50.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterView::~BmFilterView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmFilterView::CreateListViewItem( BmListModelItem* item,
																  BMessage*) {
	return new BmFilterItem( this, item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmFilterView::AddModelItem( BmListModelItem* item) {
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
void BmFilterView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("FilterView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsFilterView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsFilterView::BmPrefsFilterView() 
	:	inherited( "Filters")
	,	mCurrAddonView( NULL)
{
	// N.B.: Since a LayeredGroup does not seem to work properly when it gets its
	//       children added through AddChild(), we take the somewhat ugly path of
	//			filling the addon-views into an array and statically stuff the 
	//			array's items into the LayeredGroup (see below):
	BmFilterAddonPrefsView* prefsView[10] 
		= { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
	// We collect the different addon-kinds in another arraay in order to stuff
	// them into the add-filter popup-menu:
	char* k[10] 
		= { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

	// collect filter-addon-views and addon-names:
	int pv=0;
	int kk=0;
	BmFilterAddonMap::iterator iter;
	for( iter = FilterAddonMap.begin(); iter != FilterAddonMap.end(); ++iter) {
		BmInstantiateFilterPrefsFunc func 
			= iter->second.instantiateFilterPrefsFunc;
		if (func) {
			BmFilterAddonPrefsView* pview 
				= (*func)( 400, 200, 1E5, 1E5, iter->first);
			prefsView[pv++] = iter->second.addonPrefsView = pview;
			k[kk++] = (char*)pview->Kind();
		}
	}

	MView* view = 
		new UserResizeSplitView(
			new VGroup(
				minimax(200,120),
				new HGroup(
					new BetterScrollView( 
						minimax(200,60,1E5,1E5), 
						mFilterListView = new BmFilterView( 200, 100),
						BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
						| BM_SV_CAPTION,
						"99 filters"
					),
					new Space( minimax(5,0,5,0)),
					new VGroup(
						new HGroup(
							mAddPopup = new MPopup( 
								NULL, 
								new BMessage(BM_ADD_FILTER), this, 
								k[0], k[1], k[2], k[3], k[4], k[5], 
								k[6], k[7], k[8], k[9], NULL
							),
							mAddToChainControl = new BmCheckControl( 
								"Add new filter to default-chain, too", 
								new BMessage(BM_ADD_TO_CHAIN_CHANGED), 
								this
							),
							0
						),
						new HGroup(
							mRemoveButton = new MButton( 
								"Remove Filter", 
								new BMessage( BM_REMOVE_FILTER), 
								this, minimax( -1, -1, -1, -1)
							),
							new Space(),
							0
						),
						new Space(),
						0
					),
					0
				),
				0
			),
			new VGroup(
				minimax(400,300),
				mFilterControl = new BmTextControl( "Filter name:"),
				new Space( minimax(0,5,0,5)),
				mLayeredAddonGroup = new LayeredGroup(
					// the first (more or less empty) view is used when no filter is
					// selected or the add-on for the current filter isn't available:
					new VGroup(
						new Space( minimax(5,0,5,0)),
						mInfoLabel = new MStringView( ""),
						new Space( ),
						0
					),
					// now add up to 10 addon-views (this means that Beam currently
					// only supports 10 different filter-addon types):
					prefsView[0],
					prefsView[1],
					prefsView[2],
					prefsView[3],
					prefsView[4],
					prefsView[5],
					prefsView[6],
					prefsView[7],
					prefsView[8],
					prefsView[9],
					0
				),
				0
			),
			"hsplitter", 180, B_HORIZONTAL, true, true, false, false, false, 
			B_FOLLOW_NONE
		);

	mAddToChainControl->SetValue( true);

	mGroupView->AddChild( dynamic_cast<BView*>(view));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsFilterView::~BmPrefsFilterView() {
	TheBubbleHelper->SetHelp( mFilterListView, NULL);
	TheBubbleHelper->SetHelp( mFilterControl, NULL);
	TheBubbleHelper->SetHelp( mAddToChainControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mFilterListView, 
		"This listview shows every filter you have defined."
	);
	TheBubbleHelper->SetHelp( 
		mFilterControl, 
		"Here you can enter a name for this filter.\n"
		"This name is used to identify this filter in Beam."
	);
	TheBubbleHelper->SetHelp( 
		mAddToChainControl, 
		"If checked, new filters will be added\n"
		"to the default chain automatically."
	);

	mFilterControl->SetTarget( this);
	mAddToChainControl->SetTarget( this);

	mFilterListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mFilterListView->SetTarget( this);
	mFilterListView->StartJob( TheFilterList.Get());

	mAddPopup->Menu()->SetLabelFromMarked( false);
	mAddPopup->MenuBar()->SetLabelFromMarked( false);
	mAddPopup->MenuItem()->SetLabel( "  Add Filter...  ");

	// initialize all addon-views:
	BmFilterAddonMap::const_iterator iter;
	for( iter = FilterAddonMap.begin(); iter != FilterAddonMap.end(); ++iter) {
		BmFilterAddonPrefsView* prefsView = iter->second.addonPrefsView;
		if (prefsView) {
			prefsView->Initialize();
			prefsView->StartWatching( this, BM_NTFY_FILTER_ADDON_MODIFIED);
		}
	}

	ShowFilter( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::Activated() {
	inherited::Activated();

	// activate all addon-views:
	BmFilterAddonMap::const_iterator iter;
	for( iter = FilterAddonMap.begin(); iter != FilterAddonMap.end(); ++iter) {
		BmFilterAddonPrefsView* prefsView = iter->second.addonPrefsView;
		if (prefsView)
			prefsView->Activate();
	}

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::WriteStateInfo() {
	mFilterListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsFilterView::SanityCheck() {
	return DoSanityCheck( TheFilterList.Get(), Name());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::SaveData() {
	TheFilterList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::UndoChanges() {
	TheFilterList->Cleanup();
	TheFilterList->StartJobInThisThread();
	ShowFilter( -1);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mFilterListView->CurrentSelection( 0);
				ShowFilter( index);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( mCurrFilter && source == mFilterControl) {
					// rename filter:
					TheFilterList->RenameItem( 
						mCurrFilter->Name(), mFilterControl->Text()
					);
					NoticeChange();
				}
				break;
			}
			case BM_ADD_FILTER: {
				BMenuItem* item = mAddPopup->Menu()->FindMarked();
				if (item) {
					BmString filterKind( item->Label());
					BmString defaultKey 
						= TheFilterList->DefaultNameForFilterKind( filterKind);
					BmString key = defaultKey;
					for( int32 i=1; TheFilterList->FindItemByKey( key); ++i) {
						key = defaultKey;
						key << "_" << i;
					}
					BmFilter* newFilter = new BmFilter( key.String(), filterKind, 
																   TheFilterList.Get());
					TheFilterList->AddItemToList( newFilter);
					mFilterControl->MakeFocus( true);
					mFilterControl->TextView()->SelectAll();
					if (mAddToChainControl->Value()) {
						// add new filter to default chain:
						BmRef< BmListModelItem> chainItem;
						chainItem = TheFilterChainList->FindItemByKey( 
							BM_DefaultItemLabel
						);
						BmFilterChain* chain = dynamic_cast< BmFilterChain*>( 
							chainItem.Get()
						);
						if (chain) {
							BmChainedFilter* chainedFilter 
								= new BmChainedFilter( newFilter->Key().String(), 
															  chain->ChainedFilters());
							chain->ChainedFilters()->AddItemToList( chainedFilter);
						}
					}
					NoticeChange();
				}
				break;
			}
			case BMM_CREATE_FILTER: {
				BmMailRefVect* refVect = NULL;
				msg->FindPointer( BeamApplication::MSG_MAILREF_VECT, 
									  (void**)&refVect);
				if (!refVect || refVect->empty())
					break;
				BmMailRef* ref = refVect->front().Get();
				BmString key( "new filter");
				for( int32 i=1; TheFilterList->FindItemByKey( key); ++i) {
					key = BmString("new filter_")<<i;
				}
				BmString sieveKind( "SIEVE");
				BmFilter* newFilter = new BmFilter( key.String(), sieveKind, 
															   TheFilterList.Get());
				BmFilterAddon* addon = newFilter->Addon();
				if (ref && addon) {
					addon->SetupFromMailData( 
						ref->Subject(), ref->From(), ref->To()
					);
					TheFilterList->AddItemToList( newFilter);
					mFilterControl->MakeFocus( true);
					mFilterControl->TextView()->SelectAll();
					if (mAddToChainControl->Value()) {
						// add new filter to default chain:
						BmRef< BmListModelItem> chainItem;
						chainItem = TheFilterChainList->FindItemByKey( 
							BM_DefaultItemLabel
						);
						BmFilterChain* chain = dynamic_cast< BmFilterChain*>( 
							chainItem.Get()
						);
						if (chain) {
							BmChainedFilter* chainedFilter 
								= new BmChainedFilter( newFilter->Key().String(), 
															  chain->ChainedFilters());
							chain->ChainedFilters()->AddItemToList( chainedFilter);
						}
					}
					NoticeChange();
				}
				delete refVect;
				break;
			}
			case BM_REMOVE_FILTER: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( 
						"Remove Filter", 
						(BmString("Are you sure about removing the filter\n\n\t<") 
							<< mCurrFilter->Name() << ">?").String(),
						"Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( 
						new BMessage(BM_REMOVE_FILTER), 
						BMessenger( this)
					));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheFilterChainList->RemoveFilterFromAllChains( 
							mCurrFilter->Key()
						);
						TheFilterList->RemoveItemFromList( mCurrFilter.Get());
						mCurrFilter = NULL;
						NoticeChange();
					}
				}
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_FILTER_ADDON_MODIFIED: {
						NoticeChange();
						break;
					}
				}
				break;
			}
			case BM_COMPLAIN_ABOUT_FIELD: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					BmString complaint;
					complaint = msg->FindString( MSG_COMPLAINT);
					// first step, tell user about complaint:
					BAlert* alert = new BAlert( "Sanity Check Failed", 
														 complaint.String(),
													 	 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(*msg), BMessenger( this)));
					BmFilter* filter=NULL;
					msg->FindPointer( MSG_ITEM, (void**)&filter);
					BmListViewItem* filterItem 
						= mFilterListView->FindViewItemFor( filter);
					if (filterItem)
						mFilterListView->Select( 
							mFilterListView->IndexOf( filterItem)
						);
				} else {
					// second step, set corresponding focus:
					BmString fieldName;
					fieldName = msg->FindString( MSG_FIELD_NAME);
				}
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
void BmPrefsFilterView::ShowFilter( int32 selection) {
	bool enabled = (selection != -1);
	mFilterControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrFilter = NULL;
		mCurrAddonView = NULL;
		mInfoLabel->SetText("");
		mLayeredAddonGroup->ActivateLayer( 0);
		mFilterControl->SetTextSilently( "");
	} else {
		BmFilterItem* filterItem = dynamic_cast< BmFilterItem*>(
			mFilterListView->ItemAt( selection)
		);
		if (filterItem) {
			if  (mCurrFilter.Get() != filterItem->ModelItem()) {
				mCurrFilter = dynamic_cast<BmFilter*>( filterItem->ModelItem());
				if (mCurrFilter) {
					mFilterControl->SetTextSilently( mCurrFilter->Name().String());
					// now find corresponding addon-view...
					mCurrAddonView 
						= FilterAddonMap[mCurrFilter->Kind()].addonPrefsView;
					// ...activate it (have to search through LayeredGroup, tsk!)...
					for( int i=0; i<mLayeredAddonGroup->CountChildren(); ++i) {
						BmFilterAddonPrefsView* view 
							= dynamic_cast< BmFilterAddonPrefsView*>( 
									mLayeredAddonGroup->ChildAt( i)
							  );
						if (view == mCurrAddonView)
							mLayeredAddonGroup->ActivateLayer( i);
					}
					// ...and show the filter's data inside the addon-view:
					if (mCurrAddonView)
						mCurrAddonView->ShowFilter( mCurrFilter->Addon());
					else {
						// addon has not been loaded, we tell user:
						mLayeredAddonGroup->ActivateLayer( 0);
						BmString s = BmString("The corresponding add-on (")
											<< mCurrFilter->Kind() 
											<< ") could not be loaded, so this filter "
											<< "has been disabled!";
						mInfoLabel->SetText( s.String());
					}
				}
			}
		} else
			mCurrFilter = NULL;
	}
}
