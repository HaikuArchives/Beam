/*
	BmBodyPartView.cpp
		$Id$
*/

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmBodyPartItem
\********************************************************************************/

int16 BmBodyPartItem::nUnnamedFileCounter = 0;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartItem::BmBodyPartItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item)
{
	BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( _item);

	BBitmap* icon = TheResources->IconByName( bodyPart->mContentType.mValue);
	SetColumnContent( 0, icon, 2.0, false);

	BString sizeString = BytesToString( bodyPart->mDecodedLength, true);
	BString fileName = bodyPart->mFileName;
	if (!fileName.Length()) {
		fileName = BString("unknown")<<++nUnnamedFileCounter;
	}

	// set column-values:
	BmListColumn cols[] = {
		{ fileName.String(),									false },
		{ bodyPart->mContentType.mValue.String(),		false },
		{ sizeString.String(),								true },
		{ bodyPart->mContentLanguage.String(),			false },
		{ bodyPart->mContentDescription.String(),		false },
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
BmBodyPartView::BmBodyPartView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_BodyPartView", 
					  B_SINGLE_SELECTION_LIST, true, false)
	,	mShowAllParts( false)
{
	SetViewColor( White);
	fSelectedItemColorWindowActive = 
	fSelectedItemColorWindowInactive = 
	fLightColumnCol = 						BeBackgroundGrey;

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
BmListViewItem* BmBodyPartView::CreateListViewItem( BmListModelItem* item, 
																	BMessage* archive) {
	BmBodyPart* modelItem = dynamic_cast<BmBodyPart*>( item);
	if (!modelItem->mIsMultiPart && (mShowAllParts || !modelItem->ShouldBeShownInline())) {
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
	AddAllModelItems()
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::AddAllModelItems() {
	// initializations:
	BmBodyPartItem::ResetUnnamedFileCounter();
	int i;
	for( i=nFirstTextCol; i<CountColumns(); ++i) {
		mColWidths[i] = nColWidths[i];
	}
	// do add all items:
	inherited::AddAllModelItems();
	// adjust size accordingly:
	float width = Bounds().Width();
	int32 count = FullListCountItems();
	float itemHeight = count ? ItemAt(0)->Height() : 0;
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
	if (mailView)
		mailView->CalculateVerticalOffset();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BString BmBodyPartView::StateInfoBasename()	{ 
	return "BodyPartView";
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	if (Parent())
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
			case BM_BODYPARTVIEW_SHOWINLINE: {
				mShowAllParts = false;
				ShowBody( dynamic_cast<BmBodyPartList*>( DataModel()));
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailHeaderView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBodyPartView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "HeaderViewMenu", false, false);

	BMenuItem* item = new BMenuItem( "Show All MIME-Bodies", 
												new BMessage( mShowAllParts
																  ? BM_BODYPARTVIEW_SHOWINLINE
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

