/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsMailConstrView_h
#define _BmPrefsMailConstrView_h

#include "BmPrefsView.h"

class BFilePanel;
class BmCheckControl;
class BmMenuControl;
class BmTextControl;
class MButton;
/*------------------------------------------------------------------------------*\
	BmPrefsMailConstrView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsMailConstrView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_CHARSET_SELECTED		 		= 'bmCS',
		BM_FORWARD_TYPE_SELECTED		= 'bmFS',
		BM_QUOTE_FORMATTING_SELECTED	= 'bmQS',
		BM_HARD_WRAP_CHANGED				= 'bmHC',
		BM_USED_CHARSET_SELECTED		= 'bmUS',
		BM_LOOK_IN_PEOPLE_CHANGED		= 'bmLP',
		BM_ADD_PEOPLE_NAME_CHANGED		= 'bmPN',
		BM_SELECT_PEOPLE_FOLDER			= 'bmSP',
		BM_UNDO_MODE_SELECTED		 	= 'bmUM'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsMailConstrView();
	virtual ~BmPrefsMailConstrView();
	
	// native methods:
	void SetupUsedCharsetsMenu();
	void SetupUsedCharsetsPrefs();

	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

private:

	BmString PeopleFolderButtonLabel();

	BmCheckControl* mHardWrapControl;
	BmTextControl* mMaxLineLenControl;
	BmTextControl* mQuotingStringControl;
	BmMenuControl* mQuoteFormattingControl;
	BmMenuControl* mUndoModeControl;

	BmMenuControl* mDefaultCharsetControl;
	BmMenuControl* mUsedCharsetsControl;

	BmTextControl* mForwardSubjectStrControl;
	BmTextControl* mForwardIntroStrControl;

	BmTextControl* mReplySubjectStrControl;
	BmTextControl* mReplyIntroStrControl;
	BmTextControl* mReplyIntroStrPrivateControl;

	BmCheckControl* mAddNameToPeopleControl;
	BmCheckControl* mLookInPeopleFolderControl;
	MButton* mPeopleFolderButton;
	BFilePanel* mPeoplePanel;

	// Hide copy-constructor and assignment:
	BmPrefsMailConstrView( const BmPrefsMailConstrView&);
	BmPrefsMailConstrView operator=( const BmPrefsMailConstrView&);
};

#endif
