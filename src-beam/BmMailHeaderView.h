/*
	BmMailHeaderView.h
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


#ifndef _BmMailHeaderView_h
#define _BmMailHeaderView_h

#include <View.h>

#include "BmMailHeader.h"
#include "BmRefManager.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailHeaderView:
\*------------------------------------------------------------------------------*/
#define BM_HEADERVIEW_SMALL			'bmfa'
#define BM_HEADERVIEW_LARGE			'bmfb'
#define BM_HEADERVIEW_FULL				'bmfc'

/*------------------------------------------------------------------------------*\
	BmMailHeaderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailHeaderView : public BView {
	typedef BView inherited;

	static const int SMALL_HEADERS = 0;
	static const int LARGE_HEADERS = 1;
	static const int FULL_HEADERS = 2;

	// archival-fieldnames:
	static const char* const MSG_MODE = 		"bm:mode";

public:
	// c'tors and d'tor:
	BmMailHeaderView( BmMailHeader* header);
	~BmMailHeaderView();

	// native methods:
	void ShowHeader( BmMailHeader* header, bool invalidate=true);
	void ShowMenu( BPoint point);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);
	inline float FixedWidth() 				{ return 5000; }

	// overrides of BView base:
	void Draw( BRect bounds);
	void MessageReceived( BMessage* msg);
	void MouseDown(BPoint point);

private:
	BmRef<BmMailHeader> mMailHeader;
	int16 mDisplayMode;							// 0=small, 2=large, anyother=medium
	BFont* mFont;								// font to be used for header-fields

	// Hide copy-constructor and assignment:
	BmMailHeaderView( const BmMailHeaderView&);
	BmMailHeaderView operator=( const BmMailHeaderView&);
};

#endif
