/*
	BmMenuControl.cpp
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


#include <MenuBar.h>
#include <MenuItem.h>

#include <HGroup.h>

#include "Colors.h"

#include "BmMenuControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::BmMenuControl( const char* label, BMenu* menu, float weight, 
										float maxWidth) 
	:	inherited( BRect(0,0,400,20), NULL, label, menu, true, B_FOLLOW_NONE)
	,	mMenu( static_cast<BMenu*>( ChildAt( 0)))
{
	ResizeToPreferred();
	BRect b = Bounds();
	float labelWidth = StringWidth( label);
	ct_mpm = minimax( StringWidth("12345678901234567890123456789012345"), b.Height()+4, 
							maxWidth, b.Height()+4, weight);
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
void BmMenuControl::MarkItem( const char* label) {
	BMenuItem* item = Menu()->FindItem( label);
	if (item)
		item->SetMarked( true);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::ClearMark() {
	BMenuItem* item = mMenu->FindMarked();
	if (item)
		item->SetMarked( false);
	MarkItem("");
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
