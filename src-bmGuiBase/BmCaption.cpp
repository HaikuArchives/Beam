/*
	BmCaption.cpp
		$Id$
*/

#include "Colors.h"

#include "BmCaption.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::BmCaption( BRect frame, const char* text) 
	:	inherited( frame, NULL, text)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::~BmCaption() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCaption::Draw( BRect bounds) {
	BRect rect = Bounds();
	SetHighColor( tint_color(BeBackgroundGrey, 1.072F));
	FillRect( rect);
	SetHighColor( White);
	StrokeRect( BRect(1.0,1.0,rect.right,rect.bottom));
	SetHighColor( BeShadow);
	StrokeRect( rect);
	SetLowColor( BeLightShadow);
	SetHighColor( Black);
	font_height fInfo;
	be_plain_font->GetHeight( &fInfo);
	float offset = (1+rect.Height()-(fInfo.ascent+fInfo.descent))/2.0;
	float width = be_plain_font->StringWidth( Text());
	BPoint pos( rect.Width()-width-2.0, fInfo.ascent+offset);
	DrawString( Text(), pos);
}
