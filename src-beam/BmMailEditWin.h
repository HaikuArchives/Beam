/*
	BmMailEditWin.h
		$Id$
*/

#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include <MessageFilter.h>

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
	bool UpdateMailFields();

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
	BmTextControl* mSenderControl;
	BmTextControl* mSubjectControl;
	BmTextControl* mToControl;
	
	BmMenuControl* mCharsetControl;
	BmMenuControl* mSmtpControl;
	
	bool mShowDetails;
	MView* mOuterGroup;
};



class BmMailViewFilter : public BMessageFilter {
	typedef BMessageFilter inherited;
public:
	// c'tor:
	BmMailViewFilter( BmMailEditWin* editWin);
	
	// overrides of message-filter-base:
	filter_result Filter( BMessage* msg, BHandler** target);

private:
	BmMailEditWin* mEditWin;
};


#endif
