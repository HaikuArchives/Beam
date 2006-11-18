/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Menu.h>
#include <MenuItem.h>

#include "BmEncoding.h"
using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmPrefs.h"

BmMenuItemManager* BmMenuItemManager::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuItem::BmMenuItem(const char* label, BMessage* msg, 
							  const char* idForShortcut)
	:	BMenuItem(label, msg)
	,	mShortcutID(idForShortcut ? idForShortcut : label)
{
	TheMenuItemManager->Register(this);
	UpdateShortcut();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuItem::BmMenuItem(BMenu* menu, BMessage* msg, 
							  const char* idForShortcut)
	:	BMenuItem(menu, msg)
	,	mShortcutID(idForShortcut)
{
	TheMenuItemManager->Register(this);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMenuItem::~BmMenuItem()
{
	TheMenuItemManager->Unregister(this);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuItem::UpdateShortcut()
{
	BmString shortcut = ThePrefs->GetShortcutFor(mShortcutID.String());
	int32 modifiers = 0;
	char keycode = ParseShortcut(shortcut, &modifiers);
	SetShortcut(keycode, modifiers);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuItemManager::UpdateAll() {
	mLock.Lock();
	BmViewSet::iterator iter;
	for(iter=mViewSet.begin(); iter != mViewSet.end(); ++iter) {
		BmMenuItem* menuItem = (*iter);
		if (menuItem)
			menuItem->UpdateShortcut();
	}
	mLock.Unlock();
}

/*------------------------------------------------------------------------------*\
	ParseShortcut(shortcut, modifiers)
		-	parses given shortcut and returns info about the corresponding
			keycode and modifiers
\*------------------------------------------------------------------------------*/
char ParseShortcut(BmString shortcut, int32* _modifiers) {
	shortcut.RemoveSet(" \t");
	shortcut.ToUpper();
	char keycode = 0;
	int32 modifiers = 0;
	int32 pos;
	if ((pos=shortcut.FindFirst("<SHIFT>")) != B_ERROR) {
		modifiers |= B_SHIFT_KEY;
		shortcut.Remove(pos, 7);
	}
	if (shortcut=="<RIGHT_ARROW>")
		keycode = B_RIGHT_ARROW;
	else if (shortcut=="<LEFT_ARROW>")
		keycode = B_LEFT_ARROW;
	else if (shortcut=="<UP_ARROW>")
		keycode = B_UP_ARROW;
	else if (shortcut=="<DOWN_ARROW>")
		keycode = B_DOWN_ARROW;
	else if (shortcut.Length())
		keycode = shortcut[0];
	if (_modifiers)
		*_modifiers = modifiers;
	return keycode;
}

/*------------------------------------------------------------------------------*\
	AddItemToMenu( menu, item, target)
		-	adds item to menu and sets item's target
\*------------------------------------------------------------------------------*/
void AddItemToMenu(BMenu* menu, BMenuItem* item, BHandler* target) {
	if (target)
		item->SetTarget(target);
	menu->AddItem(item);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateMenuItem(const char* label, int32 msgWhat, 
								  const char* idForShortcut) {
	return CreateMenuItem(label, new BMessage(msgWhat), idForShortcut);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateMenuItem(const char* label, BMessage* msg, 
								  const char* idForShortcut) {
	return new BmMenuItem(label, msg, idForShortcut);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateSubMenuItem(const char* label, int32 msgWhat, 
									  const char* idForShortcut, BMenu* subMenu) {
	return CreateSubMenuItem(label, new BMessage(msgWhat), idForShortcut, subMenu);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* CreateSubMenuItem(const char* label, BMessage* msg, 
									  const char* idForShortcut, BMenu* subMenu) 
{
	if (!subMenu)
		subMenu = new BMenu(label);
	return new BmMenuItem(subMenu, msg, idForShortcut ? idForShortcut : label);
}
