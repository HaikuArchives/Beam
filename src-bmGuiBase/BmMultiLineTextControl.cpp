/*
	BmMultiLineTextControl.cpp
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


#include <MenuField.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <MWindow.h>

#ifdef __POWERPC__
#define BM_BUILDING_SANTAPARTSFORBEAM 1
#endif

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
	float divPos = label ? StringWidth( label)+27 : 0;
	BFont font;
	m_text_view->GetFont( &font);
	font_height fh;
	font.GetHeight( &fh);
	float lineHeight = fh.ascent + fh.descent + fh.leading; 
	float height = lineHeight*lineCount+12;
	if (minTextLen)
		ct_mpm = minimax( divPos + font.StringWidth("W")*minTextLen, height+4, 
								1E5, fixedHeight ? height+4 : 1E5);
	else
		ct_mpm = minimax( divPos + font.StringWidth("W")*10, height+6, 
								1E5, fixedHeight ? height+6 : 1E5);
	SetDivider( divPos);
	if (labelIsMenu) {
		float width, height;
		GetPreferredSize( &width, &height);
		BPopUpMenu* popup = new BPopUpMenu( label, true, false);
		mMenuField = new BMenuField( BRect( 2,0,Divider(),height), NULL, label, popup, 
											  true, B_FOLLOW_NONE, B_WILL_DRAW);
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
	float occupiedSpace = Divider()-11;
	m_text_view->MoveTo( occupiedSpace, 4);
	m_text_view->ResizeTo( frame.Width()-occupiedSpace-5, 
								  frame.Height()-m_text_view->Frame().top-4-2);
	return frame;
}
