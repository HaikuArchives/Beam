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

#ifdef __POWERPC__
#define BM_BUILDING_SANTAPARTSFORBEAM 1
#endif

#include "Colors.h"

#include "split.hh"
using namespace regexx;

#include "BmMenuControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuControl::BmMenuControl( const char* label, BMenu* menu, float weight, 
										float maxWidth, const char* fitText)
	:	inherited( BRect(0,0,400,20), NULL, label, menu, true, B_FOLLOW_NONE)
	,	mMenu( static_cast<BMenu*>( ChildAt( 0)))
{
	ResizeToPreferred();
	BRect b = Bounds();
	float labelWidth = StringWidth( label);
	SetDivider( label ? labelWidth+27 : 0);
	if (fitText) {
		float fixedWidth = StringWidth( fitText)+Divider()+27;
		ct_mpm = minimax( fixedWidth, b.Height()+4, 
								fixedWidth, b.Height()+4);
	} else
		ct_mpm = minimax( StringWidth("12345678901234567890123456789012345"), 
								b.Height()+4, maxWidth, b.Height()+4, weight);
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
void BmMenuControl::MarkItem( const char* label, bool recurse) {
	BMenuItem* item = NULL;
	if (recurse) {
		doClearMark( Menu());
		MenuItem()->SetLabel( label);
		// we walk down the tree to find the corresponding menu-item:
		vector<BmString> itemVect;
		split( "/", label, itemVect);
		BMenu* currMenu = Menu();
		for( uint32 i=0; currMenu && i<itemVect.size(); ++i) {
			BmString str = itemVect[i];
			item = currMenu->FindItem( str.String());
			currMenu = item 
							? item->Submenu()
							: NULL;
		}
	} else
		item = Menu()->FindItem( label);
	if (item)
		item->SetMarked( true);
	else
		ClearMark();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::ClearMark() {
	MenuItem()->SetLabel("");
	doClearMark( Menu());
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControl::doClearMark( BMenu* menu) {
	if (!menu)
		return;
	BMenuItem* item;
	while( (item = menu->FindMarked()) != NULL)
		item->SetMarked( false);
	int32 count=menu->CountItems();
	for( int i=0; i<count; ++i) {
		BMenu* subMenu = menu->SubmenuAt( i);
		if (subMenu)
			doClearMark( subMenu);
	}
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
	if (occupiedSpace < 3)
		occupiedSpace = 3;					// leave room for focus-rectangle
	mMenu->MoveTo( occupiedSpace, mMenu->Frame().top);
	mMenu->ResizeTo( frame.Width()-occupiedSpace-6, mMenu->Frame().Height());
	return frame;
}
