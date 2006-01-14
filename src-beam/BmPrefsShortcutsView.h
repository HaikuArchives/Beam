/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
