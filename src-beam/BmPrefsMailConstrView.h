/*
	BmPrefsMailConstrView.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


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
		BM_EACH_BCC_CHANGED 				= 'bmBC',
		BM_PREFER_USER_AGENT_CHANGED 	= 'bmUA',
		BM_GENERATE_MSGIDS_CHANGED 	= 'bmGI',
		BM_QP_SAFE_CHANGED	 			= 'bmQP',
		BM_ATTACH_VCARDS_CHANGED 		= 'bmAV',
		BM_CHARSET_SELECTED		 		= 'bmCS',
		BM_FORWARD_TYPE_SELECTED		= 'bmFS',
		BM_QUOTE_FORMATTING_SELECTED	= 'bmQS',
		BM_ALLOW_8_BIT_CHANGED			= 'bm8C',
		BM_IMPORT_EXPORT_UTF8_CHANGED	= 'bmIE',
		BM_HARD_WRAP_AT_78_CHANGED		= 'bmHW',
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

	BmCheckControl* mGenerateIDsControl;
	BmCheckControl* mMakeQpSafeControl;
	BmCheckControl* mSpecialForEachBccControl;
	BmCheckControl* mPreferUserAgentControl;
	BmCheckControl* mAllow8BitControl;
	BmCheckControl* mImportExportUtf8Control;
	BmCheckControl* mHardWrapAt78Control;
	BmCheckControl* mHardWrapControl;
	BmTextControl* mMaxLineLenControl;
	BmTextControl* mQuotingStringControl;
	BmMenuControl* mQuoteFormattingControl;
	BmMenuControl* mUndoModeControl;

	BmMenuControl* mDefaultCharsetControl;
	BmMenuControl* mUsedCharsetsControl;

	BmMenuControl* mDefaultForwardTypeControl;
	BmTextControl* mForwardIntroStrControl;
	BmTextControl* mForwardSubjectStrControl;
	BmTextControl* mForwardSubjectRxControl;
	BmCheckControl* mDontAttachVCardsControl;

	BmTextControl* mReplyIntroStrControl;
	BmTextControl* mReplyIntroStrPrivateControl;
	BmTextControl* mReplySubjectStrControl;
	BmTextControl* mReplySubjectRxControl;

	BmCheckControl* mAddNameToPeopleControl;
	BmCheckControl* mLookInPeopleFolderControl;
	MButton* mPeopleFolderButton;
	BFilePanel* mPeoplePanel;

	// Hide copy-constructor and assignment:
	BmPrefsMailConstrView( const BmPrefsMailConstrView&);
	BmPrefsMailConstrView operator=( const BmPrefsMailConstrView&);
};

#endif
