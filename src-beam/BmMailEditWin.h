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
class MCheckBox;
class MPictureButton;

class BmMailEditWin : public BmWindow
{
	typedef BmWindow inherited;
	friend class BmMailViewFilter;

public:
	// creator-funcs, c'tors and d'tor:
	static BmMailEditWin* CreateInstance( BmMailRef* mailRef=NULL);
	static BmMailEditWin* CreateInstance( BmMail* mail=NULL);
	BmMailEditWin( BmMailRef* mailRef=NULL, BmMail* mail=NULL);
	~BmMailEditWin();

	// native methods:
	void EditMail( BmMailRef* ref);
	void EditMail( BmMail* mail);

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	
	// getters:
	BmRef<BmMail> CurrMail() const;

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
	
	MCheckBox* mEditHeaderControl;

	bool mShowDetails;
	MView* mOuterGroup;
	bool mModified;
	uint32 mModificationID;

	// Hide copy-constructor and assignment:
	BmMailEditWin( const BmMailEditWin&);
	BmMailEditWin operator=( const BmMailEditWin&);
};



#endif
