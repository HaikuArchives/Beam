/*
	BmMenuControl.cpp
		$Id$
*/

#include <HGroup.h>

#include "Colors.h"

#include "BmMenuControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::BmMenuControl( const char* label, BMenu* menu) 
	:	inherited( BRect(0,0,200,20), NULL, label, menu, true, B_FOLLOW_NONE)
	,	mParent( NULL)
	,	mMenu( static_cast<BMenu*>( ChildAt( 0)))
{
	ResizeToPreferred();
	BRect b = Bounds();
	float labelWidth = StringWidth( label);
	ct_mpm = minimax( StringWidth("123456789012345678901234567890"), b.Height()+4, -1, b.Height()+4);
	SetDivider( label ? labelWidth+27 : 0);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::~BmMenuControl() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::DetachFromParent() {
	if (Parent()) {
		mParent = dynamic_cast<HGroup*>(Parent());
		RemoveSelf();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::ReattachToParent() {
	if (mParent) {
		mParent->AddChild( this);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
minimax BmMenuControl::layoutprefs() {
	return mpm = ct_mpm;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmMenuControl::layout(BRect frame) {
	if (frame == Frame())
		return frame;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	float occupiedSpace = Divider()-10;
	mMenu->MoveTo( occupiedSpace, mMenu->Frame().top);
	mMenu->ResizeTo( frame.Width()-occupiedSpace-6, mMenu->Frame().Height());
	return frame;
}
