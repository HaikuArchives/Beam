/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsWin_h
#define _BmPrefsWin_h

#include "ColumnListView.h"

#include "BmWindow.h"

class UserResizeSplitView;
class BmPrefsViewContainer;
class MButton;

class BmPrefsWin : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_VSPLITTER;

public:
	// creator-func, c'tors and d'tor:
	static BmPrefsWin* CreateInstance();
	BmPrefsWin();
	~BmPrefsWin();

	// native methods:
	void SendMsgToSubView( const BmString& subViewName, BMessage* msg);

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();

	static BmPrefsWin* theInstance;

protected:
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);

private:
	ColumnListView* CreatePrefsListView( int32 width, int32 height);
	void PrefsListSelectionChanged( int32 numSelected);

	ColumnListView* mPrefsListView;
	BmPrefsViewContainer* mPrefsViewContainer;
	UserResizeSplitView* mVertSplitter;
	
	bool mChanged;
	MButton* mSaveButton;
	MButton* mRevertButton;
	MButton* mDefaultsButton;

	// Hide copy-constructor and assignment:
	BmPrefsWin( const BmPrefsWin&);
	BmPrefsWin operator=( const BmPrefsWin&);
};

#define ThePrefsWin BmPrefsWin::theInstance

#endif
