/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmGuiUtil_h
#define _BmGuiUtil_h

#include <set>

#include <MenuItem.h>

#include "BmString.h"

class BHandler;
class BMenu;
class BMenuItem;
class BMessage;


template <class T> 
class BmViewManager
{
	typedef set<T*> BmViewSet;
public:
	BmViewManager()
	:	mLock( "ViewManager") {}
	virtual ~BmViewManager() {}
	void Register(T* v)
	{
		mLock.Lock();
		mViewSet.insert(v);
		mLock.Unlock();
	}
	void Unregister(T* v)
	{
		mLock.Lock();
		mViewSet.erase(v);
		mLock.Unlock();
	}
	virtual void UpdateAll();
	static BmViewManager<T>* Instance()
	{
		if (!theInstance)
			theInstance = new BmViewManager<T>;
		return theInstance;
	}
private:
	static BmViewManager<T>* theInstance;
	BmViewSet mViewSet;
	BLocker mLock;
};

class BmMenuItem : public BMenuItem
{
public:
	BmMenuItem(const char* label, BMessage* msg, const char* idForShortcut);
	BmMenuItem(BMenu* subMenu, BMessage* msg, const char* idForShortcut);
	virtual ~BmMenuItem();
	void UpdateShortcut();
private:
	BmString mShortcutID;
};

typedef BmViewManager<BmMenuItem> BmMenuItemManager;
#define TheMenuItemManager (BmMenuItemManager::Instance())

/*------------------------------------------------------------------------------*\
	utility functions that make menu-creation easier:
\*------------------------------------------------------------------------------*/
char ParseShortcut( BmString shortcut, int32* _modifiers);
void AddItemToMenu( BMenu* menu, BMenuItem* item, BHandler* target=NULL);
BMenuItem* CreateMenuItem( const char* label, int32 msgWhat, 
									const char* idForShortcut=NULL);
BMenuItem* CreateMenuItem( const char* label, BMessage* msg, 
									const char* idForShortcut=NULL);
BMenuItem* CreateSubMenuItem( const char* label, int32 msgWhat, 
										const char* idForShortcut=NULL,
										BMenu* subMenu=NULL);
BMenuItem* CreateSubMenuItem( const char* label, BMessage* msg, 
										const char* idForShortcut,
										BMenu* subMenu=NULL);

#endif
