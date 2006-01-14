/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsGeneralView_h
#define _BmPrefsGeneralView_h

#include "BmPrefsView.h"



class BFilePanel;
class BmTextControl;
class BmCheckControl;
class BmMenuControl;
class MButton;
/*------------------------------------------------------------------------------*\
	BmPrefsGeneralView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsGeneralView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_SHOW_TOOLTIPS_CHANGED 			= 'bmTC',
		BM_MAKE_BEAM_STD_APP	 				= 'bmBS',
		BM_SELECT_MAILBOX		 				= 'bmSM',
		BM_SELECT_ICONBOX		 				= 'bmSI',
		BM_WORKSPACE_SELECTED				= 'bmWS',
		BM_TOOLBAR_LABEL_SELECTED			= 'bmTL',
		BM_SHOW_TOOLBAR_BORDER_CHANGED	= 'bmTB',
		BM_LISTVIEW_LIKE_TRACKER_CHANGED	= 'bmLT'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsGeneralView();
	virtual ~BmPrefsGeneralView();
	
	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void WriteStateInfo();
	void SaveData();
	void UndoChanges();
	void SetDefaults();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);
	
	static const char* const MSG_WORKSPACE;

private:
	BmString MailboxButtonLabel();
	BmString IconboxButtonLabel();

	BmCheckControl* mShowTooltipsControl;
	BmCheckControl* mShowToolbarBorderControl;
	BmCheckControl* mListviewLikeTrackerControl;
	BmMenuControl* mWorkspaceControl;
	BmMenuControl* mToolbarLabelControl;

	MButton* mMailboxButton;

	BFilePanel* mMailboxPanel;

	MButton* mIconboxButton;

	BFilePanel* mIconboxPanel;

	// Hide copy-constructor and assignment:
	BmPrefsGeneralView( const BmPrefsGeneralView&);
	BmPrefsGeneralView operator=( const BmPrefsGeneralView&);
};

#endif
