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
	static BmMailViewWin* CreateInstance( BmMailRef* mailRef=NULL);
	BmMailViewWin( BmMailRef* mailRef=NULL);
	~BmMailViewWin();

	// native methods:
	void ShowMail( BmMailRef* mailRef);

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
	BmToolbarButton* mRedirectButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	MView* mOuterGroup;

	// Hide copy-constructor and assignment:
	BmMailViewWin( const BmMailViewWin&);
	BmMailViewWin operator=( const BmMailViewWin&);
};


#endif
