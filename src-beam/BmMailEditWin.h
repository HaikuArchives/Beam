/*
	BmMailEditWin.h
		$Id$
*/

#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include <MWindow.h>

class MMenuBar;

class BmMailView;
class BmMailViewContainer;
class BmToolbarButton;
class CLVContainerView;

class BmMailEditWin : public MWindow
{
	typedef MWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_FRAME = 		"bm:frm";

public:
	// creator-func, c'tors and d'tor:
	static BmMailEditWin* CreateInstance();
	BmMailEditWin();
	~BmMailEditWin();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t Archive( BMessage* archive, bool deep=true) const;

private:
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	MMenuBar* CreateMenu();
	bool Store();
	status_t Unarchive( BMessage* archive, bool deep=true);

	BmMailView* mMailView;
	
	BmToolbarButton* mSendButton;
	BmToolbarButton* mSaveButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mAttachButton;
	BmToolbarButton* mPeopleButton;
	BmToolbarButton* mPrintButton;
};

#endif
