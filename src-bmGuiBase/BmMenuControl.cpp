/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <MenuBar.h>
#include <MenuItem.h>

#include <HGroup.h>

#include "Colors.h"

#include "split.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmMenuControl.h"
#include "BmMenuControllerBase.h"

static float kDanoExtraLabelOffset = 3.0;
	// offset required on Dano/Zeta in order to align label with 
	// textcontrol labels.

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::BmMenuControl( const char* label, BMenu* menu, float weight, 
										float maxWidth, const char* fitText)
	:	inherited( BRect(0,0,400,20), NULL, label, menu, true, B_FOLLOW_NONE)
	,	mMenuBar( MenuBar())
{
	mMenuBar->ResizeToPreferred();
	float labelWidth = StringWidth( label);
	if (BeamOnDano) {
		SetDivider( 13 + (label ? labelWidth+19 : 0));
		mMenuBar->ResizeBy(0.0, 5.0);
	} else
		SetDivider( (label ? labelWidth + 3 : 0));
	float minHeight = mMenuBar->Frame().Height()+6;
	if (fitText) {
		float fixedWidth = StringWidth( fitText)+Divider()+27;
		ct_mpm = minimax( fixedWidth, minHeight, fixedWidth, minHeight);
	} else {
		ct_mpm = minimax( 
			StringWidth("1234567890")+Divider()+27, minHeight, 
			maxWidth, minHeight, weight
		);
	}
	ResizeTo( ct_mpm.mini.x, ct_mpm.mini.y);
	if (!BeamOnDano)
		mMenuBar->ResizeBy( 0.0, -1.0);
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
void BmMenuControl::AllAttached() {
	if (BeamOnDano)
		inherited::AllAttached();
	else
		// circumvent resizing done by BMenuField::AllAttached(), as this
		// messes up our own size calculations
		BView::AllAttached();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::MakeEmpty() {
	BMenu* menu = Menu();
	if (!menu)
		return;
	BMenuItem* menuItem;
	while((menuItem = menu->RemoveItem((int32)0)))
		delete menuItem;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::MarkItem( const char* label) {
	BmMenuControllerBase* menuContr 
		= dynamic_cast< BmMenuControllerBase*>( Menu());
	MenuItem()->SetLabel( label);
	if (menuContr) {
		menuContr->MarkItem( label);
		BmMenuControllerBase::MarkItemInMenu( menuContr, label);
	} else {
		BMenuItem* item = Menu()->FindItem( label);
		if (item)
			item->SetMarked( true);
		else
			ClearMark();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::ClearMark() {
	MenuItem()->SetLabel("");
	BmMenuControllerBase::ClearMarkInMenu( Menu());
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::SetEnabled( bool enabled) {
	inherited::SetEnabled( enabled);
	if (enabled)
		SetFlags( Flags() | B_NAVIGABLE);
	else
		SetFlags( Flags() & (0xFFFFFFFF^B_NAVIGABLE));
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
void BmMenuControl::SetDivider( float divider) {
	if (BeamOnDano)
		inherited::SetDivider( divider-13-kDanoExtraLabelOffset);
	else
		inherited::SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMenuControl::Divider() const {
	return inherited::Divider();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmMenuControl::layout(BRect inFrame) {
	if (inFrame == Frame())
		return inFrame;
	BRect frame = inFrame;
	if (BeamOnDano)
		frame.left += kDanoExtraLabelOffset;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	float occupiedSpace = BeamOnDano
									? Divider()+1+13
									: Divider() + 1;

	mMenuBar->MoveTo( occupiedSpace, BeamOnDano ? 5 : 3);
	mMenuBar->ResizeTo( frame.Width()-occupiedSpace-6, 
							  mMenuBar->Frame().Height());
	if (BeamOnDano) {
		// on Dano/Zeta there seems to be a bug with computing the 
		// max-content-width under some conditions. Unfortunately, liblayout
		// is able to trigger exactly these conditions >:o/
		// As a workaround, we explicitly set the max-content-width to a
		// value that works (actually, it's too large, but it doesn't matter):
		mMenuBar->SetMaxContentWidth( frame.Width());
		// additionally, there are drawing artefacts on Zeta, visible
		// in the Beam-Prefs. Explicitly requesting a redraw fixes this:
		Invalidate();
	}

	return inFrame;
}
