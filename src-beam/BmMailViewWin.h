/*
	BmMailViewWin.h
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


#ifndef _BmMailViewWin_h
#define _BmMailViewWin_h

#include "BmWindow.h"

class MMenuBar;

class BmMailRefView;
class BmMailView;
class BmMailViewContainer;
class BmMenuController;
class BmToolbarButton;
class UserResizeSplitView;

class BmMailViewWin : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_HSPLITTER;

public:
	// creator-func, c'tors and d'tor:
	static BmMailViewWin* CreateInstance( BmMailRef* mailRef=NULL);
	BmMailViewWin( BmMailRef* mailRef=NULL);
	~BmMailViewWin();

	void ShowMail( BmMailRef* mailRef, bool async=true);

	// overrides of BmWindow base:
	void BeginLife();
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);
	
	// getters:
	BmMailView* MailView() const			{ return mMailView; }
	
private:
	void CreateGUI();
	MMenuBar* CreateMenu();

	BmMailRefView* mMailRefView;
	BmMailView* mMailView;
	
	UserResizeSplitView* mHorzSplitter;
	
	BmToolbarButton* mNewButton;
	BmToolbarButton* mReplyButton;
	BmToolbarButton* mForwardButton;
	BmToolbarButton* mPrintButton;
	BmToolbarButton* mTrashButton;
	
	MView* mOuterGroup;

	BmMenuController* mFilterMenu;

	static float nNextXPos;
	static float nNextYPos;

	// Hide copy-constructor and assignment:
	BmMailViewWin( const BmMailViewWin&);
	BmMailViewWin operator=( const BmMailViewWin&);
};


#endif
