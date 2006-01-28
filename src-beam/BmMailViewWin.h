/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailViewWin_h
#define _BmMailViewWin_h

#include "BmWindow.h"

class MMenuBar;

class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmMenuController;
class BmToolbarButton;
class BmToolbar;
class UserResizeSplitView;

class BmMailViewWin : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_HSPLITTER;

public:
	// creator-func, c'tors and d'tor:
	static BmMailViewWin* CreateInstance( BmMailRef* mailRef=NULL);
	BmMailViewWin( BmMailRef* mailRef=NULL);
	~BmMailViewWin();

	void ShowMail( BmMailRef* mailRef, bool async=true);

	// overrides of BmWindow base:
	void BeginLife();
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);
	
	// getters:
	BmMailView* MailView() const			{ return mMailView; }
	
private:
	void CreateGUI();
	MMenuBar* CreateMenu();

	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	
	UserResizeSplitView* mHorzSplitter;
	
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	BmToolbar* mToolbar;
	
	MView* mOuterGroup;

	BmMenuController* mFilterMenu;

	static float nNextXPos;
	static float nNextYPos;

	// Hide copy-constructor and assignment:
	BmMailViewWin( const BmMailViewWin&);
	BmMailViewWin operator=( const BmMailViewWin&);
};


#endif
