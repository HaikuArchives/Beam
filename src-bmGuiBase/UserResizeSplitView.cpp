//		$Id$
//Name:		UserResizeSplitView.cpp
//Authors:	Brian Tietz
//				Oliver Tappe
//Copyright 1999-2005


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <Application.h>

//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "UserResizeSplitView.h"
#include "BubbleHelper.h"
#include "Cursors.h"
#include "Colors.h"
#include "ColumnListView.h"

static const int32 Thickness = 4;


//******************************************************************************************************
//**** UserResizeSplitView
//******************************************************************************************************
UserResizeSplitView::UserResizeSplitView(MView* top_or_left, 
	MView* right_or_bottom,
	const char* name, float divider_left_or_top, orientation posture, 
	bool should_resize_left_or_top, bool should_resize_right_or_bottom,
	bool can_hide_left_or_top, bool can_hide_right_or_bottom,
	bool move_slider_on_frame_resize, uint32 resize_mask, uint32 flags)
: BView(BRect(0,0,0,0),name,resize_mask,flags)
{
	m_divider_left_or_top = divider_left_or_top;
	m_cached_width = 0;
	m_cached_height = 0;
	m_posture = posture;
	m_should_resize_left_or_top = should_resize_left_or_top;
	m_should_resize_right_or_bottom = should_resize_right_or_bottom;
	m_can_hide_left_or_top = can_hide_left_or_top;
	m_can_hide_right_or_bottom = can_hide_right_or_bottom;
	m_move_slider_on_frame_resize = move_slider_on_frame_resize;
	m_dragging = false;
	SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	m_left_or_top = top_or_left;
	if (top_or_left)
		ct_mpm = top_or_left->ct_mpm;
	m_right_or_bottom = right_or_bottom;
	m_left_or_top_BV = dynamic_cast<BView*>(top_or_left);
	m_right_or_bottom_BV = dynamic_cast<BView*>(right_or_bottom);
	AddChild( m_left_or_top_BV);
	AddChild( m_right_or_bottom_BV);
	if (m_posture==B_VERTICAL)
		TheBubbleHelper->SetCursor( this, c_v_resize_cursor());
	else
		TheBubbleHelper->SetCursor( this, c_h_resize_cursor());
}


float UserResizeSplitView::LeftOrTopMinSize() {
	return (m_posture==B_VERTICAL 
				? m_left_or_top->mpm.mini.x 
				: m_left_or_top->mpm.mini.y);
}

float UserResizeSplitView::LeftOrTopMaxSize() {
	return (m_posture==B_VERTICAL 
				? m_left_or_top->ct_mpm.maxi.x 
				: m_left_or_top->ct_mpm.maxi.y);
}

float UserResizeSplitView::RightOrBottomMinSize() {
	return (m_posture==B_VERTICAL 
				? m_right_or_bottom->mpm.mini.x 
				: m_right_or_bottom->mpm.mini.y);
}

float UserResizeSplitView::RightOrBottomMaxSize() {
	return (m_posture==B_VERTICAL 
				? m_right_or_bottom->ct_mpm.maxi.x 
				: m_right_or_bottom->ct_mpm.maxi.y);
}

void UserResizeSplitView::SetPreferredDividerLeftOrTop(float divider_left_or_top)
{
	m_divider_left_or_top = divider_left_or_top;
}

void UserResizeSplitView::SetDividerLeftOrTop(float divider_left_or_top, 
															 bool force)
{
	if (!force && m_divider_left_or_top == divider_left_or_top)
		return;

	BRect bounds = Bounds();

	if (divider_left_or_top > LeftOrTopMaxSize())
		divider_left_or_top = LeftOrTopMaxSize() + 1;
	if (divider_left_or_top < LeftOrTopMinSize()) {
		if (m_can_hide_left_or_top)
			divider_left_or_top = 0;
		else
			divider_left_or_top = LeftOrTopMinSize() + 1;
	}

	if(m_posture == B_HORIZONTAL)
	{
		if (divider_left_or_top > bounds.Height()-RightOrBottomMinSize()-Thickness) {
			if (m_can_hide_right_or_bottom)
				divider_left_or_top = bounds.Height()-Thickness;
			else
				divider_left_or_top 
					= bounds.Height()-RightOrBottomMinSize()-Thickness;
		}
	} else {
		if (divider_left_or_top > bounds.Width()-RightOrBottomMinSize()-Thickness) {
			if (m_can_hide_right_or_bottom)
				divider_left_or_top = bounds.Width()-Thickness;
			else
				divider_left_or_top 
					= bounds.Width()-RightOrBottomMinSize()-Thickness;
		}
	}

	BRect invalid;
	if(m_posture == B_HORIZONTAL)
	{
		if(m_should_resize_left_or_top && m_left_or_top)
			ResizeLeftOrTopChildTo( divider_left_or_top);
		if(m_right_or_bottom)
		{
			m_right_or_bottom_BV->MoveTo(bounds.left, 
												  divider_left_or_top+Thickness);
			if(m_should_resize_right_or_bottom)
				ResizeRightOrBottomChildTo(bounds.Height() - divider_left_or_top 
													- Thickness);
		}
		invalid.Set(bounds.left, 
						MIN(m_divider_left_or_top-1, divider_left_or_top-1),
						bounds.right,
						MAX(m_divider_left_or_top, divider_left_or_top)+Thickness);
	}
	else
	{
		if(m_should_resize_left_or_top && m_left_or_top)
			ResizeLeftOrTopChildTo( divider_left_or_top);
		if(m_right_or_bottom)
		{
			m_right_or_bottom_BV->MoveTo(divider_left_or_top+Thickness,bounds.top);
			if(m_should_resize_right_or_bottom)
				ResizeRightOrBottomChildTo( bounds.Width() - divider_left_or_top 
														- Thickness);
		}
		invalid.Set(MIN(m_divider_left_or_top, divider_left_or_top),
						bounds.top,
						MAX(m_divider_left_or_top, divider_left_or_top)+Thickness,
						bounds.bottom);
	}
	Invalidate(invalid);
	m_divider_left_or_top = divider_left_or_top;
}


