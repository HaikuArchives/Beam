//Name:		ListSelectionAlert.cpp
//Author:	Oliver Tappe, based on code by Brian Tietz
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

/*
	[Oliver Tappe]:
		Be very careful, this code is just a HACK!
		It works for what Beam does with it, but it is not up for general use.
*/


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <string.h>
#include <Button.h>
#include <Screen.h>


//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "ListSelectionAlert.h"
#include "Colors.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** Constants
//******************************************************************************************************
const float c_text_border_width = 2;
const float c_text_margin = 1;
const float c_item_spacing = 9;
const float c_non_inline_label_spacing = 5;
const float c_inline_label_spacing = 7;
const float c_usual_button_width = 74;


//******************************************************************************************************
//**** BMessage what constants for internal use
//******************************************************************************************************
const uint32 c_button_pressed = 0;


//******************************************************************************************************
//**** ListSelectionAlert
//******************************************************************************************************


ListSelectionAlert::ListSelectionAlert(const char* title, const char* info_text, BList& items,
	const char* initial_selection,
	const char* button_0_label, const char* button_1_label, float min_list_box_width,
	int min_list_box_height, button_width width_style, const BRect* frame,
	window_look look, window_feel feel, uint32 flags)
: BWindow((frame?*frame:BRect(50,50,60,60)),title,look,feel,flags)
{
	//Get unadjusted sizes of the two text boxes
	BRect info_box;
	BRect sel_list_rect;
	const char* strings[2] = {info_text?info_text:"",initial_selection?initial_selection:""};
	escapement_delta zero_escapements[2] = {{0,0},{0,0}};
	BRect result_rects[2];
	be_plain_font->GetBoundingBoxesForStrings(strings,2,B_SCREEN_METRIC,zero_escapements,result_rects);
	struct font_height plain_font_height;
	be_plain_font->GetHeight(&plain_font_height);
	info_box = result_rects[0];
	info_box.bottom = info_box.top + (ceil(plain_font_height.ascent) + ceil(plain_font_height.descent))*4;
	sel_list_rect = result_rects[1];
	sel_list_rect.bottom = sel_list_rect.top + min_list_box_height;
	sel_list_rect.InsetBy(0-(c_text_margin+c_text_border_width),
		0-(c_text_margin+c_text_border_width));
	if(sel_list_rect.Width() < min_list_box_width)
		sel_list_rect.right = sel_list_rect.left+min_list_box_width;

	//Position and create label
	m_label_view = NULL;
	if(info_text)
	{
		info_box.OffsetTo(c_item_spacing,c_item_spacing);
		info_box.OffsetBy(c_non_inline_label_spacing,-2);
		info_box.bottom += 2;		//Compensate for offset used by BTextView
		info_box.right += 1;
		m_label_view = new BTextView(info_box,NULL,info_box.OffsetToCopy(0,0),
			B_FOLLOW_LEFT|B_FOLLOW_TOP,B_WILL_DRAW);
		m_label_view->SetText(info_text);
		m_label_view->MakeEditable(false);
		m_label_view->MakeSelectable(false);
		m_label_view->SetWordWrap(true);
		m_label_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}
	else
	{
		info_box.Set(0,0,0,0);
	}

	//Create buttons
	m_buttons[0] = NULL;
	m_buttons[1] = NULL;
	BMessage* message;
	float button_0_width, button_1_width, buttons_height;
	float max_buttons_width = 0;
	if(button_0_label != NULL)
	{
		message = new BMessage(c_button_pressed);
		message->AddInt32("which",0);
		m_buttons[0] = new BButton(BRect(0,0,0,0),button_0_label,button_0_label,message,B_FOLLOW_LEFT|
			B_FOLLOW_BOTTOM);
		m_buttons[0]->GetPreferredSize(&button_0_width,&buttons_height);
		max_buttons_width = button_0_width;
	}
	if(button_1_label != NULL)
	{
		message = new BMessage(c_button_pressed);
		message->AddInt32("which",1);
		m_buttons[1] = new BButton(BRect(0,0,0,0),button_1_label,button_1_label,message,B_FOLLOW_RIGHT|
			B_FOLLOW_BOTTOM);
		m_buttons[1]->GetPreferredSize(&button_1_width,&buttons_height);
		if(max_buttons_width < button_1_width)
			max_buttons_width = button_1_width;
	}
	
	//Position and create list-box
	sel_list_rect.OffsetTo(c_item_spacing,info_box.bottom+c_non_inline_label_spacing-4);
		//-5 is to compensate for extra pixels that BeOS adds to the font height
	if(frame != NULL)
	{
		if(sel_list_rect.left + min_list_box_width < frame->Width()-c_item_spacing)
			sel_list_rect.right = frame->Width()+c_item_spacing;
		else
			sel_list_rect.right = sel_list_rect.left + min_list_box_width;
	}

	sel_list_rect.right += B_V_SCROLL_BAR_WIDTH;
	sel_list_rect.bottom += B_H_SCROLL_BAR_HEIGHT;

	m_list_view = new ColumnListView( minimax(0,0,1E5,1E5), sel_list_rect, NULL, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
												 B_SINGLE_SELECTION_LIST, true, true);

	CLVContainerView* container 
		= m_list_view->Initialize( sel_list_rect, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
											B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);
	m_list_view->AddColumn( 
		new CLVColumn( title, 300.0, CLV_TELL_ITEMS_WIDTH, 300.0));

	CLVEasyItem* item;
	int32 count=items.CountItems();
	for( int32 i=0; i<count; ++i) {
		item = new CLVEasyItem( 0, false, false, 18.0);
		item->SetColumnContent( 0, (char*)items.ItemAt(i));
		m_list_view->AddItem( item);
		if (initial_selection && strcmp(initial_selection,(char*)items.ItemAt(i))==0)
			m_list_view->Select( i);
	}

	//Position the buttons
	BButton* right_button = NULL;
	if(m_buttons[1] != NULL)
		right_button = m_buttons[1];
	else if(m_buttons[0] != NULL)
		right_button = m_buttons[0];
	if(right_button != NULL)
	{
		if(width_style == B_WIDTH_AS_USUAL)
			right_button->ResizeTo(c_usual_button_width,buttons_height);
		else if(width_style == B_WIDTH_FROM_LABEL)
			right_button->ResizeTo(right_button==m_buttons[1]?button_1_width:button_0_width,
				buttons_height);
		else //(width_style == B_WIDTH_FROM_WIDEST)
			right_button->ResizeTo(max_buttons_width,buttons_height);
		right_button->MoveTo(sel_list_rect.right+B_V_SCROLL_BAR_WIDTH-right_button->Frame().Width()+1,
			sel_list_rect.bottom+B_H_SCROLL_BAR_HEIGHT+c_item_spacing);
		right_button->MakeDefault(true);
	}
	if(m_buttons[0] != NULL && m_buttons[1] != NULL)
	{
		float button_left;
		if(width_style == B_WIDTH_AS_USUAL)
			m_buttons[0]->ResizeTo(c_usual_button_width,buttons_height);
		else if(width_style == B_WIDTH_FROM_LABEL)
			m_buttons[0]->ResizeTo(button_0_width,buttons_height);
		else //if(width_style == B_WIDTH_FROM_WIDEST)
			m_buttons[0]->ResizeTo(max_buttons_width,buttons_height);
		button_left = right_button->Frame().left-m_buttons[0]->Frame().Width()-10;
		m_buttons[0]->MoveTo(button_left,sel_list_rect.bottom+B_H_SCROLL_BAR_HEIGHT+c_item_spacing);
	}

	//Resize the window
	float height;
	if(m_buttons[0])
		height = m_buttons[0]->Frame().bottom+c_item_spacing-1;
	else if(m_buttons[1])
		height = m_buttons[1]->Frame().bottom+c_item_spacing-1;
	else
		height = sel_list_rect.bottom+c_item_spacing-1;
	ResizeTo(sel_list_rect.right+c_item_spacing+B_V_SCROLL_BAR_WIDTH,height+B_H_SCROLL_BAR_HEIGHT);

	BRect bounds = Bounds();
	if(frame == NULL)
	{
		BRect screen_frame = BScreen().Frame();
		MoveTo(ceil((screen_frame.Width()-bounds.Width())/2),
			ceil((screen_frame.Height()-bounds.Height()-19)/2));
	}

	//Create the background view and add the children
	BView* filler = new ListSelectionAlertBackgroundView(bounds,sel_list_rect);
	if(m_label_view)
		filler->AddChild(m_label_view);
	filler->AddChild(container);
	if(m_buttons[0])
		filler->AddChild(m_buttons[0]);
	if(m_buttons[1])
		filler->AddChild(m_buttons[1]);
	AddChild(filler);

	//Complete the setup
	m_invoker = NULL;
	m_done_mutex = B_ERROR;
	m_selection_buffer = NULL;
	m_button_pressed = NULL;
	float min_width = c_item_spacing;
	if(m_buttons[0])
		min_width += (m_buttons[0]->Frame().Width() + c_item_spacing);
	if(m_buttons[1])
		min_width += (m_buttons[0]->Frame().Width() + c_item_spacing);
	if(min_width < 120)
		min_width = 120;
	float min_height = sel_list_rect.top;
	min_height += (ceil(plain_font_height.ascent)+ceil(plain_font_height.descent));
	min_height += ((c_text_margin+c_text_border_width)*2 + c_item_spacing);
	if(m_buttons[0] || m_buttons[1])
		min_height += (buttons_height + c_item_spacing);
	min_width -= 2;		//Need this for some reason
	min_height -= 2;
	SetSizeLimits(min_width,100000,min_height,100000);
	m_shortcut[0] = 0;
	m_shortcut[1] = 0;
}


