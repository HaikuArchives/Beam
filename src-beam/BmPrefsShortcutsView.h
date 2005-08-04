/*
	BmPrefsShortcutsView.h
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


#ifndef _BmPrefsShortcutsView_h
#define _BmPrefsShortcutsView_h

#include <MessageFilter.h>

#include "BmListController.h"
#include "BmPrefsView.h"
#include "BmTextControl.h"

/*------------------------------------------------------------------------------*\
	BmShortcutControl
		-	
\*------------------------------------------------------------------------------*/
class BmShortcutControl : public BmTextControl
{
	typedef BmTextControl inherited;

public:
	// creator-func, c'tors and d'tor:
	BmShortcutControl( const char* label);
	~BmShortcutControl()						{ }
	
	// overrides of BmTextControl:
	void KeyDown(const char *bytes, int32 numBytes);

	static filter_result FilterHook( BMessage* msg, BHandler** handler,
												BMessageFilter* filter);

private:
	static BmShortcutControl* nTheInstance;

	// Hide copy-constructor and assignment:
	BmShortcutControl( const BmShortcutControl&);
	BmShortcutControl operator=( const BmShortcutControl&);
};




/*------------------------------------------------------------------------------*\
	BmPrefsShortcutsView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsShortcutsView : public BmPrefsView {
	typedef BmPrefsView inherited;

public:
	// c'tors and d'tor:
	BmPrefsShortcutsView();
	virtual ~BmPrefsShortcutsView();
	
	// native methods:
	void ShowShortcut( int32 selection);

	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

	// getters:

	// setters:

private:
	ColumnListView* CreateListView( int32 width, int32 height);

	ColumnListView* mListView;
	BmTextControl* mNameControl;
	BmShortcutControl* mShortcutControl;

	// Hide copy-constructor and assignment:
	BmPrefsShortcutsView( const BmPrefsShortcutsView&);
	BmPrefsShortcutsView operator=( const BmPrefsShortcutsView&);
};

#endif