void UserResizeSplitView::ResizeLeftOrTopChildTo(float width_or_height)
{
	BRect bounds = Bounds();
	if (m_left_or_top_BV && m_can_hide_left_or_top) {
		if (width_or_height < LeftOrTopMinSize()) {
			if (!m_left_or_top_BV->IsHidden())
				m_left_or_top_BV->Hide();
		} else {
			if (m_left_or_top_BV->IsHidden())
				m_left_or_top_BV->Show();
		}
	}
	if(m_posture == B_HORIZONTAL)
		m_left_or_top->layout(BRect(0, 0, bounds.right, width_or_height-1));
	else
		m_left_or_top->layout(BRect(0, 0, width_or_height-1, bounds.bottom));
}


void UserResizeSplitView::ResizeRightOrBottomChildTo(float width_or_height)
{
	BRect bounds = Bounds();
	if (m_right_or_bottom_BV && m_can_hide_right_or_bottom) {
		if (width_or_height < RightOrBottomMinSize()) {
			if (!m_right_or_bottom_BV->IsHidden())
				m_right_or_bottom_BV->Hide();
		} else {
			if (m_right_or_bottom_BV->IsHidden())
				m_right_or_bottom_BV->Show();
		}
	}
	m_right_or_bottom->layoutprefs();
	if(m_posture == B_HORIZONTAL)
		m_right_or_bottom->layout(BRect(0, bounds.bottom-width_or_height,
										  		  bounds.right, bounds.bottom));
	else
		m_right_or_bottom->layout(BRect(bounds.right-width_or_height, 0,
												  bounds.right, bounds.bottom));
}

void UserResizeSplitView::FrameResized(float new_width, float new_height)
{
	float dw = new_width - m_cached_width;
	float dh = new_height - m_cached_height;
	const float drawMargin = 3;
		// a small extra drawing margin that is needed in order to correctly
		// draw a grabber-"nipple" that has just gotten enough room to be shown

	BRect invalid;
	BRect bounds = Bounds();
	if (m_move_slider_on_frame_resize) {
		// completely invalidate and move grabber:
		if(m_posture == B_HORIZONTAL)
			invalid.Set(bounds.right-dw-drawMargin, 
							MIN(m_divider_left_or_top, m_divider_left_or_top+dh),
							bounds.right, 
							MAX(m_divider_left_or_top,
							    m_divider_left_or_top+dh)+Thickness);
		else
			invalid.Set(MIN(m_divider_left_or_top, m_divider_left_or_top+dw),
							bounds.bottom-dh-drawMargin,
							MAX(m_divider_left_or_top,
								 m_divider_left_or_top+dw)+Thickness,
							bounds.bottom);
		Invalidate(invalid);
		SetDividerLeftOrTop(m_divider_left_or_top 
										+ (m_posture == B_HORIZONTAL ? dh : dw));
	} else if ((m_posture == B_HORIZONTAL)	? dh != 0 : dw != 0) {
		SetDividerLeftOrTop(m_divider_left_or_top, true);
	} else {
		// invalidate only exposed subpart plus a margin for correct drawing:
		if(m_posture == B_HORIZONTAL)
			invalid.Set(bounds.right-dw-drawMargin, 
							MIN(m_divider_left_or_top, m_divider_left_or_top+dh),
							bounds.right, 
							MAX(m_divider_left_or_top,
							    m_divider_left_or_top+dh)+Thickness);
		else
			invalid.Set(MIN(m_divider_left_or_top, m_divider_left_or_top+dw),
							bounds.bottom-dh-drawMargin,
							MAX(m_divider_left_or_top,
								 m_divider_left_or_top+dh)+Thickness,
							bounds.bottom);
		Invalidate(invalid);
		SetDividerLeftOrTop(m_divider_left_or_top);
	}
	m_cached_width = new_width;
	m_cached_height = new_height;
}


