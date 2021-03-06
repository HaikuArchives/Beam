//Name:		DeepBevelView.cpp
//Author:	Brian Tietz
//Copyright 1999


//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "DeepBevelView.h"
#include "Colors.h"


//******************************************************************************************************
//**** DeepBevelView
//******************************************************************************************************
DeepBevelView::DeepBevelView(BRect frame, const char* name, uint32 resize_mask, uint32 flags)
: BView(frame,name,resize_mask,flags)
{
	m_background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	m_dark_1_color = BmWeakenColor( B_PANEL_BACKGROUND_COLOR, 2);
	m_cached_bounds = Bounds();
}


void DeepBevelView::Draw(BRect)
{
	SetHighColor(m_dark_1_color);
	StrokeLine(BPoint(m_cached_bounds.left,m_cached_bounds.top),BPoint(m_cached_bounds.right,
		m_cached_bounds.top));
	StrokeLine(BPoint(m_cached_bounds.left,m_cached_bounds.top+1),BPoint(m_cached_bounds.left,
		m_cached_bounds.bottom));
	SetHighColor( ui_color( B_SHINE_COLOR));
	StrokeLine(BPoint(m_cached_bounds.right,m_cached_bounds.top+1),BPoint(m_cached_bounds.right,
		m_cached_bounds.bottom-1));
	StrokeLine(BPoint(m_cached_bounds.left+1,m_cached_bounds.bottom),BPoint(m_cached_bounds.right,
		m_cached_bounds.bottom));
	SetHighColor( BmWeakenColor(B_SHADOW_COLOR, BeShadowMod));
	StrokeLine(BPoint(m_cached_bounds.left+1,m_cached_bounds.top+1),BPoint(m_cached_bounds.right-1,
		m_cached_bounds.top+1));
	StrokeLine(BPoint(m_cached_bounds.left+1,m_cached_bounds.top+2),BPoint(m_cached_bounds.left+1,
		m_cached_bounds.bottom-1));
	SetHighColor(m_background_color);
	StrokeLine(BPoint(m_cached_bounds.left+2,m_cached_bounds.bottom-1),BPoint(m_cached_bounds.right-1,
		m_cached_bounds.bottom-1));
	StrokeLine(BPoint(m_cached_bounds.right-1,m_cached_bounds.top+2),BPoint(m_cached_bounds.right-1,
		m_cached_bounds.bottom-2));
}


void DeepBevelView::FrameResized(float, float)
{
	BRect new_bounds = Bounds();
	float min_x = new_bounds.right;
	if(min_x > m_cached_bounds.right)
		min_x = m_cached_bounds.right;
	float max_x = new_bounds.right;
	if(max_x < m_cached_bounds.right)
		max_x = m_cached_bounds.right;
	float min_y = new_bounds.bottom;
	if(min_y > m_cached_bounds.bottom)
		min_y = m_cached_bounds.bottom;
	float max_y = new_bounds.bottom;
	if(max_y < m_cached_bounds.bottom)
		max_y = m_cached_bounds.bottom;
	if(min_x != max_x)
		Invalidate(BRect(min_x-1,new_bounds.top,max_x,max_y));
	if(min_y != max_y)
		Invalidate(BRect(new_bounds.left,min_y-1,max_x,max_y));
}

