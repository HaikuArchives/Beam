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


#define BM_STRIPED_LISTVIEW_CHANGED 		'bmSC'
#define BM_RESTORE_FOLDER_STATES_CHANGED 	'bmRF'
#define BM_DYNAMIC_STATUS_WIN_CHANGED 		'bmDS'
#define BM_CACHE_REFS_DISK_CHANGED	 		'bmCD'
#define BM_CACHE_REFS_MEM_CHANGED 			'bmCM'
#define BM_INOUT_AT_TOP_CHANGED 				'bmIO'
#define BM_USE_DESKBAR_CHANGED 				'bmDC'
#define BM_SHOW_TOOLTIPS_CHANGED 			'bmTC'
#define BM_MAKE_BEAM_STD_APP	 				'bmBS'
#define BM_SELECT_MAILBOX		 				'bmSM'
#define BM_BEEP_NEW_MAIL_CHANGED				'bmBN'
#define BM_WORKSPACE_SELECTED					'bmWS'

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

public:
	// c'tors and d'tor:
	BmPrefsGeneralView();
	virtual ~BmPrefsGeneralView();
	
	// overrides of BmPrefsView base:
	void Initialize();
	void WriteStateInfo();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);
	
	static const char* const MSG_WORKSPACE = "wspace";

private:
	CLVContainerView* CreateMailRefLayoutView( minimax minmax, int32 width, int32 height);
	BString MailboxButtonLabel();

	BmMailRefView* mLayoutView;
	BmTextControl* mMailMoverShowControl;
	BmTextControl* mPopperRemoveControl;
	BmTextControl* mSmtpRemoveControl;
	BmTextControl* mRemoveFailedControl;
	BmTextControl* mNetBufSizeSendControl;
	BmTextControl* mNetRecvTimeoutControl;
	BmCheckControl* mRestoreFolderStatesControl;
	BmCheckControl* mStripedListViewControl;
	BmCheckControl* mDynamicStatusWinControl;
	BmCheckControl* mCacheRefsInMemControl;
	BmCheckControl* mCacheRefsOnDiskControl;
	BmCheckControl* mInOutAtTopControl;
	BmCheckControl* mUseDeskbarControl;
	BmCheckControl* mShowTooltipsControl;
	BmCheckControl* mBeepNewMailControl;
	BmMenuControl* mWorkspaceControl;
	MButton* mMailboxButton;

	BFilePanel* mMailboxPanel;

	BmRef<BmMailFolder> mFolder;
	BmRef<BmMailRefList> mRefList;
	
	// Hide copy-constructor and assignment:
	BmPrefsGeneralView( const BmPrefsGeneralView&);
	BmPrefsGeneralView operator=( const BmPrefsGeneralView&);
};

#endif
