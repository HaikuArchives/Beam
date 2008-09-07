/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailEditWin_h
#define _BmMailEditWin_h

#include <Entry.h>
#include <MessageFilter.h>

#include "BmRefManager.h"
#include "BmWindow.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
enum {
	BM_TO_CLEAR 			= 'bMYa',
	BM_TO_ADDED				= 'bMYb',
	BM_CC_CLEAR 			= 'bMYc',
	BM_CC_ADDED				= 'bMYd',
	BM_BCC_CLEAR 			= 'bMYe',
	BM_BCC_ADDED			= 'bMYf',
	BM_CHARSET_SELECTED	= 'bMYg',
	BM_FROM_SET	 			= 'bMYi',
	BM_SMTP_SELECTED		= 'bMYj',
	BM_EDIT_HEADER_DONE	= 'bMYk',
	BM_SHOWDETAILS1		= 'bMYl',
	BM_SHOWDETAILS2		= 'bMYm',
	BM_SHOWDETAILS3		= 'bMYn',
	BM_SIGNATURE_SELECTED= 'bMYo',
	BM_TO_REMOVE			= 'bMYp',
	BM_CC_REMOVE			= 'bMYq',
	BM_BCC_REMOVE			= 'bMYr',
	BM_FILEINTO_SELECTED	= 'bMYs'
};


class MMenuBar;

class BmCheckControl;
class BmIdentity;
class BmMail;
class BmMailView;
class BmMailRef;
class BmMailViewContainer;
class BmMenuControl;
class BmMenuController;
class BmMenuControllerBase;
class BmTextControl;
class BmToolbarButton;
class BmToolbar;
class HGroup;
class MPictureButton;
class Space;
class VGroup;
class BFilePanel;


typedef vector<BmString> BmStringVect;
BmString SelectEmailForPerson( const BmStringVect& emails);

/*------------------------------------------------------------------------------*\
	BmMailEditWin
		-	
\*------------------------------------------------------------------------------*/
class BmMailEditWin : public BmWindow
{
	typedef BmWindow inherited;

	friend class BmPeopleDropMsgFilter;
	friend class BmGuiRoster;

	// state-archival members:
	static const char* const MSG_DETAIL1;
	static const char* const MSG_DETAIL2;
	static const char* const MSG_DETAIL3;

public:
	// creator-funcs, c'tors and d'tor:
	static BmMailEditWin* CreateInstance( BmMailRef* mailRef=NULL);
	static BmMailEditWin* CreateInstance( BmMail* mail=NULL);
	~BmMailEditWin();

	// overrides of BmWindow base:
	void BeginLife();
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);
	
	// getters:
	BmRef<BmMail> CurrMail() const;

	// msg-members:
	static const char* const MSG_CONTROL;
	static const char* const MSG_ADDRESS;

private:
	// hide constructors:
	BmMailEditWin();
	BmMailEditWin( BmMailRef* mailRef, BmMail* mail=NULL);

	// native methods:
	void AddAddressToTextControl( BmTextControl* cntrl, const BmString& email);
	void RemoveAddressFromTextControl( BmTextControl* cntrl, 
												  const BmString& email);
	
	void SetDetailsButton( int32 nr, int32 newVal);
	void EditMail( BmMailRef* ref);
	void EditMail( BmMail* mail);
	BmMailViewContainer* CreateMailView( minimax minmax, BRect frame);
	void CreateGUI();
	MMenuBar* CreateMenu();

	bool CreateMailFromFields( bool hardWrapIfNeeded=true);
	bool SaveMail( bool hardWrapIfNeeded=true);
	void SetFieldsFromMail( BmMail* mail, BmIdentity* identity = NULL);

	void SendMail( bool sendNow);
	bool EditHeaders();
	void HandleFromSet( const BmString& from);

	static void RebuildPeopleMenu( BmMenuControllerBase* peopleMenu);

	BmMailView* mMailView;
	
	MPictureButton* mShowDetails1Button;
	MPictureButton* mShowDetails2Button;
	MPictureButton* mShowDetails3Button;

	BmToolbarButton* mSendButton;
	BmToolbarButton* mSaveButton;
	BmToolbarButton* mNewButton;
	BmToolbarButton* mAttachButton;
	BmToolbar* mToolbar;
	
	BmTextControl* mBccControl;
	BmTextControl* mCcControl;
	BmTextControl* mToControl;

	BmTextControl* mFromControl;
	BmTextControl* mReplyToControl;
	BmTextControl* mSenderControl;
	BmTextControl* mSubjectControl;
	
	BmMenuControl* mCharsetControl;
	BmMenuControl* mSmtpControl;
	BmMenuControl* mSignatureControl;
	BmMenuControl* mFileIntoControl;
	
	BmCheckControl* mEditHeaderControl;

	bool mShowDetails1;
	bool mShowDetails2;
	bool mShowDetails3;
	bool mPrefsShowDetails1;
	bool mPrefsShowDetails2;
	bool mPrefsShowDetails3;
	VGroup* mDetails1Group;
	HGroup* mDetails2Group;
	VGroup* mDetails3Group;
	HGroup* mSubjectGroup;
	VGroup* mOuterGroup;
	Space* mSeparator;
	bool mModified;
	bool mHasNeverBeenSaved;
	bool mHasBeenSent;

	BFilePanel* mAttachPanel;

	static float nNextXPos;
	static float nNextYPos;

	// Hide copy-constructor and assignment:
	BmMailEditWin( const BmMailEditWin&);
	BmMailEditWin operator=( const BmMailEditWin&);
};



#endif
