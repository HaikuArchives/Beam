/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmRulerView_h
#define _BmRulerView_h

#include <View.h>

/*------------------------------------------------------------------------------*\
	types of messages sent by a rulerview:
\*------------------------------------------------------------------------------*/
enum {
	BM_RULERVIEW_NEW_POS		= 'bmra'
							// the user has moved the right-marging-indicator
};

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
	void SetMailViewFont( const BFont& font);

	// overrides of BView base:
	void Draw( BRect bounds);
	void MouseDown(BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint point);

	// getters:
	int32 IndicatorPos() const				{ return mIndicatorPos; }

	//	message component definitions:
	static const char* const MSG_NEW_POS;

private:
	// native methods:
	void SetIndicatorPixelPos( float pixelPos);

	BFont mMailViewFont;
	int32 mIndicatorPos;
	bool mIndicatorGrabbed;
	float mSingleCharWidth;
	static const float nXOffset;

	// Hide copy-constructor and assignment:
	BmRulerView( const BmRulerView&);
	BmRulerView operator=( const BmRulerView&);
};

#endif
