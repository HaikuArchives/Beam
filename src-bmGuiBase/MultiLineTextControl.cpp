//Name:		MultiLineTextControl.h
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
//**** System header files
//******************************************************************************************************
#include <string.h>
#include <OS.h>


#ifdef __POWERPC__
#define BM_BUILDING_SANTAPARTSFORBEAM 1
#endif

//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "MultiLineTextControl.h"
#include "Colors.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** MultiLineTextControl
//******************************************************************************************************
MultiLineTextControl::MultiLineTextControl(BRect frame, const char* name, const char* label,
	bool inline_label, const char* text, BMessage* message, uint32 resizing_mode, uint32 flags)
: BControl(frame,name,label,message,resizing_mode,B_WILL_DRAW|B_FRAME_EVENTS)
{
	m_enabled = true;
	m_modification_message = NULL;
	m_inline_label = inline_label;
	if(m_inline_label)
		m_divider = floor((frame.right-frame.left)/2);
	else
		m_divider = 0;
	ReevaluateLabelRect();

	if(m_inline_label)
	{
		m_entry_text_rect = Bounds();
		m_entry_text_rect.left = m_divider;
	}
	else
	{
		m_entry_text_rect = Bounds();
		m_entry_text_rect.top = m_label_text_rect.bottom+3;
	}
	SetViewColor( BeBackgroundGrey);
	m_text_view = new MultiLineTextControlTextView(m_entry_text_rect.InsetByCopy(2,2),flags, resizing_mode);
	if(text)
		m_text_view->SetText(text);
	m_text_margin = 3;
	ResetTextRect();
	AddChild(m_text_view);
	m_focus_color = ui_color(B_KEYBOARD_NAVIGATION_COLOR);
}


MultiLineTextControl::~MultiLineTextControl()
{
	if(m_modification_message)
		delete m_modification_message;
}


void MultiLineTextControl::AttachedToWindow()
{
	BControl::AttachedToWindow();
	rgb_color background_color = Parent()->ViewColor();
	m_dark_1_color = tint_color(background_color,B_DARKEN_1_TINT);
	m_dark_2_color = tint_color(background_color,B_DARKEN_4_TINT);
	SetViewColor(background_color);
	ReevaluateLabelRect();
}


void MultiLineTextControl::ReevaluateLabelRect()
{
	BFont font;
	GetFont(&font);
	struct font_height curr_font_height;
	font.GetHeight(&curr_font_height);
	m_label_font_ascent = ceil(curr_font_height.ascent);
	float height = m_label_font_ascent + ceil(curr_font_height.descent);
	float width = font.StringWidth(Label());
	if(m_inline_label)
	{
		m_label_text_rect.top = 4;
		m_label_text_rect.bottom = m_label_text_rect.top + height;
		m_label_text_rect.left = 3;
		m_label_text_rect.right = width;
	}
	else
	{
		m_label_text_rect.top = 0;
		m_label_text_rect.bottom = height;
		m_label_text_rect.left = 6;
		m_label_text_rect.right = m_label_text_rect.left + width;	
	}
}

void MultiLineTextControl::SetEnabled( bool enabled) {
	m_enabled = enabled;
	BControl::SetEnabled( enabled);
	m_text_view->SetViewColor( enabled ? White : BeInactiveControlGrey);
	m_text_view->MakeEditable( enabled);
	MakeFocus( false);
	Invalidate();
	m_text_view->Invalidate();
}


