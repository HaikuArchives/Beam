/*
	BmLogWindow.h
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


#ifndef _BmLogWindow_h
#define _BmLogWindow_h

#include <MWindow.h>

#include "BmLogHandler.h"

enum {
	BM_LOGENTRY_ADDED = 'bmle'
};

class BmLogWindow : public MWindow
{
	typedef MWindow inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmLogWindow* CreateAndStartInstanceFor( const char* logfileName,
																  bool showUponNews=false);
	BmLogWindow( const BRect& frame, const BmString& title, 
					 const char* logfileName, bool showUponNews);
	~BmLogWindow();
	
	// overrides of BWindow-base:
	void MessageReceived( BMessage* msg);
	bool QuitRequested();

private:
	BmString mLogfileName;
	MTextView* mLogView;
	bool mShowUponNews;

	static int32 nWinCount;

	// Hide copy-constructor and assignment:
	BmLogWindow( const BmLogWindow&);
	BmLogWindow operator=( const BmLogWindow&);
};

#endif
