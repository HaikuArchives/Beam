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
#include <MenuItem.h>
#include <PopUpMenu.h>


//#include <liblayout/LayeredGroup.h>
#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/MTabView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "UserResizeSplitView.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMultiLineTextControl.h"
#include "BmPrefs.h"
#include "BmPrefsFilterView.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmFilterItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterItem::BmFilterItem( const BmString& key, BmListModelItem* _item)
	:	inherited( key, _item, false)
{
	UpdateView( UPD_ALL);
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
void BmFilterItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmFilter* filter( dynamic_cast<BmFilter*>( ModelItem()));
	if (flags & UPD_ALL) {
		BmListColumn cols[] = {
			{ filter->Key().String(),			false },
			{ NULL, false }
		};
		SetTextCols( 0, cols);
	}
}



/********************************************************************************\
	BmFilterView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterView* BmFilterView::CreateInstance( minimax minmax, int32 width, int32 height) {
	return new BmFilterView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmFilterView::BmFilterView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FilterView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true, true, false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);

	Initialize( BRect( 0,0,width-1,height-1),
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "Name", 300.0, flags, 50.0));

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
	return new BmFilterItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmFilterView::CreateContainer( bool horizontal, bool vertical, 
												  				  bool scroll_view_corner, 
												  				  border_style border, 
																  uint32 ResizingMode, 
																  uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView, 
											 be_plain_font->StringWidth(" 99 filters "));
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
	} catch( exception &err) {
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
{
	MBorder* border = NULL;
	MView* view = 
		new UserResizeSplitView(
			new VGroup(
				minimax(200,60),
				new HGroup(
					CreateFilterListView( minimax(200,60,1E5,1E5), 200, 120),
					new Space( minimax(5,0,5,0)),
					new VGroup(
						mAddButton = new MButton("Add Filter", new BMessage(BM_ADD_FILTER), this),
						mRemoveButton = new MButton("Remove Filter", new BMessage( BM_REMOVE_FILTER), this),
						new Space(),
						0
					),
					0
				),
				0
			),
			border = new MBorder( M_LABELED_BORDER, 10, (char*)"Filter Info",
				new VGroup(
					new HGroup( 
						mFilterControl = new BmTextControl( "Filter name:"),
						new Space(),
						mTestButton = new MButton( "Check the SIEVE-script", new BMessage( BM_TEST_FILTER), this, minimax(-1,-1,-1,-1)),
						0
					),
					new Space( minimax(0,5,0,5)),
					mTabView = new MTabView(),
					0
				)
			),
			"hsplitter", 120, B_HORIZONTAL, true, true, false, B_FOLLOW_NONE
		);

	border->ct_mpm = minimax(400,150);
	mTabView->Add(
		new MTab( 
			new Space(minimax(0,0,1E5,1E5)), 
			(char*)"Graphical View"
		)
	);
	mTabView->Add(
		new MTab( 
			new VGroup( 
				mContentControl = new BmMultiLineTextControl( ""), 
				new Space( minimax(0,2,0,2)),
				0
			),
			(char*)"Script View"
		)
	);

	mGroupView->AddChild( dynamic_cast<BView*>(view));

	float divider = mFilterControl->Divider();
	mFilterControl->SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsFilterView::~BmPrefsFilterView() {
	TheBubbleHelper.SetHelp( mFilterListView, NULL);
	TheBubbleHelper.SetHelp( mFilterControl, NULL);
	TheBubbleHelper.SetHelp( mContentControl, NULL);
	TheBubbleHelper.SetHelp( mTestButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper.SetHelp( mFilterListView, "This listview shows every filter you have defined.");
	TheBubbleHelper.SetHelp( mFilterControl, "Here you can enter a name for this filter.\nThis name is used to identify this filter in Beam.");
	TheBubbleHelper.SetHelp( mContentControl, "Here you can enter the content of this filter (a SIEVE script).");
	TheBubbleHelper.SetHelp( mTestButton, "Here you can check the syntax of the SIEVE-script.");

	mFilterControl->SetTarget( this);
	mContentControl->SetTarget( this);

	mFilterListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mFilterListView->SetTarget( this);
	mFilterListView->StartJob( TheFilterList.Get());
	ShowFilter( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsFilterView::Activated() {
	inherited::Activated();
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
	if (!InitDone())
		return true;
	BmString complaint, fieldName;
	BMessage msg( BM_COMPLAIN_ABOUT_FIELD);
	BmModelItemMap::const_iterator iter;
	for( iter = TheFilterList->begin(); iter != TheFilterList->end(); ++iter) {
		BmFilter* filter = dynamic_cast<BmFilter*>( iter->second.Get());
		if (filter && !filter->SanityCheck( complaint, fieldName)) {
			msg.AddPointer( MSG_ITEM, (void*)filter);
			msg.AddString( MSG_COMPLAINT, complaint.String());
			if (fieldName.Length())
				msg.AddString( MSG_FIELD_NAME, fieldName.String());
			Looper()->PostMessage( &msg, this);
			return false;
		}
	}
	return true;
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
			case BM_MULTILINE_TEXTFIELD_MODIFIED: {
				if (mCurrFilter) {
					BView* srcView = NULL;
					msg->FindPointer( "source", (void**)&srcView);
					BmMultiLineTextControl* source = dynamic_cast<BmMultiLineTextControl*>( srcView);
					if ( source == mContentControl) {
						mCurrFilter->Content( mContentControl->Text());
						NoticeChange();
					}
				}
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( mCurrFilter && source == mFilterControl) {
					TheFilterList->RenameItem( mCurrFilter->Name(), mFilterControl->Text());
					NoticeChange();
				}
				break;
			}
			case BM_TEST_FILTER: {
				if (mCurrFilter) {
					if (!mCurrFilter->CompileScript()) {
						BAlert* alert = new BAlert( "Filter-Test", 
															 mCurrFilter->ErrorString().String(),
													 		 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
													 		 B_INFO_ALERT);
						alert->SetShortcut( 0, B_ESCAPE);
						alert->Go();
					}
				}
				break;
			}
			case BM_ADD_FILTER: {
				BmString key( "new filter");
				for( int32 i=1; TheFilterList->FindItemByKey( key); ++i) {
					key = BmString("new filter_")<<i;
				}
				TheFilterList->AddItemToList( new BmFilter( key.String(), TheFilterList.Get()));
				mFilterControl->MakeFocus( true);
				mFilterControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_FILTER: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Filter", 
														 (BmString("Are you sure about removing the filter <") << mCurrFilter->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_FILTER), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheFilterList->RemoveItemFromList( mCurrFilter.Get());
						mCurrFilter = NULL;
						NoticeChange();
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
					BmListViewItem* filterItem = mFilterListView->FindViewItemFor( filter);
					if (filterItem)
						mFilterListView->Select( mFilterListView->IndexOf( filterItem));
					mTabView->Select( 1);
				} else {
					// second step, set corresponding focus:
					BmString fieldName;
					fieldName = msg->FindString( MSG_FIELD_NAME);
					if (fieldName.ICompare( "content")==0)
						mContentControl->MakeFocus( true);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
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
	mContentControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrFilter = NULL;
		mFilterControl->SetTextSilently( "");
		mContentControl->SetTextSilently( "");
		mTestButton->SetEnabled( false);
	} else {
		BmFilterItem* filterItem = dynamic_cast<BmFilterItem*>(mFilterListView->ItemAt( selection));
		if (filterItem) {
			mCurrFilter = dynamic_cast<BmFilter*>( filterItem->ModelItem());
			if (mCurrFilter) {
				mFilterControl->SetTextSilently( mCurrFilter->Name().String());
				mContentControl->SetTextSilently( mCurrFilter->Content().String());
				mTestButton->SetEnabled( true);
			}
		} else
			mCurrFilter = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsFilterView::CreateFilterListView( minimax minmax, int32 width, int32 height) {
	mFilterListView = BmFilterView::CreateInstance( minmax, width, height);
	mFilterListView->ClickSetsFocus( true);
	return mFilterListView->ContainerView();
}
