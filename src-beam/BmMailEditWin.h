/*
	BmMailEditWin.h
		$Id$
*/

#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include "BmWindow.h"

class MMenuBar;

class BmMailView;
class BmMailViewContainer;
class BmMenuControl;
class BmToolbarButton;
class BmTextControl;
class CLVContainerView;
class MPictureButton;

class BmMailEditWin : public BmWindow
{
	typedef BmWindow inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmMailEditWin* CreateInstance();
	BmMailEditWin();
	~BmMailEditWin();

	// native methods:
	void UpdateMailFields();

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();

private:
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	void CreateGUI();
	MMenuBar* CreateMenu();

	BmMailView* mMailView;
	
	MPictureButton* mShowDetailsButton;

	BmToolbarButton* mSendButton;
	BmToolbarButton* mSaveButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mAttachButton;
	BmToolbarButton* mPeopleButton;
	BmToolbarButton* mPrintButton;
	
	BmTextControl* mBccControl;
	BmTextControl* mCcControl;
	BmTextControl* mFromControl;
	BmTextControl* mReplyToControl;
	BmTextControl* mSubjectControl;
	BmTextControl* mToControl;
	
	BmMenuControl* mCharsetControl;
	
	bool mShowDetails;
	MView* mOuterGroup;
};

#endif
