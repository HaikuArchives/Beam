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

#define BM_EACH_BCC_CHANGED 				'bmBC'
#define BM_PREFER_USER_AGENT_CHANGED 	'bmUA'
#define BM_GENERATE_MSGIDS_CHANGED 		'bmGI'
#define BM_QP_SAFE_CHANGED	 				'bmQP'
#define BM_ATTACH_VCARDS_CHANGED 		'bmAV'
#define BM_ENCODING_SELECTED		 		'bmES'
#define BM_FORWARD_TYPE_SELECTED		 	'bmFS'
#define BM_QUOTE_FORMATTING_SELECTED	'bmQS'
#define BM_ALLOW_8_BIT_CHANGED			'bm8C'
#define BM_HARD_WRAP_AT_78_CHANGED		'bmHW'
#define BM_HARD_WRAP_CHANGED				'bmHC'

class BmCheckControl;
class BmMenuControl;
class BmTextControl;
/*------------------------------------------------------------------------------*\
	BmPrefsMailConstrView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsMailConstrView : public BmPrefsView {
	typedef BmPrefsView inherited;

public:
	// c'tors and d'tor:
	BmPrefsMailConstrView();
	virtual ~BmPrefsMailConstrView();
	
	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

private:

	BmCheckControl* mGenerateIDsControl;
	BmCheckControl* mMakeQpSafeControl;
	BmCheckControl* mSpecialForEachBccControl;
	BmCheckControl* mPreferUserAgentControl;
	BmCheckControl* mAllow8BitControl;
	BmCheckControl* mHardWrapAt78Control;
	BmCheckControl* mHardWrapControl;
	BmTextControl* mMaxLineLenControl;
	BmMenuControl* mDefaultEncodingControl;
	BmTextControl* mQuotingStringControl;
	BmMenuControl* mQuoteFormattingControl;

	BmMenuControl* mDefaultForwardTypeControl;
	BmTextControl* mForwardIntroStrControl;
	BmTextControl* mForwardSubjectStrControl;
	BmTextControl* mForwardSubjectRxControl;
	BmCheckControl* mDontAttachVCardsControl;

	BmTextControl* mReplyIntroStrControl;
	BmTextControl* mReplySubjectStrControl;
	BmTextControl* mReplySubjectRxControl;

	// Hide copy-constructor and assignment:
	BmPrefsMailConstrView( const BmPrefsMailConstrView&);
	BmPrefsMailConstrView operator=( const BmPrefsMailConstrView&);
};

#endif