void MultiLineTextControl::Draw(BRect update_rect)
{
	if (m_entry_text_rect.Height()==0 || m_entry_text_rect.Width()==0)
		return;
	if (m_enabled) {
		if(update_rect.Intersects(m_label_text_rect)) {
			SetHighColor( Black);
			DrawString(Label(),BPoint(m_label_text_rect.left,m_label_text_rect.top+m_label_font_ascent));
		}
		if(update_rect.Intersects(m_entry_text_rect))
		{
			rgb_color original_color = HighColor();
			SetHighColor(m_dark_1_color);
			StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.top));
			StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top+1),
				BPoint(m_entry_text_rect.left,m_entry_text_rect.bottom));
			SetHighColor(White);
			StrokeLine(BPoint(m_entry_text_rect.right,m_entry_text_rect.top+1),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom-1));
			StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom));
			if(m_text_view->IsFocus())
			{
				SetHighColor(m_focus_color);
				StrokeRect(m_entry_text_rect.InsetByCopy(1,1));
			}
			else
			{
				SetHighColor(m_dark_2_color);
				StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+1),
					BPoint(m_entry_text_rect.right-1,m_entry_text_rect.top+1));
				StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+2),
					BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom-1));
			}
			SetHighColor(original_color);
		}
	} else {
		if(update_rect.Intersects(m_label_text_rect)) {
			SetHighColor( BeInactiveGrey);
			DrawString(Label(),BPoint(m_label_text_rect.left,m_label_text_rect.top+m_label_font_ascent));
		}
		if(update_rect.Intersects(m_entry_text_rect))
		{
			rgb_color original_color = HighColor();
			SetHighColor(BeBackgroundGrey);
			StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.top));
			StrokeLine(BPoint(m_entry_text_rect.left,m_entry_text_rect.top+1),
				BPoint(m_entry_text_rect.left,m_entry_text_rect.bottom));
			SetHighColor(BeInactiveControlGrey);
			StrokeLine(BPoint(m_entry_text_rect.right,m_entry_text_rect.top+1),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom-1));
			StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom),
				BPoint(m_entry_text_rect.right,m_entry_text_rect.bottom));

			SetHighColor(BeShadow);
			StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+1),
				BPoint(m_entry_text_rect.right-1,m_entry_text_rect.top+1));
			StrokeLine(BPoint(m_entry_text_rect.left+1,m_entry_text_rect.top+2),
				BPoint(m_entry_text_rect.left+1,m_entry_text_rect.bottom-1));
			SetHighColor(original_color);
		}
	}
}

void MultiLineTextControl::FrameResized(float, float)
{
	BRect new_entry_text_rect = m_text_view->Frame().InsetByCopy(-2,-2);

	float right_min = m_entry_text_rect.right;
	if(right_min > new_entry_text_rect.right)
		right_min = new_entry_text_rect.right;
	float right_max = m_entry_text_rect.right;
	if(right_max < new_entry_text_rect.right)
		right_max = new_entry_text_rect.right;
	float bottom_min = m_entry_text_rect.bottom;
	if(bottom_min > new_entry_text_rect.bottom)
		bottom_min = new_entry_text_rect.bottom;
	float bottom_max = m_entry_text_rect.bottom;
	if(bottom_max < new_entry_text_rect.bottom)
		bottom_max = new_entry_text_rect.bottom;

	if(new_entry_text_rect.right != m_entry_text_rect.right)
		Invalidate(BRect(right_min-1,new_entry_text_rect.top,right_max,bottom_max));
	if(new_entry_text_rect.bottom != m_entry_text_rect.bottom)
		Invalidate(BRect(new_entry_text_rect.left,bottom_min-1,right_max,bottom_max));

	m_entry_text_rect = new_entry_text_rect;
	ResetTextRect();
}


void MultiLineTextControl::MakeFocus(bool flag)
{
	m_text_view->MakeFocus(flag);
}


void MultiLineTextControl::SetLabel(const char *text)
{
	BControl::SetLabel(text);
	BRect old_rect = m_label_text_rect;
	ReevaluateLabelRect();
	Invalidate(old_rect|m_label_text_rect);
}


void MultiLineTextControl::SetText(const char *text)
{
	m_text_view->SetText(text);
}


const char* MultiLineTextControl::Text(void) const
{
	return m_text_view->Text();
}


