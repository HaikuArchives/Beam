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
