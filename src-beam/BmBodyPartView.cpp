/*
	BmBodyPartView.cpp
		$Id$
*/

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Roster.h>
#include <Window.h>

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
	COL_ICON = 0,
	COL_NAME,
	COL_MIMETYPE,
	COL_SIZE,
	COL_LANGUAGE,
	COL_DESCRIPTION
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

	BString sizeString = BytesToString( bodyPart->DecodedLength(), true);

	// set column-values:
	BmListColumn cols[] = {
		{ bodyPart->FileName().String(),					false },
		{ bodyPart->MimeType().String(),					false },
		{ sizeString.String(),								true },
		{ bodyPart->Language().String(),					false },
		{ bodyPart->Description().String(),				false },
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

const int16 BmBodyPartView::nFirstTextCol = 1;
float BmBodyPartView::nColWidths[10] = {20,100,100,70,70,200,0,0,0,0};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::BmBodyPartView( minimax minmax, int32 width, int32 height, 
										  bool editable)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_BodyPartView", 
					  B_SINGLE_SELECTION_LIST, true, false)
	,	mShowAllParts( false)
	,	mEditable( editable)
{
	SetViewColor( BeBackgroundGrey);
	fLightColumnCol = 						BeBackgroundGrey;
	if (!editable) {	
		fSelectedItemColorWindowActive = 
		fSelectedItemColorWindowInactive = BeBackgroundGrey;
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

	AddColumn( new CLVColumn( "Icon", 18.0, 0, 18.0));
	AddColumn( new CLVColumn( "Name", nColWidths[2], 0, 20.0));
	AddColumn( new CLVColumn( "Mimetype", nColWidths[1], 0, 20.0));
	AddColumn( new CLVColumn( "Size", nColWidths[3], CLV_RIGHT_JUSTIFIED, 20.0));
	AddColumn( new CLVColumn( "Language", nColWidths[4], 0, 20.0));
	AddColumn( new CLVColumn( "Description", nColWidths[5], 0, 20.0));

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartView::~BmBodyPartView() { 
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
	if (!modelItem->IsMultiPart() && (mShowAllParts || !modelItem->ShouldBeShownInline())) {
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
		StartJob( body, false);
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
	// adjust size accordingly:
	float width = Bounds().Width();
	int32 count = FullListCountItems();
	float itemHeight = count ? ItemAt(0)->Height()+1 : 0;
							// makes this view disappear if no BodyPart is shown
	ResizeTo( width, count*itemHeight);
	// update info about maximum column widths:
	for( int i=0; i<count; ++i) {
		BmListViewItem* viewItem = dynamic_cast<BmListViewItem*>( FullListItemAt( i));
		for( int c=nFirstTextCol; c<CountColumns(); ++c) {
			float textWidth = StringWidth( viewItem->GetColumnContentText(c));
			mColWidths[c] = MAX( mColWidths[c], textWidth+10);
		}
	}
	// adjust column widths, if neccessary:
	BmMailView* mailView = (BmMailView*)Parent();
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		ColumnAt( i)->SetWidth( mColWidths[i]);
	}
	if (mailView) {
		mailView->ScrollTo( 0, 0);
		mailView->CalculateVerticalOffset();
	}
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
	BPoint mousePos;
	uint32 buttons;
	GetMouse( &mousePos, &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
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
			case B_SIMPLE_DATA: {
				AddAttachment( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailHeaderView:\n\t") << err.what());
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
		Window()->UpdateIfNeeded();
		entry_ref eref = bodyPart->WriteToTempFile();
		status_t res = be_roster->Launch( &eref);
		if (res != B_OK && res != B_ALREADY_RUNNING) {
			ShowAlert( "Sorry, could not launch application for this attachment (unknown mimetype perhaps?)");
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
	const char* filename = bodyPartItem->GetColumnContentText( 1);
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

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
}

