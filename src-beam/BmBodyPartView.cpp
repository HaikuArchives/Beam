/*
	BmBodyPartView.cpp
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
#include <FilePanel.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmMailView.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmBodyPartItem
\********************************************************************************/

enum Columns {
	COL_EXPANDER = 0,
	COL_ICON,
	COL_NAME,
	COL_MIMETYPE,
	COL_SIZE,
	COL_DESCRIPTION,
	COL_TRANSFER_ENC,
	COL_LANGUAGE
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartItem::BmBodyPartItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item)
{
	BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( _item);

	BBitmap* icon = TheResources->IconByName( bodyPart->MimeType());
	SetColumnContent( COL_ICON, icon, 2.0, false);

	BString sizeString = bodyPart->IsMultiPart() 
								? "" 
								: BytesToString( bodyPart->DecodedLength(), true);

	// set column-values:
	BmListColumn cols[] = {
		{ bodyPart->FileName().String(),					false },
		{ bodyPart->MimeType().String(),					false },
		{ sizeString.String(),								true },
		{ bodyPart->Description().String(),				false },
		{ bodyPart->TransferEncoding().String(),		false },
		{ bodyPart->Language().String(),					false },
		{ NULL, false }
	};
	SetTextCols( BmBodyPartView::nFirstTextCol, cols, false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartItem::~BmBodyPartItem() { 
}



/********************************************************************************\
	BmBodyPartView
\********************************************************************************/
#define BACKGROUND_COL BeBackgroundGrey

const int16 BmBodyPartView::nFirstTextCol = 2;
float BmBodyPartView::nColWidths[10] = {10,20,20,20,20,20,20,20,0,0};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::BmBodyPartView( minimax minmax, int32 width, int32 height, 
										  bool editable)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_BodyPartView", 
					  B_SINGLE_SELECTION_LIST, true, true)
	,	mShowAllParts( false)
	,	mEditable( editable)
	,	mSavePanel( NULL)
	,	mIsUsedForPrinting( false)
{
	SetViewColor( BACKGROUND_COL);
	fLightColumnCol = BACKGROUND_COL;
	if (!editable) {	
		fSelectedItemColorWindowActive = 
		fSelectedItemColorWindowInactive = BACKGROUND_COL;
	} else {
		fSelectedItemColorWindowActive = 
		fSelectedItemColorWindowInactive = BeShadow;
	}

	Initialize( BRect(0,0,800,40), B_WILL_DRAW | B_FRAME_EVENTS,
					B_FOLLOW_NONE, false, false, false, B_FANCY_BORDER);
	SetResizingMode( B_FOLLOW_NONE);
	int32 flags = Flags();
	flags &= (B_NAVIGABLE^0xFFFFFFFF);
	SetFlags( flags);

	UseStateCache( false);

	AddColumn( new CLVColumn( NULL, nColWidths[0], 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 10.0));
	AddColumn( new CLVColumn( "Icon", nColWidths[1], CLV_PUSH_PASS, 18.0));
	AddColumn( new CLVColumn( "Name", nColWidths[2], 0, 20.0));
	AddColumn( new CLVColumn( "Mimetype", nColWidths[3], 0, 20.0));
	AddColumn( new CLVColumn( "Size", nColWidths[4], CLV_RIGHT_JUSTIFIED, 20.0));
	AddColumn( new CLVColumn( "Description", nColWidths[5], 0, 20.0));
	AddColumn( new CLVColumn( "Transfer", nColWidths[6], 0, 20.0));
	AddColumn( new CLVColumn( "", nColWidths[7], 0, 20.0));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::~BmBodyPartView() { 
	delete mSavePanel;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmBodyPartView::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddBool( MSG_SHOWALL, mShowAllParts);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmBodyPartView::Unarchive( BMessage* archive, bool deep=true) {
	status_t ret = archive->FindBool( MSG_SHOWALL, &mShowAllParts);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmBodyPartView::CreateListViewItem( BmListModelItem* item, 
																	BMessage* archive) {
	BmBodyPart* modelItem = dynamic_cast<BmBodyPart*>( item);
	if (mShowAllParts || !modelItem->IsMultiPart() && !modelItem->ShouldBeShownInline()) {
		return new BmBodyPartItem( item->Key(), item);
	} else {
		return NULL;
	}
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmBodyPartView::CreateContainer( bool horizontal, bool vertical, 
												  					bool scroll_view_corner, 
												  					border_style border, 
																	uint32 ResizingMode, 
																	uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, false, 
											 false, false, border, false, false);
}

/*------------------------------------------------------------------------------*\
	ShowBody()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ShowBody( BmBodyPartList* body) {
	try {
		StopJob();
		if (!body) {
			ResizeTo( Bounds().Width(), 0);
			MoveTo( 0, 0);
			SetViewColor( White);
		} else {
			SetViewColor( BACKGROUND_COL);
			StartJob( body, !mIsUsedForPrinting);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BodyPartView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddAttachment()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAttachment( BMessage* msg) {
	if (!mEditable)
		return;
	entry_ref ref;
	BmBodyPartList* bodyPartList = dynamic_cast<BmBodyPartList*>( DataModel());
	if (!bodyPartList)
		return;
	for( int i=0; msg->FindRef( "refs", i, &ref) == B_OK; ++i) {
		bodyPartList->AddAttachmentFromRef( &ref);
	}
	ShowBody( bodyPartList);
}

/*------------------------------------------------------------------------------*\
	AdjustVerticalSize()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AdjustVerticalSize() {
	float width = Bounds().Width();
	int32 count = CountItems();
	float itemHeight = count ? ItemAt(0)->Height()+1 : 0;
							// makes this view disappear if no BodyPart is shown
	ResizeTo( width, count*itemHeight);
	Invalidate();
	BmMailView* mailView = (BmMailView*)Parent();
	if (mailView) {
		mailView->ScrollTo( 0, 0);
		mailView->CalculateVerticalOffset();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ExpansionChanged( CLVListItem* _item, bool expanded) {
	inherited::ExpansionChanged( _item, expanded);
	AdjustVerticalSize();
}

/*------------------------------------------------------------------------------*\
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAllModelItems() {
	// initializations:
	int i;
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		mColWidths[i] = nColWidths[i];
	}
	// do add all items:
	inherited::AddAllModelItems();
	// update info about maximum column widths:
	int32 count = FullListCountItems();
	for( int i=0; i<count; ++i) {
		BmListViewItem* viewItem = dynamic_cast<BmListViewItem*>( FullListItemAt( i));
		for( int c=nFirstTextCol; c<CountColumns(); ++c) {
			float textWidth = StringWidth( viewItem->GetColumnContentText(c));
			mColWidths[c] = MAX( mColWidths[c], 
										textWidth+10+EXPANDER_SHIFT*viewItem->OutlineLevel());
		}
		Expand(FullListItemAt(i));
	}
	// adjust column widths, if neccessary:
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		ColumnAt( i)->SetWidth( mColWidths[i]);
	}

	AdjustVerticalSize();
}

/*------------------------------------------------------------------------------*\
	RemoveModelItem( msg)
		-	we 
\*------------------------------------------------------------------------------*/
void BmBodyPartView::RemoveModelItem( BmListModelItem* item) {
	BM_LOG2( BM_LogModelController, BString(ControllerName())<<": removing one item from listview");
	inherited::RemoveModelItem( item);
	BmBodyPartList* bodyPartList = dynamic_cast<BmBodyPartList*>( DataModel());
	ShowBody( bodyPartList);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BString BmBodyPartView::StateInfoBasename()	{ 
	return "BodyPartView";
}

/*------------------------------------------------------------------------------*\
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::KeyDown(const char *bytes, int32 numBytes) { 
	if ( numBytes == 1 && mEditable) {
		switch( bytes[0]) {
			case B_DELETE: {
				int32 idx = CurrentSelection(0);
				if (idx >= 0) {
					BmBodyPartList* body = dynamic_cast<BmBodyPartList*>( DataModel());
					BmBodyPartItem* bodyItem = dynamic_cast<BmBodyPartItem*>(ItemAt( idx));
					body->RemoveItemFromList( dynamic_cast<BmBodyPart*>( bodyItem->ModelItem()));
				}				
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
	} else 
		inherited::KeyDown( bytes, numBytes);
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	if (!mEditable && Parent())
		Parent()->MakeFocus( true);
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons)==B_OK 
	&& !mEditable && buttons == B_SECONDARY_MOUSE_BUTTON) {
		int32 clickIndex = IndexOf( point);
		if (clickIndex >= 0) {
			Select( clickIndex);
		} else 
			DeselectAll();
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_BODYPARTVIEW_SHOWALL: {
				mShowAllParts = true;
				ShowBody( dynamic_cast<BmBodyPartList*>( DataModel()));
				break;
			}
			case BM_BODYPARTVIEW_SHOWATTACHMENTS: {
				mShowAllParts = false;
				ShowBody( dynamic_cast<BmBodyPartList*>( DataModel()));
				break;
			}
			case BM_BODYPARTVIEW_SAVE_ATTACHMENT: {
				int32 index = CurrentSelection( 0);
				if (index < 0)
					break;
				BmBodyPartItem* bodyPartItem = dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
				if (!bodyPartItem)
					break;
				BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( bodyPartItem->ModelItem());
				if (!bodyPart)
					break;
				// first step, let user select filename to save under:
				if (!mSavePanel) {
					mSavePanel = new BFilePanel( B_SAVE_PANEL, new BMessenger(this), NULL,
														  B_FILE_NODE, false);
				}
				mSavePanel->SetSaveText( bodyPart->FileName().String());
				mSavePanel->Show();
				break;
			}
			case B_CANCEL: {
				// since a SavePanel seems to avoid quitting, thus stopping Beam from proper exit,
				// we detroy the panel:
				delete mSavePanel;
				mSavePanel = NULL;
				break;
			}
			case B_SAVE_REQUESTED: {
				// since a SavePanel seems to avoid quitting, thus stopping Beam from proper exit,
				// we detroy the panel:
				delete mSavePanel;
				mSavePanel = NULL;
				int32 index = CurrentSelection( 0);
				if (index < 0)
					break;
				BmBodyPartItem* bodyPartItem = dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
				if (!bodyPartItem)
					break;
				BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( bodyPartItem->ModelItem());
				if (!bodyPart)
					break;
				entry_ref destDirRef;
				if (msg->FindRef( "directory", 0, &destDirRef) == B_OK) {
					BString name = msg->FindString( "name");
					bodyPart->SaveAs( destDirRef, name);
				}
				break;
			}
			case B_SIMPLE_DATA: {
				AddAttachment( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BodyPartView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ItemInvoked( index)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ItemInvoked( int32 index) {
	BmBodyPartItem* bodyPartItem = dynamic_cast<BmBodyPartItem*>( FullListItemAt( index));
	if (bodyPartItem) {
		BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( bodyPartItem->ModelItem());
		if (!bodyPart)
			return;
		bodyPartItem->Highlight( true);
		InvalidateItem( index);
		Window()->UpdateIfNeeded();		// for immediate highlight
		entry_ref eref = bodyPart->WriteToTempFile();
		if (bmApp->HandlesMimetype( bodyPart->MimeType())) {
			BMessage msg( B_REFS_RECEIVED);
			msg.AddRef( "refs", &eref);
			bmApp->RefsReceived( &msg);
		} else {
			BString mimetype = bodyPart->MimeType();
			BString mtTrustInfo = ThePrefs->GetString( "MimeTypeTrustInfo", 
																	 "<application/pdf:T><application:W><:T>");
			bool doIt = true;
			Regexx rx;
			int count = rx.exec( mtTrustInfo, "<(.*?):(\\w)>", Regexx::global);
			for( int i=0; i<count; ++i) {
				BString match = rx.match[i].atom[0];
				if ( mimetype.ICompare( match, match.Length()) == 0) {
					BString trustLevel = rx.match[i].atom[1];
					if (trustLevel.ICompare( "T") != 0) {
						BString s("The attachment may contain harmful content, are you sure to open it?");
						BAlert* alert = new BAlert( "", s.String(), "Yes", "No");
						alert->SetShortcut( 1, B_ESCAPE);
						doIt = (alert->Go() == 0);
					}
					break;
				}
			}
			if (doIt) {
				BEntry entry( &eref);
				BPath path;
				entry.GetPath( &path);
				status_t res=entry.InitCheck();
				if (res==B_OK && path.InitCheck()==B_OK && path.Path()) {
					update_mime_info( path.Path(), false, true, false);
					res = be_roster->Launch( &eref);
					if (res != B_OK && res != B_ALREADY_RUNNING) {
						ShowAlert( BString("Sorry, could not launch application for this attachment (unknown mimetype perhaps?)\n\nError: ") << strerror(res));
					}
				} else
					ShowAlert( BString("Sorry, could not open this attachment.\n\nError: ") << strerror(res));
			}
		}
		bodyPartItem->Highlight( false);
		InvalidateItem( index);
	}
}

/*------------------------------------------------------------------------------*\
	InitiateDrag()
		-	
\*------------------------------------------------------------------------------*/
bool BmBodyPartView::InitiateDrag( BPoint where, int32 index, bool wasSelected) {
	if (!wasSelected)
		return false;
	BMessage dragMsg( B_SIMPLE_DATA);
	BmBodyPartItem* bodyPartItem = dynamic_cast<BmBodyPartItem*>(ItemAt( index));
	BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>(bodyPartItem->ModelItem());
	const char* filename = bodyPartItem->GetColumnContentText( COL_NAME);
	entry_ref eref = bodyPart->WriteToTempFile( filename);
	dragMsg.AddInt32( "be:actions", B_MOVE_TARGET);
	dragMsg.AddString( "be:clip_name", filename);
	dragMsg.AddRef( "refs", &eref);
	BFont font;
	GetFont( &font);
	float lineHeight = MAX(TheResources->FontLineHeight( &font),20.0);
	float baselineOffset = TheResources->FontBaselineOffset( &font);
	BRect dragRect( 0, 0, 200-1, 1*lineHeight-1);
	BView* dummyView = new BView( dragRect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* dragImage = new BBitmap( dragRect, B_RGBA32, true);
	dragImage->AddChild( dummyView);
	dragImage->Lock();
	dummyView->SetHighColor( B_TRANSPARENT_COLOR);
	dummyView->FillRect( dragRect);
	dummyView->SetDrawingMode( B_OP_ALPHA);
	dummyView->SetHighColor( 0, 0, 0, 128);
	dummyView->SetBlendingMode( B_CONSTANT_ALPHA, B_ALPHA_COMPOSITE);
	// now we add the selected item to drag-image and to drag-msg:
	const BBitmap* icon = bodyPartItem->GetColumnContentBitmap( 0);
	if (icon) {
		dummyView->DrawBitmapAsync( icon, BPoint(0,0));
	}
	dummyView->DrawString( filename, BPoint(20.0,baselineOffset));
	dragImage->Unlock();
	DragMessage( &dragMsg, dragImage, B_OP_ALPHA, BPoint( 10.0, 10.0));
	return true;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "BodyPartViewMenu", false, false);

	BMenuItem* item = new BMenuItem( "Show All MIME-Bodies", 
												new BMessage( mShowAllParts
																  ? BM_BODYPARTVIEW_SHOWATTACHMENTS
																  : BM_BODYPARTVIEW_SHOWALL));
	item->SetTarget( this);
	item->SetMarked( mShowAllParts);
	theMenu->AddItem( item);

	theMenu->AddSeparatorItem();
	item = new BMenuItem( "Save Attachment As...", 
								 new BMessage( BM_BODYPARTVIEW_SAVE_ATTACHMENT));
	item->SetTarget( this);
	theMenu->AddItem( item);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
}

