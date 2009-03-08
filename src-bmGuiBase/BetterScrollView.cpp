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

#include <stdio.h>

//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "BmBasics.h"
#include "BetterScrollView.h"
#include "Colors.h"


//******************************************************************************************************
//**** BetterScrollView CLASS
//******************************************************************************************************

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BetterScrollView::BetterScrollView(minimax minmax, BView *target, 
	BmScrollViewFlags svFlags, const char* captionMaxText)
	: BScrollView("BetterScrollView", target,
					  B_FOLLOW_NONE, B_FRAME_EVENTS | B_WILL_DRAW, 
					  svFlags & BM_SV_H_SCROLLBAR, svFlags & BM_SV_V_SCROLLBAR,
					  B_FANCY_BORDER)
	, mCaption( NULL)
	, mCaptionWidth( 0)
	, mBusyView( NULL)
	, mTarget(target)
{
	ct_mpm = minmax;
	mDataRect.Set(-1,-1,-1,-1);
	mHScroller = ScrollBar(B_HORIZONTAL);
	mVScroller = ScrollBar(B_VERTICAL);
	if (BeamOnDano) {
		// on Dano/Zeta, a fancy border has a border-size of three instead of two,
		// so we need to adjust the target's size in order to compensate...
		if (mTarget) {
			mTarget->MoveBy( -1.0, -1.0);
			mTarget->ResizeBy( 1.0, 1.0);
		}
		// ...and move and resize the h-scroller's size...
		if (mHScroller) {
#ifdef B_ZETA_VERSION_1_2_0
			mHScroller->MoveBy( -1.0, 0.0);
#else
			mHScroller->MoveBy( -1.0, 1.0);
#endif
			mHScroller->ResizeBy( 2.0, 0.0);
		}
		// ...and finally move and resize the v-scroller, too:
		if (mVScroller) {
			float hOffs = Bounds().right - B_V_SCROLL_BAR_WIDTH - 1
						- mVScroller->Frame().left;
				// workaround for difference between Zeta 1.0.1 and older versions
			mVScroller->MoveBy( hOffs, -1.0);
			mVScroller->ResizeBy( 0.0, 2.0);
		}
	} else {
		// on R5, we need to correct the h-scroller-position slightly:
		if (mHScroller) {
			mHScroller->MoveBy( 1.0, 0.0);
			mHScroller->ResizeBy( -1.0, 0.0);
		}
	}
	if (svFlags & BM_SV_CORNER) {
		BRect bounds = Bounds();
		mScrollViewCorner 
			= new ScrollViewCorner(bounds.right-B_V_SCROLL_BAR_WIDTH-1,
										  bounds.bottom-B_H_SCROLL_BAR_HEIGHT-1);
		if (BeamOnDano)
			// looks better on Zeta:
			mScrollViewCorner->ResizeBy(-1.0, -1.0);
		AddChild(mScrollViewCorner);
	}
	else
		mScrollViewCorner = NULL;

	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	BRect frame;
	BPoint LT;
	if (mHScroller)
		frame = mHScroller->Frame();
	else {
		frame = Bounds().InsetByCopy( 1.0, 1.0);
		frame.left += 1;
		frame.right -= 1;
		frame.bottom -= 2;
		frame.top = frame.bottom - B_H_SCROLL_BAR_HEIGHT;
	}
	if (svFlags & BM_SV_BUSYVIEW) {
		LT = frame.LeftTop();
		mBusyView = new BmBusyView( LT);
		BRect bvFrame = mBusyView->Frame();
		float bvWidth = 1+bvFrame.right-bvFrame.left;
		if (mHScroller) {
			// a horizontal scrollbar exists, we shrink it to make room 
			// for the busyview:
			mHScroller->ResizeBy( -bvWidth, 0.0);
			mHScroller->MoveBy( bvWidth, 0.0);
		}
		AddChild( mBusyView);
		frame.left += bvWidth;
	}
	if (svFlags & BM_SV_CAPTION) {
		if (captionMaxText)
			mCaptionWidth = 10 + be_plain_font->StringWidth(captionMaxText);
		LT = frame.LeftTop();
		if (mHScroller) {
			// a horizontal scrollbar exists, we shrink it to make room 
			// for the caption:
			mHScroller->ResizeBy( -mCaptionWidth, 0.0);
			mHScroller->MoveBy( mCaptionWidth, 0.0);
		} else {
			// no horizontal scrollbar, so the caption occupies all the 
			// remaining space:
			mCaptionWidth = frame.Width();
		}
		mCaption = new BmCaption( 
			BRect( LT.x, LT.y, LT.x+mCaptionWidth-1, LT.y+frame.Height()), ""
		);
		AddChild( mCaption);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BetterScrollView::~BetterScrollView()
{ }


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::AddActionView(BView* actionView)
{
	if (!actionView)
		return;

	AddChild( actionView);
	mActionViews.push_back(actionView);

	BRect b = Bounds();
	float top = 2.0;
	for(uint32 i=0; i<mActionViews.size(); ++i) {
		BView* actionView = mActionViews[i];
		actionView->MoveTo(b.right-B_V_SCROLL_BAR_WIDTH-1, top);
		top += actionView->Bounds().Height();
	}
	if (mVScroller) {
		mVScroller->ResizeTo(B_V_SCROLL_BAR_WIDTH, 
			mVScroller->Bounds().Height()-top);
		mVScroller->MoveTo(b.right-B_V_SCROLL_BAR_WIDTH-1, top+1);
	}
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::SetDataRect(BRect data_rect, bool scrolling_allowed)
{
	mDataRect = data_rect;
	UpdateScrollBars(scrolling_allowed);
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::FrameResized(float new_width, float new_height)
{
	BScrollView::FrameResized(new_width,new_height);
	Invalidate(BRect(MAX(0,new_width-20), MAX(0,new_height-20), new_width, new_height));
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::AttachedToWindow()
{
	BScrollView::AttachedToWindow();
	UpdateScrollBars(false);
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BetterScrollView::SetBorderHighlighted( bool highlighted)
{
	Draw( Bounds());
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::WindowActivated(bool active)
{
	if (mScrollViewCorner) {
		bool scEnabled = (
			mHScroller && mHScroller->Proportion() < 1.0 
			|| mVScroller && mVScroller->Proportion() < 1.0
		);
		mScrollViewCorner->SetEnabled( scEnabled && active);
		mScrollViewCorner->Draw(
			BRect(0.0, 0.0, B_V_SCROLL_BAR_WIDTH, B_H_SCROLL_BAR_HEIGHT)
		);
	}
	BScrollView::WindowActivated(active);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::Draw( BRect rect) {
	if (BeamOnDano)
		// avoid special drawing of border by calling BView::Draw()
		BView::Draw( rect);
#ifndef __HAIKU__
	else
		BScrollView::Draw( rect);
#else
	else {
		BRect bounds = Bounds();
		SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
		StrokeRect( bounds.InsetByCopy(1, 1));
	}
#endif
	if (mTarget) {
		BRect bounds = Bounds();
		rgb_color color = 
			IsFocus() || mTarget->IsFocus() 
				? keyboard_navigation_color()
				: ui_color(B_UI_PANEL_BACKGROUND_COLOR);
		if (mHScroller && mVScroller && !mScrollViewCorner) {
			BPoint lb( bounds.right-B_V_SCROLL_BAR_WIDTH, bounds.bottom);
			BPoint lt( bounds.right-B_V_SCROLL_BAR_WIDTH, 
						  bounds.bottom-B_H_SCROLL_BAR_HEIGHT);
			BPoint rt( bounds.right, bounds.bottom-B_H_SCROLL_BAR_HEIGHT);
			BeginLineArray(10);
			if (BeamOnDano) {
				// draw inner border (only left and top part, as the other
				// parts are drawn by scrollbars anyway):
				SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
				BRect border = bounds.InsetByCopy(1.0, 1.0);
				StrokeLine( border.LeftTop(), border.RightTop());
				StrokeLine( border.LeftTop(), border.LeftBottom());
			}
			AddLine( bounds.LeftBottom(), lb, color);
			AddLine( bounds.RightTop(), rt, color);
			AddLine( lb, lt, color);
			AddLine( lt, rt, color);
			AddLine( bounds.LeftTop(), bounds.RightTop(), color);
			AddLine( bounds.LeftTop(), bounds.LeftBottom(), color);
			EndLineArray();
		} else {
			SetHighColor( color);
			StrokeRect( bounds);
			if (BeamOnDano) {
				// draw inner border:
				SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
				StrokeRect( bounds.InsetByCopy(1.0, 1.0));
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::UpdateScrollBars(bool scrolling_allowed)
{
	//Figure out the bounds and scroll if necessary
	BRect view_bounds = mTarget->Bounds();

	float page_width, page_height, view_width, view_height;
	view_width = view_bounds.right-view_bounds.left;
	view_height = view_bounds.bottom-view_bounds.top;

	float min,max;
	if(scrolling_allowed)
	{
		//Figure out the width of the page rectangle
		page_width = mDataRect.right-mDataRect.left;
		page_height = mDataRect.bottom-mDataRect.top;
		if(view_width > page_width)
			page_width = view_width;
		if(view_height > page_height)
			page_height = view_height;
	
		//Adjust positions
		float delta_x = 0.0;
		if(mHScroller)
		{
			if(view_bounds.left < mDataRect.left)
				delta_x = mDataRect.left - view_bounds.left;
			else if(view_bounds.right > mDataRect.left+page_width)
				delta_x = mDataRect.left+page_width - view_bounds.right;
		}
	
		float delta_y = 0.0;
		if(mVScroller)
		{
			if(view_bounds.top < mDataRect.top)
				delta_y = mDataRect.top - view_bounds.top;
			else if(view_bounds.bottom > mDataRect.top+page_height)
				delta_y = mDataRect.top+page_height - view_bounds.bottom;
		}
	
		if(delta_x != 0.0 || delta_y != 0.0)
		{
			mTarget->ScrollTo(BPoint(view_bounds.left+delta_x,view_bounds.top+delta_y));
			view_bounds = Bounds();
		}
	}
	else
	{
		min = mDataRect.left;
		if(view_bounds.left < min)
			min = view_bounds.left;
		max = mDataRect.right;
		if(view_bounds.right > max)
			max = view_bounds.right;
		page_width = max-min;
		min = mDataRect.top;
		if(view_bounds.top < min)
			min = view_bounds.top;
		max = mDataRect.bottom;
		if(view_bounds.bottom > max)
			max = view_bounds.bottom;
		page_height = max-min;
	}

	//Figure out the ratio of the bounds rectangle width or height to the page rectangle width or height
	float width_prop = view_width/page_width;
	float height_prop = view_height/page_height;

	//Set the scroll bar ranges and proportions.  If the whole document is visible, inactivate the
	//slider
	bool active_scroller = false;
	if(mHScroller)
	{
		if(width_prop >= 1.0)
			mHScroller->SetRange(0.0,0.0);
		else
		{
			min = mDataRect.left;
			max = mDataRect.left + page_width - view_width;
			if(view_bounds.left < min)
				min = view_bounds.left;
			if(view_bounds.left > max)
				max = view_bounds.left;
			mHScroller->SetRange(min,max);
			mHScroller->SetSteps(ceil(view_width/20), view_width);
			active_scroller = true;
		}
		mHScroller->SetProportion(width_prop);
	}
	if (mVScroller)
	{
		if(height_prop >= 1.0)
			mVScroller->SetRange(0.0,0.0);
		else
		{
			min = mDataRect.top;
			max = mDataRect.top + page_height - view_height;
			if(view_bounds.top < min)
				min = view_bounds.top;
			if(view_bounds.top > max)
				max = view_bounds.top;
			mVScroller->SetRange(min,max);
			mVScroller->SetSteps(ceil(view_height/20), view_height);
			active_scroller = true;
		}
		mVScroller->SetProportion(height_prop);
	}
	if (mScrollViewCorner) {
		bool scEnabled = (
			mHScroller && mHScroller->Proportion() < 1.0 
			|| mVScroller && mVScroller->Proportion() < 1.0
		);
		mScrollViewCorner->SetEnabled( scEnabled);
		mScrollViewCorner->Invalidate();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::SetCaptionText( const char* text) {
	if (mCaption)
		mCaption->SetText( text);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::SetErrorText( const BmString& text) {
	if (mBusyView)
		mBusyView->SetErrorText( text);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::SetBusy() {
	if (mBusyView)
		mBusyView->SetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::UnsetBusy() {
	if (mBusyView)
		mBusyView->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BetterScrollView::PulseBusyView() {
	if (mBusyView) 
		mBusyView->Pulse();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
minimax BetterScrollView::layoutprefs()
{
	return mpm=ct_mpm;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BRect BetterScrollView::layout( BRect r) {
	MoveTo(r.LeftTop());
	ResizeTo(r.Width(),r.Height());
	if (mTarget) {
		BRect targetRect = mTarget->Frame();
		if (mHScroller || mBusyView || mCaption)
			targetRect.bottom = r.Height() - 1 - B_H_SCROLL_BAR_HEIGHT - 1;
		else
			targetRect.bottom = r.bottom-2;
		if (mVScroller)
			targetRect.right = r.Width() - 1 - B_V_SCROLL_BAR_WIDTH - 1;
		else
			targetRect.right = r.right-2;
		mTarget->ResizeTo(targetRect.Width(), targetRect.Height());
	}
	float fullCaptionWidth = r.Width() - 2.0;
	if (mBusyView) {
		BRect bvFrame = mBusyView->Frame();
		mBusyView->MoveTo( bvFrame.left, r.Height()-B_H_SCROLL_BAR_HEIGHT-1);
		fullCaptionWidth -= bvFrame.Width();
	}
	if (mCaption) {
		BRect cpFrame = mCaption->Frame();
		mCaption->MoveTo( cpFrame.left, r.Height()-B_H_SCROLL_BAR_HEIGHT-1);
		if (!mCaptionWidth && (!mHScroller || mHScroller->IsHidden())) {
			if (mVScroller)
				fullCaptionWidth -= B_V_SCROLL_BAR_WIDTH + 2;
			mCaption->ResizeTo( fullCaptionWidth, cpFrame.Height());
			mCaption->Invalidate();
		}
	}
	for(uint32 i=0; i<mActionViews.size(); ++i)
	{
		BView* actionView = mActionViews[i];
		if (!actionView)
			continue;
		BRect avFrame = actionView->Frame();
		actionView->MoveTo( r.Width()-B_V_SCROLL_BAR_WIDTH-1, avFrame.top);
	}
	return r;
}
