/*
	BmMailHeaderView.cpp
		$Id$
*/

#include <MenuItem.h>
#include <PopUpMenu.h>

#include "Colors.h"

#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmMailHeaderView.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderView( BmMailHeader* header)
	:	inherited( BRect( 0, 0, 0, 0),
					  "MailHeaderView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMailHeader( NULL)
	,	mDisplayMode( LARGE_HEADERS)
	,	mFont( new BFont( be_bold_font))
{
	SetViewColor( B_TRANSPARENT_COLOR);
	ShowHeader( header);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::~BmMailHeaderView() {
	delete mFont;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddInt16( MSG_MODE, mDisplayMode);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Unarchive( BMessage* archive, bool deep=true) {
	status_t ret = archive->FindInt16( MSG_MODE, &mDisplayMode);
	return ret;
}

/*------------------------------------------------------------------------------*\
	ShowHeader()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::ShowHeader( BmMailHeader* header) {
	mMailHeader = header;
	float height = 0;
	if (header) {
		int numLines = 0;
		switch (mDisplayMode) {
			case SMALL_HEADERS: {
				// small mode, 3 fields are displayed:
				numLines = 3;
				break;
			}
			case FULL_HEADERS: {
				// full mode, ALL fields are displayed:
				numLines = mMailHeader->NumLines();
				break;
			}
			default: {
				// large mode, 5 fields are displayed:
				numLines = 5;
				break;
			}
		};
		height = 6+(TheResources->FontLineHeight( mFont)+3)*numLines;
		ResizeTo( Bounds().Width(), height);
		BmMailView* mailView = (BmMailView*)Parent();
		if (mailView)
			mailView->SetVerticalOffset( height);
	}
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::Draw( BRect bounds) {
	inherited::Draw( bounds);
	
	BRect r = Bounds();

	if (!mMailHeader) {
		SetHighColor( White);
		FillRect( r);
		return;
	}

	float fh = TheResources->FontHeight( mFont);
	float lh = TheResources->FontLineHeight( mFont)+3;

	const char* titles[] = {
		"Subject:",
		"From:",
		"Date:",
		"To:",
		"Cc:",
		NULL
	};

	mFont->SetFace( B_BOLD_FACE);
	SetFont( mFont);
	float titleWidth = 0.0;
	if (mDisplayMode == FULL_HEADERS) {
		// determine widest field name:
		int numLines = mMailHeader->NumLines();
		const char *start = mMailHeader->HeaderString().String();
		for( int l=0; l<numLines; l++) {
			const char* end = strchr( start, '\n');
			if (!end)
				break;
			if (*start!=' ' && *start!='\t') {
				char* colonPos = strchr( start, ':');
				if (!colonPos)
					break;
				BString field( start, colonPos-start);
				field << ":";
				float w = StringWidth( field.String()) + 20;
				if (w > titleWidth)
					titleWidth = w;
			}
			start = end+1;
		}
	} else {
		for( int i=0; titles[i]; ++i) {
			float w = StringWidth( titles[i]) + 20;
			if (w > titleWidth)
				titleWidth = w;
		}
	}

	SetHighColor( BeListSelectGrey);
	FillRect( BRect(0, 0, titleWidth, r.Height()-1));

	SetHighColor( BeLightShadow);
	FillRect( BRect(0+titleWidth, 0, r.Width(), r.Height()-1));

	SetHighColor( BeShadow);
	StrokeLine( BPoint(0,r.Height()), BPoint( r.Width(), r.Height()));

	float x_off = 5.0;
	float y_off = 2.0;
	if (mDisplayMode != FULL_HEADERS) {
		// small or large mode, display 3 or 5 fields:
		SetHighColor( Black);
		SetLowColor( BeListSelectGrey);
		int numShown = mDisplayMode == 0 ? 3 : 5;
		for( int32 l=0; l<numShown; ++l) {
			DrawString( titles[l], BPoint( titleWidth-StringWidth(titles[l])-x_off, y_off+l*lh+fh) );
		}
		mFont->SetFace( B_REGULAR_FACE);
		SetFont( mFont);
		SetLowColor( BeLightShadow);
		DrawString( mMailHeader->GetFieldVal( "Subject").String(), BPoint( titleWidth+x_off, y_off+0*lh+fh) );
		DrawString( mMailHeader->GetEnhancedFieldVal( "From").String(), BPoint( titleWidth+x_off, y_off+1*lh+fh) );
		DrawString( mMailHeader->GetEnhancedFieldVal( "Date").String(), BPoint( titleWidth+x_off, y_off+2*lh+fh) );
		if (mDisplayMode == LARGE_HEADERS) {
			DrawString( mMailHeader->GetEnhancedFieldVal( "To").String(), BPoint( titleWidth+x_off, y_off+3*lh+fh) );
			DrawString( mMailHeader->GetEnhancedFieldVal( "Cc").String(), BPoint( titleWidth+x_off, y_off+4*lh+fh) );
		}
	} else {
		// full mode, display complete header:
		SetHighColor( Black);
		SetLowColor( BeLightShadow);
		int numLines = mMailHeader->NumLines();
		const char *start = mMailHeader->HeaderString().String();
		for( int l=0; l<numLines; l++) {
			const char* end = strchr( start, '\n');
			if (!end)
				break;
			if (*start!=' ' && *start!='\t') {
				char* colonPos = strchr( start, ':');
				if (!colonPos)
					break;
				BString field( start, colonPos-start);
				field << ":";
				mFont->SetFace( B_BOLD_FACE);
				SetFont( mFont);
				DrawString( field.String(), BPoint( titleWidth-StringWidth(field.String())-x_off, y_off+l*lh+fh) );
				start = colonPos+1;
			}
			for( ; start<end-1 && (*start==' ' || *start=='\t'); ++start)
				;
			BString line( start, end-start-1);
			start = end+1;
			mFont->SetFace( B_REGULAR_FACE);
			SetFont( mFont);
			DrawString( line.String(), BPoint( titleWidth+x_off, y_off+l*lh+fh) );
		}
	}
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::MouseDown( BPoint point) {
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
void BmMailHeaderView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_HEADERVIEW_SMALL: {
				mDisplayMode = 0;
				ShowHeader( mMailHeader);
				break;
			}
			case BM_HEADERVIEW_LARGE: {
				mDisplayMode = 1;
				ShowHeader( mMailHeader);
				break;
			}
			case BM_HEADERVIEW_FULL: {
				mDisplayMode = 2;
				ShowHeader( mMailHeader);
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
void BmMailHeaderView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "HeaderViewMenu", false, false);

	BMenuItem* item = new BMenuItem( "Small Header (Cooked)", new BMessage( BM_HEADERVIEW_SMALL));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == SMALL_HEADERS);
	theMenu->AddItem( item);
	item = new BMenuItem( "Large Header (Cooked)", new BMessage( BM_HEADERVIEW_LARGE));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == LARGE_HEADERS);
	theMenu->AddItem( item);
	item = new BMenuItem( "Full Header (Raw)", new BMessage( BM_HEADERVIEW_FULL));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == FULL_HEADERS);
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