void UserResizeSplitView::Draw(BRect updateRect)
{
	BRect b = Bounds();
	const float dist = 3;
	const float count 
		= floor((m_posture == B_HORIZONTAL ? b.Width() : b.Height()) / dist);
	rgb_color lightColor = ui_color(B_UI_SHINE_COLOR);
	rgb_color darkColor = BmWeakenColor( B_UI_SHADOW_COLOR, 3);
	BeginLineArray(count*2);
	float x = m_posture == B_HORIZONTAL ? 2 : m_divider_left_or_top + 1;
	float y = m_posture == B_HORIZONTAL ? m_divider_left_or_top + 1 : 2;
	for (int c=0; c<count; ++c) {
		AddLine(BPoint(x, y), BPoint(x ,y), darkColor);
		AddLine(BPoint(x+1, y+1), BPoint(x+1, y+1), lightColor);
		if (m_posture == B_HORIZONTAL)
			x += dist;
		else
			y += dist;
	}
	EndLineArray();
}


void UserResizeSplitView::MouseDown(BPoint where)
{
	BView::MouseDown( where);
	float mouse_position = 0;
	if(m_posture == B_HORIZONTAL)
		mouse_position = where.y;
	else
		mouse_position = where.x;
	if(mouse_position >= m_divider_left_or_top 
	&& mouse_position < m_divider_left_or_top+Thickness)
	{
		m_dragging = true;
		m_drag_mouse_offset = mouse_position-m_divider_left_or_top;
		SetMouseEventMask(B_POINTER_EVENTS,
								B_NO_POINTER_HISTORY | B_LOCK_WINDOW_FOCUS);
	}
	TheBubbleHelper->EnableHelp( false);
	if (m_posture==B_VERTICAL)
		be_app->SetCursor( c_v_resize_cursor());
	else
		be_app->SetCursor( c_h_resize_cursor());
}


void UserResizeSplitView::MouseUp(BPoint where)
{
	be_app->SetCursor( B_CURSOR_SYSTEM_DEFAULT);
	TheBubbleHelper->EnableHelp( true);
	m_dragging = false;
	float mouse_position = 0;
	if(m_posture == B_HORIZONTAL)
		mouse_position = where.y;
	else
		mouse_position = where.x;
	BView::MouseUp( where);
}


void UserResizeSplitView::MouseMoved(BPoint where, uint32 code, 
												 const BMessage* message)
{
	BView::MouseMoved( where, code, message);
	if(m_dragging)
	{
		if(m_posture == B_HORIZONTAL)
			SetDividerLeftOrTop(where.y - m_drag_mouse_offset);
		else
			SetDividerLeftOrTop(where.x - m_drag_mouse_offset);
	}
}

// additions for liblayout
minimax UserResizeSplitView::layoutprefs()
{
	minimax mmLeft;
	minimax mmRight;
	if (m_left_or_top)
		mmLeft = m_left_or_top->layoutprefs();
	if (m_right_or_bottom)
		mmRight = m_right_or_bottom->layoutprefs();
	if (m_posture == B_VERTICAL) {
		mpm.mini.x = MAX( mmLeft.mini.x+mmRight.mini.x+Thickness, ct_mpm.mini.x);
		mpm.mini.y = MAX( MAX(mmLeft.mini.y,mmRight.mini.y), ct_mpm.mini.y);
		mpm.maxi.x = MIN( mmLeft.maxi.x+mmRight.maxi.x+Thickness, ct_mpm.maxi.x);
		mpm.maxi.y = MIN( MAX(mmLeft.maxi.y,mmRight.maxi.y), ct_mpm.maxi.y);
	} else {
		mpm.mini.y = MAX( mmLeft.mini.y+mmRight.mini.y+Thickness, ct_mpm.mini.y);
		mpm.mini.x = MAX( MAX(mmLeft.mini.x,mmRight.mini.x), ct_mpm.mini.x);
		mpm.maxi.y = MIN( mmLeft.maxi.y+mmRight.maxi.y+Thickness, ct_mpm.maxi.y);
		mpm.maxi.x = MIN( MAX(mmLeft.maxi.x,mmRight.maxi.x), ct_mpm.maxi.x);
	}
	return mpm;
}

BRect UserResizeSplitView::layout(BRect frame)
{
	if (frame == Frame())
		return frame;
	MoveTo(frame.LeftTop());
	ResizeTo(frame.Width(),frame.Height());
	BRect rect = ConvertFromParent( frame);
	m_cached_width = rect.Width();
	m_cached_height = rect.Height();
	if (m_posture == B_HORIZONTAL) {
		if (m_left_or_top)
			m_left_or_top->layout( BRect(rect.left, rect.top,
												  rect.right, m_divider_left_or_top-1));
		if (m_right_or_bottom)
			m_right_or_bottom->layout( BRect(rect.left, 
														m_divider_left_or_top+Thickness,
														rect.right, rect.bottom));
	} else {
		if (m_left_or_top)
			m_left_or_top->layout( BRect(rect.left, rect.top,
												  m_divider_left_or_top-1, rect.bottom));
		if (m_right_or_bottom)
			m_right_or_bottom->layout( BRect(m_divider_left_or_top+Thickness,
														rect.top, rect.right, rect.bottom));
	}
	return frame;
}
