//Name:		DeepBevelView.cpp
//Author:	Brian Tietz
//Copyright 1999
//Conventions:
//	Global constants (declared with const) and #defines - begin with "c_" followed by lowercase
//		words separated by underscores.
//		(E.G., #define c_my_constant 5).
//		(E.G., const int c_my_constant = 5;).
//	Global variables - begin with "g_" followed by lowercase words separated by underscores.
//		(E.G., int g_my_global;).
//	New data types (classes, structs, typedefs, etc.) - begin with an uppercase letter followed by
//		lowercase words separated by uppercase letters.  Enumerated constants contain a prefix
//		associating them with a particular enumerated set.
//		(E.G., typedef int MyTypedef;).
//		(E.G., enum MyEnumConst {c_mec_one, c_mec_two};)
//	Private member variables - begin with "m_" followed by lowercase words separated by underscores.
//		(E.G., int m_my_member;).
//	Public or friend-accessible member variables - all lowercase words separated by underscores.
//		(E.G., int public_member;).
//	Argument and local variables - begin with a lowercase letter followed by
//		lowercase words separated by underscores.  If the name is already taken by a public member
//		variable, prefix with a_ or l_
//		(E.G., int my_local; int a_my_arg, int l_my_local).
//	Functions (member or global) - begin with an uppercase letter followed by lowercase words
//		separated by uppercase letters.
//		(E.G., void MyFunction(void);).


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
	m_dark_1_color = tint_color(m_background_color,B_DARKEN_1_TINT);
	m_dark_2_color = tint_color(m_background_color,B_DARKEN_4_TINT);
	m_cached_bounds = Bounds();
}


void DeepBevelView::Draw(BRect update_rect)
{
	SetHighColor(m_dark_1_color);
	StrokeLine(BPoint(m_cached_bounds.left,m_cached_bounds.top),BPoint(m_cached_bounds.right,
		m_cached_bounds.top));
	StrokeLine(BPoint(m_cached_bounds.left,m_cached_bounds.top+1),BPoint(m_cached_bounds.left,
		m_cached_bounds.bottom));
	SetHighColor(White);
	StrokeLine(BPoint(m_cached_bounds.right,m_cached_bounds.top+1),BPoint(m_cached_bounds.right,
		m_cached_bounds.bottom-1));
	StrokeLine(BPoint(m_cached_bounds.left+1,m_cached_bounds.bottom),BPoint(m_cached_bounds.right,
		m_cached_bounds.bottom));
	SetHighColor(m_dark_2_color);
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


void DeepBevelView::FrameResized(float width, float height)
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

