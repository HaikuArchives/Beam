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
	:	inherited( BRect(0,0,200,20), NULL, label, menu, false, B_FOLLOW_NONE)
	,	mParent( NULL)
{
	ResizeToPreferred();
	BRect b = Bounds();
	ct_mpm = minimax( b.Width(), b.Height()+3, 1E5, b.Height()+3);
	SetDivider( label ? StringWidth( label)+27 : 0);
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
	return frame;
}
