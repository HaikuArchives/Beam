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

#include <layout-all.h>

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
BmSignatureItem::BmSignatureItem( BString key, BmListModelItem* _item)
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
		BmListColumn cols[] = {
			{ sig->Key().String(),						false },
			{ sig->Dynamic() ? "*" : "",				false },
			{ sig->Content().String(),					false },
			{ NULL, false }
		};
		SetTextCols( 0, cols, !ThePrefs->GetBool("StripedListView"));
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
																		BMessage* archive) {
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
		BM_SHOWERR( BString("SignatureView:\n\t") << err.what());
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
			CreateSigListView( minimax(500,60,1E5,1E5), 500, 150),
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
					mContentControl = new BmMultiLineTextControl( "Content:", false),
					new Space( minimax(0,5,0,5)),
					new HGroup( 
						mDynamicControl = new BmCheckControl( "Content is dynamic (execute the given command via shell, fetch stdout as signature)", 
																		  new BMessage(BM_DYNAMIC_CHANGED), 
																		  this),
						new Space(),
						mTestButton = new MButton( "Test this signature", new BMessage( BM_TEST_SIGNATURE), this, minimax(-1,-1,-1,-1)),
						0
					),
					mCharsetControl = new BmMenuControl( "Charset:", new BPopUpMenu("")),
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
			new Space(minimax(0,0,1E5,1E5,0.5)),
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
	for( int i=0; BM_Encodings[i].charset; ++i) {
		AddItemToMenu( mCharsetControl->Menu(), 
							new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_CHARSET_SELECTED)), this);
	}
	// mark default charset:
	BMenuItem* item;
	item = mCharsetControl->Menu()->FindItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
	if (item)
		item->SetMarked( true);

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
					if ( source == mContentControl)
						mCurrSig->Content( mContentControl->Text());
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
				break;
			}
			case BM_DYNAMIC_CHANGED: {
				bool val = mDynamicControl->Value();
				if (mCurrSig)
					mCurrSig->Dynamic( val);
				mCharsetControl->SetEnabled( val);
				mTestButton->SetEnabled( val);
				break;
			}
			case BM_CHARSET_SELECTED: {
				if (mCurrSig) {
					BMenuItem* item = mCharsetControl->Menu()->FindMarked();
					if (item)
						mCurrSig->Encoding( CharsetToEncoding( item->Label()));
					else
						mCurrSig->Encoding( ThePrefs->GetInt( "DefaultEncoding"));
				}
				break;
			}
			case BM_TEST_SIGNATURE: {
				if (mCurrSig) {
					BString sigString = mCurrSig->GetSignatureString();
					if (sigString.Length()) {
						BAlert* alert = new BAlert( "Signature-Test", 
														 (BString("Please check the results below.\n-- \n")<<sigString).String(),
													 	 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
													 	 B_INFO_ALERT);
						alert->SetShortcut( 0, B_ESCAPE);
						alert->Go();
					}
				}
				break;
			}
			case BM_ADD_SIGNATURE: {
				BString key( "new signature");
				for( int32 i=1; TheSignatureList->FindItemByKey( key); ++i) {
					key = BString("new signature_")<<i;
				}
				TheSignatureList->AddItemToList( new BmSignature( key.String(), TheSignatureList.Get()));
				mSignatureControl->MakeFocus( true);
				mSignatureControl->TextView()->SelectAll();
				break;
			}
			case BM_REMOVE_SIGNATURE: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Signature", 
														 (BString("Are you sure about removing the signature <") << mCurrSig->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_SIGNATURE), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheSignatureList->RemoveItemFromList( mCurrSig.Get());
						mCurrSig = NULL;
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
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
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
		mCharsetControl->SetEnabled( false);
		mCharsetControl->MarkItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
		mTestButton->SetEnabled( false);
	} else {
		BmSignatureItem* sigItem = dynamic_cast<BmSignatureItem*>(mSigListView->ItemAt( selection));
		if (sigItem) {
			mCurrSig = sigItem->ModelItem();
			if (mCurrSig) {
				mSignatureControl->SetTextSilently( mCurrSig->Name().String());
				mContentControl->SetTextSilently( mCurrSig->Content().String());
				mDynamicControl->SetValue( mCurrSig->Dynamic());
				mCharsetControl->MarkItem( EncodingToCharset( mCurrSig->Encoding()).String());
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
	return mSigListView->ContainerView();
}
