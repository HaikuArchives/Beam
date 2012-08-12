/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <MenuField.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <MWindow.h>

#include "Colors.h"

#include "BmString.h"

#include "BmMultiLineTextControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineTextControl::BmMultiLineTextControl( const char* label, bool labelIsMenu, 
															   int32 lineCount, int32 minTextLen,
															   bool fixedHeight)
	:	inherited( BRect(0,0,0,0), NULL, label, true, NULL, NULL, B_FOLLOW_NONE)
	,	mLabelIsMenu( labelIsMenu)
{
	ResizeToPreferred();
	float divPos = 0;
	if (label)
		divPos = StringWidth(label) + (mLabelIsMenu ? 27 : 3);
	BFont font;
	m_text_view->GetFont( &font);
	font_height fh;
	font.GetHeight( &fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading; 
	float height = lineHeight*float(lineCount)+12.0f;
	if (minTextLen)
		ct_mpm = minimax( int(divPos + font.StringWidth("W")*float(minTextLen)), 
								int(height+4), 100000, 
								fixedHeight ? int(height+4) : 100000);
	else
		ct_mpm = minimax( int(divPos + font.StringWidth("W")*10), 
								int(height+4), 100000, fixedHeight 
								? int(height+4) : 100000);
	inherited::SetDivider( divPos);
	if (labelIsMenu) {
		float width, height;
		GetPreferredSize( &width, &height);
		BPopUpMenu* popup = new BPopUpMenu( label, true, false);
		mMenuField = new BMenuField( BRect( 2,0,Divider(),height), NULL, label, 
											  popup, true, B_FOLLOW_NONE, B_WILL_DRAW);
		mMenuField->SetDivider( 0);
		AddChild( mMenuField);
	}
	SetModificationMessage( new BMessage(BM_MULTILINE_TEXTFIELD_MODIFIED));
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineTextControl::~BmMultiLineTextControl() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineTextControl::SetDivider( float divider) {
	float diff = divider-Divider();
	ct_mpm.maxi.x += diff;
	inherited::SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMultiLineTextControl::Divider() const {
	return inherited::Divider();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineTextControl::SetEnabled( bool enabled) {
	inherited::SetEnabled( enabled);
	if (mLabelIsMenu)
		mMenuField->SetEnabled( enabled);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineTextControl::SetText( const char* text) {
	inherited::SetText( text);
	m_text_view->ScrollToSelection();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineTextControl::SetTextSilently( const char* text) {
	SetModificationMessage( NULL);
	SetText( text);
	SetModificationMessage( new BMessage(BM_MULTILINE_TEXTFIELD_MODIFIED));
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineTextControl::FrameResized( float new_width, float new_height) {
	BRect curr = Bounds();
	Invalidate( BRect( Divider(), 0, new_width-1, curr.Height()-1));
	inherited::FrameResized( new_width, new_height);
	m_text_view->ScrollToSelection();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
minimax BmMultiLineTextControl::layoutprefs() {
	return mpm=ct_mpm;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmMultiLineTextControl::layout(BRect frame) {
	if (frame == Frame())
		return frame;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	float occupiedSpace = 3 + Divider();
	m_text_view->MoveTo( occupiedSpace, 4);
	m_text_view->ResizeTo( frame.Width()-occupiedSpace-4, 
								  frame.Height()-m_text_view->Frame().top-4-2);
	return frame;
}