ListSelectionAlert::~ListSelectionAlert()
{
	if(m_invoker)
		delete m_invoker;
}


int32 ListSelectionAlert::Go(char* selection_buffer, int32 buffer_size)
{
	int32 button_pressed = -1;
	m_selection_buffer = selection_buffer;
	m_button_pressed = &button_pressed;
	m_buffer_size = buffer_size;
	sem_id done_mutex = create_sem(0,"list_selection_alert_done");
	m_done_mutex = done_mutex;
	if(done_mutex < B_NO_ERROR)
	{
		Quit();
		return -1;
	}
	m_list_view->MakeFocus(true);
	Show();
	acquire_sem(done_mutex);
	delete_sem(done_mutex);
	return button_pressed;
}


status_t ListSelectionAlert::Go(BInvoker *invoker)
{
	m_invoker = invoker;
	m_list_view->MakeFocus(true);
	Show();
	return B_NO_ERROR;
}


void ListSelectionAlert::SetShortcut(int32 index, char shortcut)
{
	m_shortcut[index] = shortcut;
}


char ListSelectionAlert::Shortcut(int32 index) const
{
	return m_shortcut[index];
}


BTextView* ListSelectionAlert::LabelView(void) const
{
	return m_label_view;
}


ColumnListView* ListSelectionAlert::ListView(void) const
{
	return m_list_view;
}


