/*
	BmWindow.h
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


#ifndef _BmWindow_h
#define _BmWindow_h

#include <String.h>

#ifdef B_BEOS_VERSION_DANO
	class BPopUpMenu;
#endif
#include <MWindow.h>

class BmWindow : public MWindow
{
	typedef MWindow inherited;

	static const char* const MSG_FRAME = 		"bm:frm";

public:
	// creator-func, c'tors and d'tor:
	BmWindow( const char* statfileName, BRect frame, const char* title,
				 window_look look, window_feel feel, uint32 flags);
	~BmWindow();

	// native methods:
	virtual status_t ArchiveState( BMessage* archive) const;
	virtual status_t UnarchiveState( BMessage* archive);
	virtual bool ReadStateInfo();
	virtual bool WriteStateInfo();
	virtual void BeginLife()				{}

	// overrides of BWindow-base:
	void Quit();
	void Show();
	
protected:
	BString mStatefileName;
	bool mLifeHasBegun;

	// Hide copy-constructor and assignment:
	BmWindow( const BmWindow&);
	BmWindow operator=( const BmWindow&);
};


#endif
