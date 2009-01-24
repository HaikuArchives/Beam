/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include "BmWindow.h"

class BmMailFolderView;
class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmMenuController;
class MMenuBar;
class BmToolbarButton;
class CLVContainerView;
class UserResizeSplitView;
class BmLogWindow;
class BmMailRefFilterControl;
class BmMailRefViewFilterControl;

class BmMainWindow : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_VSPLITTER;
	static const char* const MSG_HSPLITTER;

public:
	// creator-func, c'tors and d'tor:
	static BmMainWindow* CreateInstance();
	BmMainWindow();
	~BmMainWindow();

	// class-methods:
	static bool IsAlive();

	// native methods:
	void BeginLife();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	void WorkspacesChanged( uint32 oldWorkspaces, uint32 newWorkspaces);
	void Minimize( bool minimize);

	static BmMainWindow* theInstance;

protected:
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);

private:
	MMenuBar* CreateMenu();
	void MailFolderSelectionChanged( bool haveSelectedFolder);
	void MailRefSelectionChanged( bool haveSelectedRef);
	void MailViewChanged( bool hasMail);

	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	UserResizeSplitView* mVertSplitter;
	UserResizeSplitView* mHorzSplitter;
	
	BmLogWindow* mErrLogWin;
	
	BmToolbarButton* mCheckButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mTofuButton;
	BmToolbarButton* mSpamButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	BmMenuController* mAccountMenu;
	
	BmMailRefFilterControl* mMailRefFilterControl;
	BmMailRefViewFilterControl* mMailRefViewFilterControl;
	
	MMenuBar* mMainMenuBar;
	static bool nIsAlive;

	// Hide copy-constructor and assignment:
	BmMainWindow( const BmMainWindow&);
	BmMainWindow operator=( const BmMainWindow&);
};

#define TheMainWindow BmMainWindow::theInstance

#endif
