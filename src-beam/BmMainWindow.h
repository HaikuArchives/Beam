/*
	BmMainWindow.h
		$Id$
*/

#ifndef _BmMainWindow_h
#define _BmMainWindow_h

#include <MMenuBar.h>
#include <MWindow.h>

#include "BmController.h"

class BMenu;

class BmMailFolderView;
class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmToolbarButton;
class CLVContainerView;
class UserResizeSplitView;

class BmMainMenuBar : public MMenuBar, public BmJobController
{
	typedef MMenuBar inherited;
	typedef BmJobController inheritedController;

public:

	BmMainMenuBar();

	void MessageReceived( BMessage* msg);

	BHandler* GetControllerHandler() 	{ return this; }
	void JobIsDone( bool completed);

	void SetAccountMenu( BMenu* m)		{ mAccountMenu = m; }
	
private:
	BMenu* mAccountMenu;
};



class BmMainWindow : public MWindow
{
	typedef MWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_FRAME = 		"bm:frm";
	static const char* const MSG_VSPLITTER = 	"bm:vspl";
	static const char* const MSG_HSPLITTER = 	"bm:hspl";

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
	status_t Archive( BMessage* archive, bool deep=true) const;

	static BmMainWindow* theInstance;

private:
	CLVContainerView* CreateMailFolderView( minimax minmax, int32 width, int32 height);
	CLVContainerView* CreateMailRefView( minimax minmax, int32 width, int32 height);
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	MMenuBar* CreateMenu();
	bool Store();
	status_t Unarchive( BMessage* archive, bool deep=true);
	void MailRefSelectionChanged( int32 numSelected);

	BmMailFolderView* mMailFolderView;
	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	UserResizeSplitView* mVertSplitter;
	UserResizeSplitView* mHorzSplitter;
	
	BmToolbarButton* mCheckButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mReplyAllButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mBounceButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	BmMainMenuBar* mMainMenuBar;

	static bool nIsAlive;
};

#define TheMainWindow BmMainWindow::theInstance

#endif
