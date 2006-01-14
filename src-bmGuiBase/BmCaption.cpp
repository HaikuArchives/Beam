/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "Colors.h"

#include <cstdio>

#include "BmBasics.h"
#include "BmCaption.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::BmCaption( BRect frame, const char* text) 
	:	inherited( frame, NULL, B_FOLLOW_NONE, B_WILL_DRAW)
	,	mText(text)
	,	mHighlight(false)
{
	SetViewColor( B_TRANSPARENT_COLOR);
	SetLowUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	BFont captionFont( *be_plain_font);
	float captionFontSize = be_plain_font->Size();
	if (captionFontSize > 12)
		captionFontSize = 12;
	captionFont.SetSize( captionFontSize);
	SetFont( &captionFont);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::~BmCaption() {
}

/*------------------------------------------------------------------------------*\
	SetText( )
		-	
\*------------------------------------------------------------------------------*/
void BmCaption::SetText( const char* txt)
{
	mText = txt; 
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	SetHighlight( )
		-	
\*------------------------------------------------------------------------------*/
void BmCaption::SetHighlight( bool highlight, const char* label)
{
	mHighlight = highlight;
	mHighlightLabel = label;
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCaption::Draw( BRect updateRect) {
	BRect r = Bounds();
	if (mHighlight) {
		if (BeamOnDano)
			SetLowColor( keyboard_navigation_color());
		else {
			rgb_color highlightCol = {255, 217, 121, 255};
			SetLowColor( highlightCol);
		}
	}
	else
		SetLowColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
	FillRect( r.InsetByCopy(1.0, 1.0), B_SOLID_LOW);

	SetHighColor( ui_color( B_UI_SHINE_COLOR));
	StrokeLine( BPoint(0.0,1.0), BPoint(r.right-1,1.0));
	StrokeLine( BPoint(0.0,1.0), r.LeftBottom());
	SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
	if (BeamOnDano)
		StrokeLine( r.RightTop(), r.RightBottom());
	else
		// looks better on R5, as it blends with scrollbar:
		StrokeLine( r.RightTop(), r.RightBottom(), B_SOLID_LOW);
	StrokeLine( r.LeftTop(), r.RightTop());
	StrokeLine( r.LeftBottom(), r.RightBottom());

	SetHighColor( ui_color( B_UI_PANEL_TEXT_COLOR));
	font_height fInfo;
	BFont captionFont;
	GetFont(&captionFont);
	captionFont.GetHeight( &fInfo);
	float offset = (1+r.Height()-(fInfo.ascent+fInfo.descent))/2.0;
	float freeWidth = r.Width();
	if (mHighlight && mHighlightLabel.Length()) {
		freeWidth -= StringWidth(mHighlightLabel.String())+2;
		BPoint pos( 2.0, fInfo.ascent+offset);
		DrawString( mHighlightLabel.String(), pos);
	}
	const char* text = mText.String();
	float width;
	while(1) {
		width = StringWidth(text);
		if (width+4.0 < freeWidth)
			break;
		text++;
		while((*text & 0xc0) == 0x80)
			text++;		// skip UTF8 subsequence chars
		if (!*text)
			break;
	}
	BPoint pos( r.Width()-width-2.0, fInfo.ascent+offset);
	DrawString( text, pos);
}
