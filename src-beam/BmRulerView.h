/*
	BmRulerView.h
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


#ifndef _BmRulerView_h
#define _BmRulerView_h

#include <View.h>

/*------------------------------------------------------------------------------*\
	types of messages sent by a rulerview:
\*------------------------------------------------------------------------------*/
#define BM_RULERVIEW_NEW_POS		'bmra'
							// the user has moved the right-marging-indicator

/*------------------------------------------------------------------------------*\
	BmRulerView
		-	
\*------------------------------------------------------------------------------*/
class BmRulerView : public BView {
	typedef BView inherited;

public:
	// c'tors and d'tor:
	BmRulerView( const BFont& font);
	~BmRulerView();

	// native methods:
	void SetIndicatorPos( int32 pos);

	// overrides of BView base:
	void Draw( BRect bounds);
	void MouseDown(BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint point);

	//	message component definitions:
	static const char* const MSG_NEW_POS = "bm:newpos";

private:
	// native methods:
	void SetIndicatorPixelPos( float pixelPos);

	BFont mMailViewFont;
	int32 mIndicatorPos;
	bool mIndicatorGrabbed;
	float mSingleCharWidth;
	static const float nXOffset = 4.0;

	// Hide copy-constructor and assignment:
	BmRulerView( const BmRulerView&);
	BmRulerView operator=( const BmRulerView&);
};

#endif
