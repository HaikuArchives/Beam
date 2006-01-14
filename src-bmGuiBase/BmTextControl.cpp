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

#include "Colors.h"

#include "BmBasics.h"
#include "BmMenuControllerBase.h"
#include "BmTextControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControl::BmTextControl( const char* label, bool labelIsMenu, 
										int32 fixedTextLen, int32 minTextLen)
	:	inherited( BRect(0,0,0,0), NULL, labelIsMenu ? "" : label, NULL, NULL, 
					  B_FOLLOW_NONE)
	,	mLabelIsMenu( labelIsMenu)
	,	mTextView( static_cast<BTextView*>( ChildAt( 0)))
{
	InitSize( label, fixedTextLen, minTextLen, NULL);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControl::BmTextControl( const char* label, BmMenuControllerBase* menu, 
										int32 fixedTextLen, int32 minTextLen)
	:	inherited( BRect(0,0,0,0), NULL, "", NULL, NULL, B_FOLLOW_NONE)
	,	mLabelIsMenu( true)
	,	mTextView( static_cast<BTextView*>( ChildAt( 0)))
{
	InitSize( label, fixedTextLen, minTextLen, menu);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControl::~BmTextControl() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::InitSize( const char* label, int32 fixedTextLen, 
										int32 minTextLen, BmMenuControllerBase* popup) {
	ResizeToPreferred();
	BRect b = Bounds();
	float divPos = label ? StringWidth( label)+27 : 0;
	inherited::SetDivider( divPos);
	BFont font;
	mTextView->GetFont( &font);
	if (fixedTextLen) {
		mTextView->SetMaxBytes( fixedTextLen);
		float width = divPos + font.StringWidth("W")*fixedTextLen;
		ct_mpm = minimax( width, b.Height()+4, width, b.Height()+4);
	} else {
		if (minTextLen)
			ct_mpm = minimax( divPos + font.StringWidth("W")*minTextLen, 
									b.Height()+4, 1E5, b.Height()+4);
		else
			ct_mpm = minimax( divPos + font.StringWidth("W")*10, b.Height()+4, 
									1E5, b.Height()+4);
	}
	if (mLabelIsMenu) {
		float width, height;
		GetPreferredSize( &width, &height);
//		if (!popup)
//			popup = new BmMenuControllerBase( label, true, false);
		if (BeamOnDano)
			mMenuField 
				= new BMenuField( BRect( 2,2,Divider(),height), NULL, label,
										popup, true, B_FOLLOW_NONE, B_WILL_DRAW);
		else
			mMenuField 
				= new BMenuField( BRect( 2,0,Divider(),height), NULL, label,
										popup, true, B_FOLLOW_NONE, B_WILL_DRAW);
		mMenuField->SetDivider( 0);
		AddChild( mMenuField);
	}
	SetModificationMessage( new BMessage(BM_TEXTFIELD_MODIFIED));
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::SetDivider( float divider) {
	float diff = divider-Divider();
	ct_mpm.maxi.x += diff;
	inherited::SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmTextControl::Divider() const {
	return inherited::Divider();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::SetEnabled( bool enabled) {
	inherited::SetEnabled( enabled);
	if (enabled)
		mTextView->SetFlags( mTextView->Flags() | B_NAVIGABLE);
	else
		mTextView->SetFlags( mTextView->Flags() & (0xFFFFFFFF^B_NAVIGABLE));
	if (mLabelIsMenu)
		mMenuField->SetEnabled( enabled);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::SetText( const char* text) {
	inherited::SetText( text);
	TextView()->ScrollToSelection();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::SetTextSilently( const char* text) {
	SetModificationMessage( NULL);
	SetText( text);
	SetModificationMessage( new BMessage(BM_TEXTFIELD_MODIFIED));
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::FrameResized( float new_width, float new_height) {
	BRect curr = Bounds();
	Invalidate( BRect( Divider(), 0, new_width-1, curr.Height()));
	inherited::FrameResized( new_width, new_height);
	TextView()->ScrollToSelection();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControllerBase* BmTextControl::Menu() const { 
	return mMenuField 
				? dynamic_cast< BmMenuControllerBase*>( mMenuField->Menu())
				: NULL; 
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
minimax BmTextControl::layoutprefs() {
	return mpm=ct_mpm;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmTextControl::layout(BRect frame) {
	if (frame == Frame())
		return frame;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	float occupiedSpace = Divider()-10;
	if (occupiedSpace < 3)
		occupiedSpace = 3;					// leave room for focus-rectangle
	mTextView->MoveTo( occupiedSpace, 5);
	mTextView->ResizeTo( frame.Width()-occupiedSpace-6, 
								mTextView->Frame().Height());
	return frame;
}
