/*
	BmPrefsGeneralView.h
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


#ifndef _BmPrefsGeneralView_h
#define _BmPrefsGeneralView_h

#include "BmMailFolder.h"
#include "BmMailRefView.h"
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
		BM_RESTORE_FOLDER_STATES_CHANGED = 'bmRF',
		BM_DYNAMIC_STATUS_WIN_CHANGED 	= 'bmDS',
		BM_CACHE_REFS_DISK_CHANGED	 		= 'bmCD',
		BM_CACHE_REFS_MEM_CHANGED 			= 'bmCM',
		BM_INOUT_AT_TOP_CHANGED 			= 'bmIO',
		BM_USE_DESKBAR_CHANGED 				= 'bmDC',
		BM_SHOW_TOOLTIPS_CHANGED 			= 'bmTC',
		BM_MAKE_BEAM_STD_APP	 				= 'bmBS',
		BM_SELECT_MAILBOX		 				= 'bmSM',
		BM_BEEP_NEW_MAIL_CHANGED			= 'bmBN',
		BM_WORKSPACE_SELECTED				= 'bmWS',
		BM_TOOLBAR_LABEL_SELECTED			= 'bmTL',
		BM_SHOW_TOOLBAR_ICONS_CHANGED		= 'bmTI',
		BM_SHOW_TOOLBAR_BORDER_CHANGED	= 'bmTB',
		BM_SHOW_ALERTS_FOR_ERRORS_CHANGED= 'bmAE',
		BM_CLOSE_VIEWWIN_CHANGED			= 'bmCW',
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

	BmTextControl* mMailMoverShowControl;
	BmTextControl* mPopperRemoveControl;
	BmTextControl* mSmtpRemoveControl;
	BmTextControl* mRemoveFailedControl;
	BmTextControl* mNetBufSizeSendControl;
	BmTextControl* mNetRecvTimeoutControl;
	BmCheckControl* mRestoreFolderStatesControl;
	BmCheckControl* mDynamicStatusWinControl;
	BmCheckControl* mCacheRefsInMemControl;
	BmCheckControl* mCacheRefsOnDiskControl;
	BmCheckControl* mInOutAtTopControl;
	BmCheckControl* mUseDeskbarControl;
	BmCheckControl* mShowTooltipsControl;
	BmCheckControl* mBeepNewMailControl;
	BmCheckControl* mShowAlertForErrorsControl;
	BmCheckControl* mShowToolbarIconsControl;
	BmCheckControl* mShowToolbarBorderControl;
	BmCheckControl* mCloseViewWinControl;
	BmCheckControl* mListviewLikeTrackerControl;
	BmMenuControl* mWorkspaceControl;
	BmMenuControl* mToolbarLabelControl;

	MButton* mMailboxButton;

	BFilePanel* mMailboxPanel;


	// Hide copy-constructor and assignment:
	BmPrefsGeneralView( const BmPrefsGeneralView&);
	BmPrefsGeneralView operator=( const BmPrefsGeneralView&);
};

#endif
