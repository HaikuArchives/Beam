/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmGuiUtil_h
#define _BmGuiUtil_h

#include "BmString.h"

class BHandler;
class BMenu;
class BMenuItem;
class BMessage;
/*------------------------------------------------------------------------------*\
	utility functions that make menu-creation easier:
\*------------------------------------------------------------------------------*/
void AddItemToMenu( BMenu* menu, BMenuItem* item, BHandler* target=NULL);
BMenuItem* CreateMenuItem( const char* label, int32 msgWhat, 
									const char* idForShortcut=NULL);
BMenuItem* CreateMenuItem( const char* label, BMessage* msg, 
									const char* idForShortcut=NULL);
BMenuItem* CreateSubMenuItem( const char* label, int32 msgWhat, 
										const char* idForShortcut=NULL);
BMenuItem* CreateSubMenuItem( const char* label, BMessage* msg, 
										const char* idForShortcut);

#endif
