/*
	BmMailViewWin.h
		$Id$
*/

#ifndef _BmMailViewWin_h
#define _BmMailViewWin_h

#include "BmWindow.h"

class MMenuBar;

class BmMailView;
class BmMailViewContainer;
class BmToolbarButton;

class BmMailViewWin : public BmWindow
{
	typedef BmWindow inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmMailViewWin* CreateInstance();
	BmMailViewWin();
	~BmMailViewWin();

	// native methods:
	void ShowMail( BmMailRef* ref);

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();

private:
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	void CreateGUI();
	MMenuBar* CreateMenu();

	BmMailView* mMailView;
	
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mReplyAllButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mBounceButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	MView* mOuterGroup;
};


#endif
