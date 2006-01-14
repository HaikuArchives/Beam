/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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

	// Hide copy-constructor and assignment:
	BmPrefsMailReadView( const BmPrefsMailReadView&);
	BmPrefsMailReadView operator=( const BmPrefsMailReadView&);
};

#endif
