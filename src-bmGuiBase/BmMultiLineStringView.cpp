/*
	BmMultiLineStringView.cpp
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


#include <stdio.h>

#include "BmMultiLineStringView.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineStringView::BmMultiLineStringView( BRect frame, const char *name,
															 const char* text, 
															 uint32 resizingMode,
															 uint32 flags)
	:	inherited(frame, name, resizingMode, flags)
	,	mText(text)
	,	mIsSelectable(true)
{
	_InitFont();
	_InitText();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineStringView::~BmMultiLineStringView()
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::SetText(const char* text)
{
	mText.SetTo(text);
	_InitText();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::SetFont(const BFont *font, uint32 properties)
{
	inherited::SetFont(font, properties);
	_InitFont();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMultiLineStringView::LineHeight()
{
	return mLineHeight;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMultiLineStringView::TextHeight()
{
	return mLineHeight * mTextLines.CountItems();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::Draw(BRect updateRect)
{
	inherited::Draw(updateRect);
	BPoint pt = mDrawPT;
	BmString line;
	for(int32 i=0; i<mTextLines.CountItems(); ++i) {
		MovePenTo(pt);
		TextLine* textLine = static_cast<TextLine*>(mTextLines.ItemAt(i));
		line.SetTo(textLine->text, textLine->len);
		DrawString(line.String());
		pt.y += mLineHeight;
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_InitFont()
{
	struct font_height fh;
	GetFontHeight(&fh);
	mLineHeight = ceil(fh.ascent+fh.descent+fh.leading);
	mDrawPT.Set(3.0, ceil(fh.ascent)+1.0);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_InitText()
{
	int32 count = mTextLines.CountItems();
	while(count)
		delete static_cast<TextLine*>(mTextLines.RemoveItem(--count));

	// split the buffer into separate text lines, putting the resulting 
	// char-pointers (with their lengths) into mTextLines:
	const char* lastPos = mText.String();
	const char* pos;
	while( (pos = strchr(lastPos, '\n')) != NULL) {
		mTextLines.AddItem(new TextLine(lastPos, pos-lastPos));
		lastPos = pos + 1;
	}
	mTextLines.AddItem(new TextLine(lastPos, strlen(lastPos)));
}	
