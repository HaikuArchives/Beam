/*
	BmGuiUtil.cpp
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

#include <Menu.h>
#include <MenuItem.h>

#include "BmGuiUtil.h"
#include "BmPrefs.h"

/*------------------------------------------------------------------------------*\
	AddItemToMenu( menu, item, target)
		-	adds item to menu and sets item's target
\*------------------------------------------------------------------------------*/
void AddItemToMenu( BMenu* menu, BMenuItem* item, BHandler* target) {
	if (target)
		item->SetTarget( target);
	menu->AddItem( item);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateMenuItem( const char* label, int32 msgWhat, const char* idForShortcut) {
	return CreateMenuItem( label, new BMessage(msgWhat), idForShortcut);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateMenuItem( const char* label, BMessage* msg, const char* idForShortcut) {
	BString shortcut = ThePrefs->GetShortcutFor( idForShortcut ? idForShortcut : label);
	shortcut.RemoveSet( " \t");
	int32 modifiers = 0;
	int32 pos;
	if ((pos=shortcut.IFindFirst("<SHIFT>")) != B_ERROR) {
		modifiers |= B_SHIFT_KEY;
		shortcut.Remove( pos, 7);
	}
	if (shortcut.Length())
		return new BMenuItem( label, msg, shortcut[0], modifiers);
	else
		return new BMenuItem( label, msg);
}
