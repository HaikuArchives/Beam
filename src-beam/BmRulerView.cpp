/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include "BubbleHelper.h"
#include "Colors.h"

#include "BmRulerView.h"
#include "BmMailView.h"
#include "BmPrefs.h"

static const char* const MEDIUM_WIDTH_CHAR = "0";

const char* const BmRulerView::MSG_NEW_POS = "bm:newpos";
const float BmRulerView::nXOffset = 4.0;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRulerView::BmRulerView( const BFont& font)
	:	inherited( BRect( 0, 0, 0, 19),
					  "RulerView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMailViewFont( font)
	,	mIndicatorPos( ThePrefs->GetInt( "MaxLineLen"))
	,	mIndicatorGrabbed( false)
	,	mSingleCharWidth( font.StringWidth( MEDIUM_WIDTH_CHAR))
{
	SetViewColor( BmWeakenColor( B_UI_DOCUMENT_BACKGROUND_COLOR, 2));
	BFont fnt( be_plain_font);
	fnt.SetSize( 8);
	SetFont( &fnt);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRulerView::~BmRulerView() {
	TheBubbleHelper->SetHelp( this, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::SetMailViewFont( const BFont& font) {
	mMailViewFont = font;
	mSingleCharWidth = mMailViewFont.StringWidth( MEDIUM_WIDTH_CHAR);
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::Draw( BRect bounds) {
	inherited::Draw( bounds);
	
	BRect r = Bounds();
	int32 maxLineLen = ThePrefs->GetBool( "NeverExceed78Chars", false)
								? 78
								: 0;
	float width 
		= maxLineLen
			? 1+maxLineLen*mSingleCharWidth
							// stop at hard-wrap border
			: r.Width();						
							// draw all the way
	float step = mSingleCharWidth;
	
	// We only draw ruler lines if we have a fixed-width font, otherwise it just
	// does not make sense.
	if (mMailViewFont.IsFixed()) {
		// draw ruler lines:
		for( float x=0; x<width; x+=step) {
			if (static_cast<int>(x)%10 == 0)
				StrokeLine( BPoint(nXOffset+x, r.bottom-10), 
								BPoint(nXOffset+x, r.bottom));
			else if (static_cast<int>(x)%5 == 0)
				StrokeLine( BPoint(nXOffset+x, r.bottom-7), 
								BPoint(nXOffset+x, r.bottom));
			else
				StrokeLine( BPoint(nXOffset+x, r.bottom-5), 
								BPoint(nXOffset+x, r.bottom));
		}
		
		SetLowColor( ViewColor());
		// draw ruler numbers:
		int num=0;
		for( float x=0; x<width; x+=step*10) {
			BmString numStr = BmString("") << num;
			num += 10;
			float w = StringWidth( numStr.String());
			DrawString( numStr.String(), BPoint( nXOffset+1+x-w/2, 8));
		}
		if (maxLineLen) {
			// draw number over right margin;
			BmString numStr = BmString("") << maxLineLen;
			float w = StringWidth( numStr.String());
			DrawString( numStr.String(), BPoint( nXOffset+width-w/2, 8));
		}
	}

	// draw right-margin indicator:
	float xPos = nXOffset+mIndicatorPos*mSingleCharWidth;
	float yPos = r.bottom-10;
	SetHighColor( 
		BmWeakenColor( B_UI_CONTROL_HIGHLIGHT_COLOR, mIndicatorGrabbed ? 0 : 2)
	);
	if (ThePrefs->GetBool( "HardWrapMailText")) {
		FillTriangle( BPoint( xPos-5, yPos),
						  BPoint( xPos+5, yPos),
						  BPoint( xPos, yPos+10));
	} else {
		StrokeTriangle( BPoint( xPos-5, yPos),
						    BPoint( xPos+5, yPos),
						    BPoint( xPos, yPos+10));
	}
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	if (Parent())
		Parent()->MakeFocus( true);
	int32 maxLineLen = ThePrefs->GetInt( "MaxLineLenForHardWrap", 998);
	if (maxLineLen < 100) {
		BmString s = BmString("The right margin is currently fixed\n"
									 "to a maximum of ") 
							<< maxLineLen 
							<< " characters.\n\nPlease check Preferences.";
		TheBubbleHelper->SetHelp( this, s.String());
	} else
		TheBubbleHelper->SetHelp( this, NULL);
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons) == B_OK 
	&& buttons == B_PRIMARY_MOUSE_BUTTON) {
		if (!mIndicatorGrabbed) {
			mIndicatorGrabbed = true;
			SetIndicatorPixelPos( point.x);
			SetMouseEventMask( B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		}
	}
}

/*------------------------------------------------------------------------------*\
	MouseMoved( point, transit, msg)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::MouseMoved( BPoint point, uint32 transit, 
										const BMessage *msg) {
	inherited::MouseMoved( point, transit, msg);
//	if (transit == B_INSIDE_VIEW || transit == B_ENTERED_VIEW) {
		if (mIndicatorGrabbed)
			SetIndicatorPixelPos( point.x);
//	}
}

/*------------------------------------------------------------------------------*\
	MouseUp( point)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::MouseUp( BPoint point) {
	inherited::MouseUp( point); 
	SetMouseEventMask( 0);
	if (mIndicatorGrabbed) {
		mIndicatorGrabbed = false;
		Invalidate();
	}
}

/*------------------------------------------------------------------------------*\
	SetIndicatorPixelPos( pixelPos)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::SetIndicatorPixelPos( float pixelPos) {
	pixelPos -= nXOffset;
	int32 newPos = static_cast<int32>( 
		(pixelPos+mSingleCharWidth/2) / mSingleCharWidth
	);
	SetIndicatorPos( newPos);
}

/*------------------------------------------------------------------------------*\
	SetIndicatorPos( pixelPos)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::SetIndicatorPos( int32 newPos) {
	if (newPos > 78 && ThePrefs->GetBool( "NeverExceed78Chars", false))
		newPos = 78;
	if (newPos < 1)
		newPos = 1;
	if (newPos != mIndicatorPos) {
		mIndicatorPos = newPos;
		Invalidate();
		BView* parent = Parent();
		if (parent && Looper()) {
			BMessage msg( BM_RULERVIEW_NEW_POS);
			msg.AddInt32( MSG_NEW_POS, newPos);
			Looper()->PostMessage( &msg, parent);
		}
	}
}