void ListSelectionAlert::MessageReceived(BMessage* message)
{
	if(message->what == c_button_pressed)
	{
		int32 which;
		if(message->FindInt32("which",&which) == B_NO_ERROR)
		{
			int32 selection = m_list_view->CurrentSelection(0);
			CLVEasyItem* item = NULL;
			if (selection > -1) 
				item = (CLVEasyItem*)m_list_view->ItemAt( selection);
			const char* selected_text = "";
			if (item)
				selected_text = item->GetColumnContentText( 0);
			if(m_done_mutex < B_NO_ERROR)
			{
				//Asynchronous version: add the necessary fields
				BMessage* message = m_invoker->Message();
				if(message && (message->AddInt32("which",which) == B_NO_ERROR ||
					message->ReplaceInt32("which",which) == B_NO_ERROR) &&
					(message->AddString("selection_text",selected_text) == B_NO_ERROR ||
					message->ReplaceString("selection_text",selected_text) == B_NO_ERROR))
					m_invoker->Invoke();
			}
			else
			{
				//Synchronous version: set the result button and text buffer, then release the thread
				//that created me
				*m_button_pressed = which;
				if(m_selection_buffer)
					Strtcpy(m_selection_buffer,selected_text,m_buffer_size);
				release_sem(m_done_mutex);
				m_done_mutex = B_ERROR;
			}
			PostMessage(B_QUIT_REQUESTED);
		}
	}
}

