/*
	BmMailEditWin.h
		$Id$
*/

#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include <MessageFilter.h>

#include "BmWindow.h"

class MMenuBar;

class BmMail;
class BmMailView;
class BmMailRef;
class BmMailViewContainer;
class BmMenuControl;
class BmToolbarButton;
class BmTextControl;
class CLVContainerView;
class MPictureButton;

class BmMailEditWin : public BmWindow
{
	typedef BmWindow inherited;
	friend class BmMailViewFilter;

public:
	// creator-func, c'tors and d'tor:
	static BmMailEditWin* CreateInstance();
	BmMailEditWin();
	~BmMailEditWin();

	// native methods:
	void EditMail( BmMailRef* ref);
	void EditMail( BmMail* mail);
	void SetEditMode( int32 mode);

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	
	// getters:
	bool IsInRawMode() const				{ return mRawMode; }

private:
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	void CreateGUI();
	MMenuBar* CreateMenu();

	bool CreateMailFromFields();
	bool SaveAndReloadMail();
	void SetFieldsFromMail( BmMail* mail);

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
	bool mRawMode;
	MView* mOuterGroup;

	// Hide copy-constructor and assignment:
	BmMailEditWin( const BmMailEditWin&);
	BmMailEditWin operator=( const BmMailEditWin&);
};



class BmMailViewFilter : public BMessageFilter {
	typedef BMessageFilter inherited;
public:
	// c'tor:
	BmMailViewFilter( int32 msgCmd, BmMailEditWin* editWin);
	
	// overrides of message-filter-base:
	filter_result Filter( BMessage* msg, BHandler** target);

private:
	BmMailEditWin* mEditWin;

	// Hide copy-constructor and assignment:
	BmMailViewFilter( const BmMailViewFilter&);
	BmMailViewFilter operator=( const BmMailViewFilter&);
};


#endif
