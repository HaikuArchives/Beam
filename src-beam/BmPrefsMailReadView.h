/*
	BmPrefsMailReadView.h
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


#ifndef _BmPrefsMailReadView_h
#define _BmPrefsMailReadView_h

#include "BmPrefsView.h"

class BmCheckControl;
class BmTextControl;
class BmMenuControl;
/*------------------------------------------------------------------------------*\
	BmPrefsMailReadView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsMailReadView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_USE_SWATCHTIME_CHANGED				= 'bmST',
		BM_TIMEMODE_IN_HEADERVIEW_SELECTED 	= 'bmTM',
		BM_SHOW_DECODED_LENGTH_CHANGED		= 'bmSD',
		BM_SELECT_NEXT_ON_DELETE_CHANGED		= 'bmSN'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsMailReadView();
	virtual ~BmPrefsMailReadView();
	
	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

private:

	BmTextControl* mMarkAsReadDelayControl;
	BmTextControl* mHeaderListSmallControl;
	BmTextControl* mHeaderListLargeControl;
	BmCheckControl* mShowDecodedLengthControl;
	BmMenuControl* mTimeModeInHeaderViewControl;
	BmCheckControl* mUseSwatchTimeInRefViewControl;
	BmCheckControl* mSelectNextOnDeleteControl;

	BmTextControl* mMimeTypeTrustInfoControl;

	// Hide copy-constructor and assignment:
	BmPrefsMailReadView( const BmPrefsMailReadView&);
	BmPrefsMailReadView operator=( const BmPrefsMailReadView&);
};

#endif
