/*
	BmTextControl.cpp
		$Id$
*/

#include <MenuField.h>
#include <PopUpMenu.h>

#include <HGroup.h>

#include "Colors.h"

#include "BmTextControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmTextControl::BmTextControl( const char* label, bool labelIsMenu) 
	:	inherited( BRect(0,0,0,0), NULL, labelIsMenu ? "" : label, NULL, NULL, B_FOLLOW_NONE)
	,	mLabelIsMenu( labelIsMenu)
	,	mTextView( static_cast<BTextView*>( ChildAt( 0)))
	,	mParent( NULL)
{
	ResizeToPreferred();
	BRect b = Bounds();
	ct_mpm = minimax( b.Width(), b.Height()+3, 1E5, b.Height()+3);
	SetDivider( label ? StringWidth( label)+27 : 0);
	if (labelIsMenu) {
		float width, height;
		GetPreferredSize( &width, &height);
		BPopUpMenu* popup = new BPopUpMenu( label, true, false);
		mMenuField = new BMenuField( BRect( 2,0,Divider(),height), NULL, label, popup, 
											  true, B_FOLLOW_NONE, B_WILL_DRAW);
		mMenuField->SetDivider( 0);
		AddChild( mMenuField);
	}
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
void BmTextControl::DetachFromParent() {
	if (Parent()) {
		mParent = dynamic_cast<HGroup*>(Parent());
		RemoveSelf();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::ReattachToParent() {
	if (mParent) {
		mParent->AddChild( this);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmTextControl::FrameResized( float new_width, float new_height) {
	BRect curr = Bounds();
	Invalidate( BRect( Divider(), 0, new_width-1, curr.Height()));
	inherited::FrameResized( new_width, new_height);
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
	float occupiedSpace = Divider()+3;
	mTextView->MoveTo( occupiedSpace, 5);
	mTextView->ResizeTo( frame.Width()-occupiedSpace-6, mTextView->Frame().Height());
	return frame;
}
