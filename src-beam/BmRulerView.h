/*
	BmRulerView.h
		$Id$
*/

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
	void SetIndicatorPos( float pixelPos);

	// overrides of BView base:
	void Draw( BRect bounds);
	void MouseDown(BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *msg);
	void MouseUp(BPoint point);

	//	message component definitions:
	static const char* const MSG_NEW_POS = "bm:newpos";

private:
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
