/*
	BmPrefsSignatureView.cpp
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

#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmMultiLineTextControl.h"
#include "BmPrefs.h"
#include "BmPrefsSignatureView.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSignatureItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_DYNAMIC,
	COL_CONTENT
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSignatureItem::BmSignatureItem( const BmString& key, BmListModelItem* _item)
	:	inherited( key, _item, false)
{
	UpdateView( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSignatureItem::~BmSignatureItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSignatureItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmSignature* sig = ModelItem();
	if (flags & UPD_ALL) {
		BmString beautifiedContent( sig->Content());
		beautifiedContent.ReplaceAll( "\n", "\\n");
		beautifiedContent.ReplaceAll( "\t", "\\t");

		BmListColumn cols[] = {
			{ sig->Key().String(),						false },
			{ sig->Dynamic() ? "*" : "",				false },
			{ beautifiedContent.String(),				false },
			{ NULL, false }
		};
		SetTextCols( 0, cols);
	}
}



/********************************************************************************\
	BmSignatureView
\********************************************************************************/

BmSignatureView* BmSignatureView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSignatureView* BmSignatureView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmSignatureView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSignatureView::BmSignatureView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_SignatureView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true, true, false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;

	Initialize( BRect( 0,0,width-1,height-1),
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "Name", 80.0, CLV_SORT_KEYABLE|flags, 50.0));
	AddColumn( new CLVColumn( "Dynamic", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Content", 400.0, flags, 40.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSignatureView::~BmSignatureView() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSignatureView::CreateListViewItem( BmListModelItem* item,
																	  BMessage*) {
	return new BmSignatureItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmSignatureView::CreateContainer( bool horizontal, bool vertical, 
												  				  bool scroll_view_corner, 
												  				  border_style border, 
																  uint32 ResizingMode, 
																  uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView, 
											 be_plain_font->StringWidth(" 99 signatures "));
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSignatureView::AddModelItem( BmListModelItem* item) {
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
void BmSignatureView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("SignatureView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsSignatureView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSignatureView::BmPrefsSignatureView() 
	:	inherited( "Signatures")
{
	MView* view = 
		new VGroup(
			CreateSigListView( minimax(400,60,1E5,1E5), 400, 150),
			new HGroup(
				mAddButton = new MButton("Add Signature", new BMessage(BM_ADD_SIGNATURE), this),
				mRemoveButton = new MButton("Remove Signature", new BMessage( BM_REMOVE_SIGNATURE), this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Signature Info",
				new VGroup(
					mSignatureControl = new BmTextControl( "Signature name:"),
					new Space( minimax(0,5,0,5)),
					mContentControl = new BmMultiLineTextControl( "Content:", false, 4, 0, true),
					new Space( minimax(0,5,0,5)),
					new HGroup( 
						mDynamicControl = new BmCheckControl( "Content is dynamic (execute the given command via shell, fetch stdout as signature)", 
																		  new BMessage(BM_DYNAMIC_CHANGED), 
																		  this),
						new Space(),
						mTestButton = new MButton( "Test this signature", new BMessage( BM_TEST_SIGNATURE), this, minimax(-1,-1,-1,-1)),
						0
					),
					new HGroup( 
						mCharsetControl = new BmMenuControl( "Charset:", new BPopUpMenu("")),
						new Space(),
						0
					),
					0
				)
			),
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Signature Display",
				new VGroup(
					mSignatureRxControl = new BmTextControl( "Regex that finds start of signature:"),
					0
				)
			),
			new Space(minimax(0,0,1E5,1E5,0.1)),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));

	float divider = mSignatureControl->Divider();
	divider = MAX( divider, mContentControl->Divider());
	divider = MAX( divider, mCharsetControl->Divider());
	mSignatureControl->SetDivider( divider);
	mContentControl->SetDivider( divider);
	mCharsetControl->SetDivider( divider);

	mSignatureRxControl->SetText( ThePrefs->GetString("SignatureRX").String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSignatureView::~BmPrefsSignatureView() {
	TheBubbleHelper.SetHelp( mSigListView, NULL);
	TheBubbleHelper.SetHelp( mSignatureControl, NULL);
	TheBubbleHelper.SetHelp( mContentControl, NULL);
	TheBubbleHelper.SetHelp( mDynamicControl, NULL);
	TheBubbleHelper.SetHelp( mCharsetControl, NULL);
	TheBubbleHelper.SetHelp( mSignatureRxControl, NULL);
	TheBubbleHelper.SetHelp( mTestButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper.SetHelp( mSigListView, "This listview shows every signature you have defined.");
	TheBubbleHelper.SetHelp( mSignatureControl, "Here you can enter a name for this signature.\nThis name is used to identify this signature in Beam.");
	TheBubbleHelper.SetHelp( mContentControl, "Here you can enter the signature text (static mode) \nor a shell-command (dynamic mode).");
	TheBubbleHelper.SetHelp( mDynamicControl, "Beam supports two kinds of signatures:\n\
static:\n\
	The text entered into the content field represents the signature itself.\n\
	Exactly this text will be appended to a mail that uses this sig.\n\
dynamic:\n\
	The text entered into the content field is a shell-command whose output (STDOUT)\n\
	represents the signature. You can use this mode to implement things like\n\
	random signature selection from an external database, for instance.");
	TheBubbleHelper.SetHelp( mCharsetControl, "Here you can define the charset of the signature-text.\nIf in doubt, just leave the default.");
	TheBubbleHelper.SetHelp( mSignatureRxControl, "This is the regular expression (perl-style) used by Beam\nto split off the signature when viewing mails.");
	TheBubbleHelper.SetHelp( mTestButton, "Here you can testrun a dynamic signature.");

	mSignatureControl->SetTarget( this);
	mContentControl->SetTarget( this);
	mDynamicControl->SetTarget( this);
	mSignatureRxControl->SetTarget( this);

	// add all encodings to menu:
	AddCharsetMenu( mCharsetControl->Menu(), this, BM_CHARSET_SELECTED);

	// mark default charset:
	BmString charset( ThePrefs->GetString( "DefaultCharset"));
	mCharsetControl->MarkItem( charset.String());
	mCharsetControl->MenuItem()->SetLabel( charset.String());

	mSigListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mSigListView->SetTarget( this);
	mSigListView->StartJob( TheSignatureList.Get());
	ShowSignature( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::Activated() {
	inherited::Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::WriteStateInfo() {
	mSigListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::SaveData() {
	TheSignatureList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::UndoChanges() {
	TheSignatureList->Cleanup();
	TheSignatureList->StartJobInThisThread();
	ShowSignature( -1);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSignatureView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mSigListView->CurrentSelection( 0);
				ShowSignature( index);
				break;
			}
			case BM_MULTILINE_TEXTFIELD_MODIFIED: {
				if (mCurrSig) {
					BView* srcView = NULL;
					msg->FindPointer( "source", (void**)&srcView);
					BmMultiLineTextControl* source = dynamic_cast<BmMultiLineTextControl*>( srcView);
					if ( source == mContentControl) {
						mCurrSig->Content( mContentControl->Text());
						NoticeChange();
					}
				}
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( mCurrSig && source == mSignatureControl)
					TheSignatureList->RenameItem( mCurrSig->Name(), mSignatureControl->Text());
				else if ( source == mSignatureRxControl)
					ThePrefs->SetString("SignatureRX", mSignatureRxControl->Text());
				NoticeChange();
				break;
			}
			case BM_DYNAMIC_CHANGED: {
				bool val = mDynamicControl->Value();
				if (mCurrSig)
					mCurrSig->Dynamic( val);
				mCharsetControl->SetEnabled( val);
				mTestButton->SetEnabled( val);
				NoticeChange();
				break;
			}
			case BM_CHARSET_SELECTED: {
				if (mCurrSig) {
					BMenuItem* item = NULL;
					msg->FindPointer( "source", (void**)&item);
					if (item) {
						mCharsetControl->ClearMark();
						mCharsetControl->MenuItem()->SetLabel( item->Label());
						mCurrSig->Charset( item->Label());
						item->SetMarked( true);
					}
					else
						mCurrSig->Charset( ThePrefs->GetString( "DefaultCharset"));
					NoticeChange();
				}
				break;
			}
			case BM_TEST_SIGNATURE: {
				if (mCurrSig) {
					BmString sigString = mCurrSig->GetSignatureString();
					if (sigString.Length()) {
						BAlert* alert = new BAlert( "Signature-Test", 
														 (BmString("Please check the results below.\n-- \n")<<sigString).String(),
													 	 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
													 	 B_INFO_ALERT);
						alert->SetShortcut( 0, B_ESCAPE);
						alert->Go();
					}
				}
				break;
			}
			case BM_ADD_SIGNATURE: {
				BmString key( "new signature");
				for( int32 i=1; TheSignatureList->FindItemByKey( key); ++i) {
					key = BmString("new signature_")<<i;
				}
				TheSignatureList->AddItemToList( new BmSignature( key.String(), TheSignatureList.Get()));
				mSignatureControl->MakeFocus( true);
				mSignatureControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_SIGNATURE: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Signature", 
														 (BmString("Are you sure about removing the signature <") << mCurrSig->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_SIGNATURE), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheSignatureList->RemoveItemFromList( mCurrSig.Get());
						mCurrSig = NULL;
						NoticeChange();
					}
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
void BmPrefsSignatureView::ShowSignature( int32 selection) {
	bool enabled = (selection != -1);
	mSignatureControl->SetEnabled( enabled);
	mContentControl->SetEnabled( enabled);
	mDynamicControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrSig = NULL;
		mSignatureControl->SetTextSilently( "");
		mContentControl->SetTextSilently( "");
		mDynamicControl->SetValue( 0);
		BmString charset( ThePrefs->GetString( "DefaultCharset"));
		mCharsetControl->ClearMark();
		mCharsetControl->MarkItem( charset.String());
		mCharsetControl->MenuItem()->SetLabel( charset.String());
		mTestButton->SetEnabled( false);
	} else {
		BmSignatureItem* sigItem = dynamic_cast<BmSignatureItem*>(mSigListView->ItemAt( selection));
		if (sigItem) {
			mCurrSig = sigItem->ModelItem();
			if (mCurrSig) {
				mSignatureControl->SetTextSilently( mCurrSig->Name().String());
				mContentControl->SetTextSilently( mCurrSig->Content().String());
				mDynamicControl->SetValue( mCurrSig->Dynamic());
				BmString charset( mCurrSig->Charset());
				mCharsetControl->ClearMark();
				mCharsetControl->MarkItem( charset.String());
				mCharsetControl->MenuItem()->SetLabel( charset.String());
				mCharsetControl->SetEnabled( mCurrSig->Dynamic());
				mTestButton->SetEnabled( mCurrSig->Dynamic());
			}
		} else
			mCurrSig = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsSignatureView::CreateSigListView( minimax minmax, int32 width, int32 height) {
	mSigListView = BmSignatureView::CreateInstance( minmax, width, height);
	mSigListView->ClickSetsFocus( true);
	return mSigListView->ContainerView();
}