void MultiLineTextControl::SetTabAllowed(bool allowed)
{
	m_text_view->SetTabAllowed(allowed);
}


bool MultiLineTextControl::TabAllowed() const
{
	return m_text_view->TabAllowed();
}


BTextView* MultiLineTextControl::TextView(void) const
{
	return m_text_view;
}


void MultiLineTextControl::SetDivider(float divider)
{
	if(m_inline_label)
	{
		BRect area = Bounds();
		area.left = divider;
		m_divider = divider;
		m_entry_text_rect = area;
		area.InsetBy(2,2);
		m_text_view->MoveTo(area.left,area.top);
		m_text_view->ResizeTo(area.Width(),area.Height());
		ReevaluateLabelRect();
		Invalidate();
	}
}


void MultiLineTextControl::SetModificationMessage(BMessage* message)
{
	if(m_modification_message)
		delete m_modification_message;
	m_modification_message = message;
}


BMessage* MultiLineTextControl::ModificationMessage() const
{
	return m_modification_message;
}


void MultiLineTextControl::ResizeToWithChildren(float width, float height)
{
	ResizeTo(width,height);
	BRect area = Bounds();
	if(m_inline_label)
		area.left = m_divider;
	else
		area.top = m_label_text_rect.bottom+3;
	m_entry_text_rect = area;
	area.InsetBy(2,2);
	m_text_view->MoveTo(area.left,area.top);
	m_text_view->ResizeTo(area.Width(),area.Height());
	m_text_view->FrameResized(area.Width(),area.Height());
	Invalidate();
}


void MultiLineTextControl::ResetTextRect()
{
	BRect textRect = m_text_view->Bounds();
	textRect.left = m_text_margin;
	textRect.top = m_text_margin;
	textRect.right -= m_text_margin;
	textRect.bottom -= m_text_margin;
	m_text_view->SetTextRect(textRect);
}


void MultiLineTextControl::Modified()
{
	if(m_modification_message)
		Invoke(m_modification_message);
}


//******************************************************************************************************
//**** MultiLineTextControlTextView
//******************************************************************************************************
MultiLineTextControlTextView::MultiLineTextControlTextView(BRect frame, uint32 flags,uint32 res_mode)
: BTextView(frame,NULL,frame.OffsetToCopy(0,0).InsetByCopy(1,1),res_mode,flags)
{
	m_tab_allowed = true;
	m_modified = false;
	SetWordWrap(true);
}


void MultiLineTextControlTextView::MakeFocus(bool flag)
{
	Parent()->Invalidate(Frame().InsetByCopy(-2,-2));
	BTextView::MakeFocus(flag);
	if(flag == false)
	{
		if(m_modified)
			((MultiLineTextControl*)Parent())->Invoke();		//Text has changed.  Invoke control.
		m_modified = false;
	}
}


void MultiLineTextControlTextView::KeyDown(const char* bytes, int32 num_bytes)
{
	if(num_bytes == 1 && (bytes[0] == B_TAB && (!m_tab_allowed)))
	{
		//Skip the text view, which would take the tab.  The BView will interpret it to change focus.
		BView::KeyDown(bytes,num_bytes);
		return;
	}
	BTextView::KeyDown(bytes,num_bytes);
}


void MultiLineTextControlTextView::InsertText(const char *text, int32 length, int32 offset,
	const text_run_array* runs)
{
	BTextView::InsertText(text,length,offset,runs);
	//Text has changed.  Invoke control.
	BView* parent = Parent();
	if(parent)
		((MultiLineTextControl*)parent)->Modified();
	m_modified = true;
}


void MultiLineTextControlTextView::DeleteText(int32 from_offset, int32 to_offset)
{
	BTextView::DeleteText(from_offset,to_offset);
	//Text has changed.  Invoke control.
	BView* parent = Parent();
	if(parent)
		((MultiLineTextControl*)parent)->Modified();
	m_modified = true;
}