void ListSelectionAlert::Quit()
{
	//Release the mutex if I'm synchronous and I haven't released it yet
	if(m_done_mutex >= B_NO_ERROR)
		release_sem(m_done_mutex);
	BWindow::Quit();
}


//******************************************************************************************************
//**** ListSelectionAlertBackgroundView
//******************************************************************************************************
ListSelectionAlertBackgroundView::ListSelectionAlertBackgroundView(BRect frame, BRect entry_text_rect)
: BView(frame,NULL,B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_FRAME_EVENTS)
{
	m_list_rect = entry_text_rect;
	m_cached_bounds = Bounds();
	rgb_color background_color = ui_color(B_PANEL_BACKGROUND_COLOR);
	m_dark_1_color = tint_color(background_color,B_DARKEN_1_TINT);
	m_dark_2_color = tint_color(background_color,B_DARKEN_4_TINT);
	SetViewColor(background_color);
	SetDrawingMode(B_OP_COPY);
}


void ListSelectionAlertBackgroundView::Draw(BRect update_rect)
{
	if(update_rect.Intersects(m_list_rect))
	{
		SetHighColor(m_dark_1_color);
		StrokeLine(BPoint(m_list_rect.left,m_list_rect.top),
			BPoint(m_list_rect.right,m_list_rect.top));
		StrokeLine(BPoint(m_list_rect.left,m_list_rect.top+1),
			BPoint(m_list_rect.left,m_list_rect.bottom));
		SetHighColor(White);
		StrokeLine(BPoint(m_list_rect.right,m_list_rect.top+1),
			BPoint(m_list_rect.right,m_list_rect.bottom-1));
		StrokeLine(BPoint(m_list_rect.left+1,m_list_rect.bottom),
			BPoint(m_list_rect.right,m_list_rect.bottom));
		SetHighColor(m_dark_2_color);
		StrokeLine(BPoint(m_list_rect.left+1,m_list_rect.top+1),
			BPoint(m_list_rect.right-1,m_list_rect.top+1));
		StrokeLine(BPoint(m_list_rect.left+1,m_list_rect.top+2),
			BPoint(m_list_rect.left+1,m_list_rect.bottom-1));
	}
}


void ListSelectionAlertBackgroundView::FrameResized(float width, float heigh)
{
	BRect new_bounds = Bounds();
	float width_delta = new_bounds.right - m_cached_bounds.right;
	float height_delta = new_bounds.bottom - m_cached_bounds.bottom;
	BRect new_entry_text_rect = m_list_rect;
	new_entry_text_rect.right += width_delta;
	new_entry_text_rect.bottom += height_delta;

	float right_min = m_list_rect.right;
	if(right_min > new_entry_text_rect.right)
		right_min = new_entry_text_rect.right;
	float right_max = m_list_rect.right;
	if(right_max < new_entry_text_rect.right)
		right_max = new_entry_text_rect.right;
	float bottom_min = m_list_rect.bottom;
	if(bottom_min > new_entry_text_rect.bottom)
		bottom_min = new_entry_text_rect.bottom;
	float bottom_max = m_list_rect.bottom;
	if(bottom_max < new_entry_text_rect.bottom)
		bottom_max = new_entry_text_rect.bottom;

	if(new_entry_text_rect.right != m_list_rect.right)
		Invalidate(BRect(right_min-1,new_entry_text_rect.top,right_max,bottom_max));
	if(new_entry_text_rect.bottom != m_list_rect.bottom)
		Invalidate(BRect(new_entry_text_rect.left,bottom_min-1,right_max,bottom_max));

	m_list_rect = new_entry_text_rect;
	m_cached_bounds = new_bounds;
}



