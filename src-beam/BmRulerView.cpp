/*
	BmRulerView.cpp
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

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmRulerView.h"
#include "BmMailView.h"
#include "BmPrefs.h"

static const char* const MEDIUM_WIDTH_CHAR = "0";

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
	SetViewColor( BeInactiveControlGrey);
	BFont font( be_plain_font);
	font.SetSize( 8);
	SetFont( &font);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRulerView::~BmRulerView() {
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
	int32 maxLineLen = ThePrefs->GetInt( "MaxLineLenForHardWrap", 998);
	float width = maxLineLen<100 
							? 1+maxLineLen*mSingleCharWidth	// stop at hard-wrap border
							: r.Width();							// draw all the way
	float step = mSingleCharWidth;
	
	// We only draw ruler lines if we have a fixed-width font, otherwise it just
	// does not make sense.
	if (mMailViewFont.IsFixed()) {
		// draw ruler lines:
		for( float x=0; x<width; x+=step) {
			if (static_cast<int>(x)%10 == 0)
				StrokeLine( BPoint(nXOffset+x, r.bottom-10), BPoint(nXOffset+x, r.bottom));
			else if (static_cast<int>(x)%5 == 0)
				StrokeLine( BPoint(nXOffset+x, r.bottom-7), BPoint(nXOffset+x, r.bottom));
			else
				StrokeLine( BPoint(nXOffset+x, r.bottom-5), BPoint(nXOffset+x, r.bottom));
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
		if (maxLineLen < 100) {
			// draw number over right margin;
			BmString numStr = BmString("") << maxLineLen;
			float w = StringWidth( numStr.String());
			DrawString( numStr.String(), BPoint( nXOffset+width-w/2, 8));
		}
	}

	// draw right-margin indicator:
	float xPos = nXOffset+mIndicatorPos*mSingleCharWidth;
	float yPos = r.bottom-10;
	if (!mIndicatorGrabbed)
		SetHighColor( LightMetallicBlue);
	else
		SetHighColor( MedMetallicBlue);
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
		BmString s = BmString("The right margin is currently fixed\nto a maximum of ") 
							<< maxLineLen 
							<< " characters.\n\nPlease check Preferences.";
		TheBubbleHelper.SetHelp( this, s.String());
	} else
		TheBubbleHelper.SetHelp( this, NULL);
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
void BmRulerView::MouseMoved( BPoint point, uint32 transit, const BMessage *msg) {
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
	int32 newPos = static_cast<int32>( (pixelPos+mSingleCharWidth/2) / mSingleCharWidth);
	SetIndicatorPos( newPos);
}

/*------------------------------------------------------------------------------*\
	SetIndicatorPos( pixelPos)
		-	
\*------------------------------------------------------------------------------*/
void BmRulerView::SetIndicatorPos( int32 newPos) {
	int32 maxPos = ThePrefs->GetInt( "MaxLineLenForHardWrap", 998);
	newPos = MAX( 1, MIN( newPos, maxPos));
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
