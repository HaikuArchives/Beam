//		$Id$
//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.

#ifndef _SGB_BETTER_SCROLL_VIEW_H_
#define _SGB_BETTER_SCROLL_VIEW_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <ScrollView.h>

#include <layout.h>

//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "BmGuiBase.h"

#include "BmBusyView.h"
#include "BmCaption.h"
#include "ScrollViewCorner.h"

typedef uint32 BmScrollViewFlags;
enum  {
	BM_SV_H_SCROLLBAR = 1<<0,
	BM_SV_V_SCROLLBAR = 1<<1,
	BM_SV_CORNER = 1<<2,
	BM_SV_BUSYVIEW = 1<<3,
	BM_SV_CAPTION = 1<<4
};


//******************************************************************************************************
//**** CLASS DECLARATIONS
//******************************************************************************************************
class IMPEXPBMGUIBASE BetterScrollView : public MView, public BScrollView
{
public:
	BetterScrollView(minimax minmax, BView* target, BmScrollViewFlags svFlags,
						  const char* captionMaxText = NULL);
	virtual ~BetterScrollView();

	void SetBusy();
	void UnsetBusy();
	void PulseBusyView();
	void SetErrorText( const BmString& text);

	void SetCaptionText( const char* text);
	void SetCaptionWidth( float width) 	{ mCaptionWidth = width; }
	BmCaption* Caption() 					{ return mCaption; }

	virtual void SetDataRect(BRect data_rect, bool scrolling_allowed = true);
	inline BRect DataRect() {return mDataRect;}

	void Draw( BRect bounds);
	void FrameResized(float new_width, float new_height);
	void AttachedToWindow();
	void WindowActivated(bool active);
	status_t SetBorderHighlighted( bool highlighted);

	// overrides of MView base:
	BRect layout( BRect);
	minimax layoutprefs();
	
protected:
	void UpdateScrollBars(bool scrolling_allowed);

	BmCaption* mCaption;
	float mCaptionWidth;
	BmBusyView* mBusyView;

	BRect mDataRect;
	BScrollBar* mHScroller;
	BScrollBar* mVScroller;
	ScrollViewCorner* mScrollViewCorner;
	BView* mTarget;
};


#endif
