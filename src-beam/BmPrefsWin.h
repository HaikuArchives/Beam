/*
	BmPrefsWin.h
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


#ifndef _BmPrefsWin_h
#define _BmPrefsWin_h

#include "ColumnListView.h"

#include "BmWindow.h"

class CLVContainerView;
class UserResizeSplitView;
class BmPrefsViewContainer;

class BmPrefsWin : public BmWindow
{
	typedef BmWindow inherited;

	// archival-fieldnames:
	static const char* const MSG_VSPLITTER = 	"bm:vspl";

public:
	// creator-func, c'tors and d'tor:
	static BmPrefsWin* CreateInstance();
	BmPrefsWin();
	~BmPrefsWin();

	// native methods:

	// overrides of BWindow base:
	void MessageReceived( BMessage*);
	bool QuitRequested();
	void Quit();

	static BmPrefsWin* theInstance;

protected:
	status_t ArchiveState( BMessage* archive) const;
	status_t UnarchiveState( BMessage* archive);

private:
	CLVContainerView* CreatePrefsListView( minimax minmax, int32 width, int32 height);
	void PrefsListSelectionChanged( int32 numSelected);

	ColumnListView* mPrefsListView;
	BmPrefsViewContainer* mPrefsViewContainer;
	UserResizeSplitView* mVertSplitter;
	
	bool mModified;

	// Hide copy-constructor and assignment:
	BmPrefsWin( const BmPrefsWin&);
	BmPrefsWin operator=( const BmPrefsWin&);
};

#define ThePrefsWin BmPrefsWin::theInstance

#endif
