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

#include "split.hh"
using namespace regexx;

#include "BmDataModel.h"
#include "BmEncoding.h"
using namespace BmEncoding;
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
BMenuItem* CreateMenuItem( const char* label, int32 msgWhat, 
									const char* idForShortcut) {
	return CreateMenuItem( label, new BMessage(msgWhat), idForShortcut);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateMenuItem( const char* label, BMessage* msg, 
									const char* idForShortcut) {
	BmString name( idForShortcut ? idForShortcut : label);
	name.RemoveAll( "...");
	BmString shortcut = ThePrefs->GetShortcutFor( name.String());
	shortcut.RemoveSet( " \t");
	shortcut.ToUpper();
	int32 modifiers = 0;
	int32 pos;
	if ((pos=shortcut.FindFirst("<SHIFT>")) != B_ERROR) {
		modifiers |= B_SHIFT_KEY;
		shortcut.Remove( pos, 7);
	}
	if (shortcut=="<RIGHT_ARROW>")
		return new BMenuItem( label, msg, B_RIGHT_ARROW, modifiers);
	else if (shortcut=="<LEFT_ARROW>")
		return new BMenuItem( label, msg, B_LEFT_ARROW, modifiers);
	else if (shortcut=="<UP_ARROW>")
		return new BMenuItem( label, msg, B_UP_ARROW, modifiers);
	else if (shortcut=="<DOWN_ARROW>")
		return new BMenuItem( label, msg, B_DOWN_ARROW, modifiers);
	else if (shortcut.Length())
		return new BMenuItem( label, msg, shortcut[0], modifiers);
	else
		return new BMenuItem( label, msg);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateSubMenuItem( const char* label, int32 msgWhat, 
										const char* idForShortcut) {
	return CreateSubMenuItem( label, new BMessage(msgWhat), idForShortcut);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateSubMenuItem( const char* label, BMessage* msg, 
										const char* idForShortcut) {
	BmString name( idForShortcut ? idForShortcut : label);
	name.RemoveAll( "...");
	BmString shortcut = ThePrefs->GetShortcutFor( name.String());
	shortcut.RemoveSet( " \t");
	shortcut.ToUpper();
	int32 modifiers = 0;
	int32 pos;
	if ((pos=shortcut.FindFirst("<SHIFT>")) != B_ERROR) {
		modifiers |= B_SHIFT_KEY;
		shortcut.Remove( pos, 7);
	}
	BMenu* subMenu = new BMenu( label);
	BMenuItem* item = new BMenuItem( subMenu, msg);
	if (shortcut=="<RIGHT_ARROW>")
		item->SetShortcut( B_RIGHT_ARROW, modifiers);
	else if (shortcut=="<LEFT_ARROW>")
		item->SetShortcut( B_LEFT_ARROW, modifiers);
	else if (shortcut=="<UP_ARROW>")
		item->SetShortcut( B_UP_ARROW, modifiers);
	else if (shortcut=="<DOWN_ARROW>")
		item->SetShortcut( B_DOWN_ARROW, modifiers);
	else if (shortcut.Length())
		item->SetShortcut( shortcut[0], modifiers);
	return item;
}

const char* const MSG_CHARSET = "bm:chset";
/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType) {
	BmString charset;
	if (!menu)
		return;
	menu->SetLabelFromMarked( false);
	// add standard entries:
	vector<BmString> charsets;
	BmString sets = ThePrefs->GetString( "StandardCharsets");
	split( BmPrefs::nListSeparator, sets, charsets);
	int32 numStdSets = charsets.size();
	for( int i=0; i<numStdSets; ++i) {
		charset = charsets[i];
		charset.ToLower();
		BMessage* msg = new BMessage( msgType);
		msg->AddString( MSG_CHARSET, charset.String());
		AddItemToMenu( menu, CreateMenuItem( charset.String(), msg), target);
	}
	// add all other charsets:
	BMenu* moreMenu = new BMenu( "<show all>");
	moreMenu->SetLabelFromMarked( false);
	BFont font( *be_plain_font);
	font.SetSize( 10);
	moreMenu->SetFont( &font);
	BmCharsetMap::const_iterator iter;
	for( iter = TheCharsetMap.begin(); iter != TheCharsetMap.end(); ++iter) {
		if (iter->second) {
			charset = iter->first;
			charset.ToLower();
			BMessage* msg = new BMessage( msgType);
			msg->AddString( MSG_CHARSET, charset.String());
			AddItemToMenu( moreMenu, 
								CreateMenuItem( charset.String(), msg), 
								target);
		}
	}
	menu->AddSeparatorItem();
	menu->AddItem( moreMenu);
}

/*------------------------------------------------------------------------------*\
	AddListToMenu()
		-	
\*------------------------------------------------------------------------------*/
typedef map< BmString, BmListModelItem* > BmSortedItemMap;

void AddListToMenu( BmListModel* list, BMenu* menu, BMessage* msgTemplate,
						  BHandler* msgTarget, BFont* font, bool skipFirstLevel,
						  bool addNoneItem, const BmString shortcuts) {
	BmSortedItemMap sortedMap;
	if (list) {
		BmModelItemMap::const_iterator iter;
		for( iter = list->begin();  iter != list->end();  ++iter) {
			BmString sortKey = iter->second->DisplayKey();
			sortedMap[sortKey.ToLower()] = iter->second.Get();
		}
		if (addNoneItem && menu) {
			BMenuItem* noneItem = new BMenuItem( BM_NoItemLabel.String(),
															 new BMessage( *msgTemplate));
			noneItem->SetTarget( msgTarget);
			menu->AddItem( noneItem);
		}
		int s=0;
		BmSortedItemMap::const_iterator siter;
		for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter, ++s) {
			if (s<shortcuts.Length())
				AddListItemToMenu( siter->second, menu, msgTemplate, msgTarget, 
										 font, skipFirstLevel, shortcuts[s]);
			else
				AddListItemToMenu( siter->second, menu, msgTemplate, msgTarget, 
										 font, skipFirstLevel);
		}
	}
}
								
/*------------------------------------------------------------------------------*\
	AddListItemToMenu()
		-	
\*------------------------------------------------------------------------------*/
void AddListItemToMenu( BmListModelItem* item, BMenu* menu, 
								BMessage* msgTemplate, BHandler* msgTarget,
								BFont* font, bool skipThisButAddChildren, 
								char shortcut) {
	if (menu) {
		BmSortedItemMap sortedMap;
		if (skipThisButAddChildren) {
			if (!item->empty()) {
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddListItemToMenu( siter->second, menu, msgTemplate, msgTarget, 
											 font);
			}
		} else {
			BMessage* msg = new BMessage( *msgTemplate);
			msg->AddString( BmListModel::MSG_ITEMKEY, item->Key().String());
			BMenuItem* menuItem;
			if (!item->empty()) {
				BMenu* subMenu = new BMenu( item->DisplayKey().String());
				if (font)
					subMenu->SetFont( font);
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddListItemToMenu( siter->second, subMenu, msgTemplate, 
											 msgTarget, font);
				menuItem = new BMenuItem( subMenu, msg);
			} else {
				menuItem = new BMenuItem( item->DisplayKey().String(), msg);
			}
			if (shortcut)
				menuItem->SetShortcut( shortcut, 0);
			menuItem->SetTarget( msgTarget);
			menu->AddItem( menuItem);
		}
	}
}
