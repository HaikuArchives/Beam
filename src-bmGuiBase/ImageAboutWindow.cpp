//Name:		ImageAboutWindow.h
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
#include <TranslationUtils.h>
#include <Application.h>
#include <Resources.h>
#include <Screen.h>
#include <Bitmap.h>
#include <StringView.h>
#include <TextView.h>

//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "ImageAboutWindow.h"
#include "NewStrings.h"
#include "Colors.h"


//******************************************************************************************************
//**** ImageAboutWindow
//******************************************************************************************************
ImageAboutWindow::ImageAboutWindow(const char* window_title, const char* app_title,
	const BBitmap* bmap, float icon_sidebar_offset, const char* body_text,
	const char* email, const char* web, const char* credits)
: BWindow(BRect(-1,-1,-1,-1),"",B_TITLED_WINDOW,
			 B_NOT_RESIZABLE|B_NOT_ZOOMABLE|B_PULSE_NEEDED)
, m_credits_pos( 0)
{
	SetTitle(window_title);

	//Extract title bitmap
	m_bitmap = (BBitmap*)bmap;
	m_title = Strdup_new(app_title);

	//Extract version resource
	m_version = NULL;
	BResources* app_version_resource = BApplication::AppResources();
	if(app_version_resource)
	{
		size_t resource_size;
		const char* app_version_data = (const char*)app_version_resource->LoadResource('APPV',
			"BEOS:APP_VERSION",&resource_size);
		if(app_version_data && resource_size > 20)
			m_version = Strdup_new(&app_version_data[20]);
	}

	//Extract individual lines of text from body_text as null-terminated strings
	m_num_lines = 0;
	m_lines = NULL;
	m_text_rects = NULL;
	int length = 0;
	int line;
	int pos;
	if(body_text)
	{
		length = strlen(body_text);
		for(pos=0; pos<length; pos++)
			if(body_text[pos] == '\n')
				m_num_lines++;
		m_num_lines++;
	}
	if(m_num_lines > 0)
	{
		m_lines = new char*[m_num_lines];	
		m_text_rects = new BRect[m_num_lines];
	}
	for(line=0; line<m_num_lines; line++)
	{
		if(body_text[0] == '\n')
		{
			m_lines[line] = NULL;
			body_text++;
			length--;
		}
		else
		{
			int line_length = 0;
			for(pos=0; pos<length; pos++)
				if(body_text[pos] != '\n' && body_text[pos] != 0)
					line_length++;
				else
					break;
			m_lines[line] = new char[line_length+1];
			Strtcpy(m_lines[line],body_text,line_length+1);
			body_text = &body_text[line_length+1];
			length -= line_length;
		}
	}

	// extract email and web, if defined:
	m_email = email ? Strdup_new( email) : NULL;
	m_web = web ? Strdup_new( web) : NULL;

	//Figure out size, etc
	float logo_right = 29;
	m_logo_rect.Set(-1,-1,-1,-1);
	m_left_of_logo.Set(0,0,29,-1);
	if(m_bitmap)
	{
		m_logo_rect = m_bitmap->Bounds().OffsetToCopy(15,6);
		logo_right = m_logo_rect.right;
		float line_edge = m_logo_rect.left+icon_sidebar_offset;
		if(line_edge < 29)
		{
			m_logo_rect.OffsetBy(29-line_edge,0);
			line_edge = 29;
		}
	
		m_left_of_logo.Set(0,0,line_edge,-1);
	}

	//Get font size information
	struct font_height font_ht;
	be_bold_font->GetHeight(&font_ht);
	m_bold_font_ascent = ceil(font_ht.ascent);
	float bold_height = m_bold_font_ascent + ceil(font_ht.descent);
	be_plain_font->GetHeight(&font_ht);
	m_plain_font_ascent = ceil(font_ht.ascent);
	float plain_height = m_plain_font_ascent + ceil(font_ht.descent);
	float plain_spacing = m_plain_font_ascent + ceil(font_ht.descent) + ceil(font_ht.leading);

	//Get bounding boxes for the various strings
	float title_width = be_bold_font->StringWidth(m_title);
	float string_widths = title_width;
	float version_width = 0;
	float email_width = 0;
	float web_width = 0;
	if(m_version)
		version_width = be_plain_font->StringWidth(m_version);
	if(string_widths < version_width)
		string_widths = version_width;
	if(m_email)
		email_width = be_plain_font->StringWidth(m_email);
	if(string_widths < email_width)
		string_widths = email_width;
	if(m_web)
		web_width = be_plain_font->StringWidth(m_web);
	if(string_widths < web_width)
		string_widths = web_width;
	for(line=0; line<m_num_lines; line++)
		if(m_lines[line] == NULL)
			m_lines[line] = (char*)"";
	float* body_lengths = NULL;
	float body_width = 0;
	if(m_num_lines > 0)
	{
		body_lengths = new float[m_num_lines];
		body_width = GetStringsMaxWidth((const char**)m_lines,m_num_lines,be_plain_font,
			body_lengths);
		if(string_widths < body_width)
			string_widths = body_width;
	}
	for(line=0; line<m_num_lines; line++)
		if(m_lines[line][0] == 0)
			m_lines[line] = NULL;

	string_widths = MAX(280, string_widths);
	
	m_title_rect.left = logo_right + 10 + floor((string_widths-title_width)/2);
	m_title_rect.top = m_logo_rect.top + (floor(((m_logo_rect.bottom-m_logo_rect.top)-
		bold_height)/2));
	if(m_title_rect.top < 10)
		m_title_rect.top = 10;
	m_title_rect.right = m_title_rect.left + title_width;
	m_title_rect.bottom = m_title_rect.top + bold_height;

	float curr_pos = m_title_rect.bottom + ceil(bold_height/4);
	if(m_version)
	{
		m_version_rect.left = logo_right + 10 + floor((string_widths-version_width)/2);
		m_version_rect.top = curr_pos;
		m_version_rect.right = m_version_rect.left + version_width;
		m_version_rect.bottom = m_version_rect.top + plain_height;
		curr_pos += plain_spacing;
	}
	if(m_num_lines > 0)
		curr_pos += ceil(bold_height*1.4);

	for(int i=0; i<m_num_lines; i++)
	{
		if(m_lines[i])
		{
			m_text_rects[i].left = logo_right + 10 + floor((string_widths-body_lengths[i])/2);
			m_text_rects[i].top = curr_pos;
			m_text_rects[i].right = m_text_rects[i].left + body_lengths[i];
			m_text_rects[i].bottom = m_text_rects[i].top + plain_height;
		}
		if(i<m_num_lines-1)
			curr_pos += plain_spacing;
		else
			curr_pos += plain_height;
	}
	if(body_lengths)
		delete[] body_lengths;

	if (m_email) {
		m_email_rect.left = logo_right + 10 + floor((string_widths-email_width)/2);
		m_email_rect.top = curr_pos;
		m_email_rect.right = m_email_rect.left + email_width;
		m_email_rect.bottom = m_email_rect.top + plain_height;
		curr_pos += plain_spacing;
	}

	if (m_web) {
		m_web_rect.left = logo_right + 10 + floor((string_widths-web_width)/2);
		m_web_rect.top = curr_pos;
		m_web_rect.right = m_web_rect.left + web_width;
		m_web_rect.bottom = m_web_rect.top + plain_height;
		curr_pos += plain_spacing;
	}

	if (credits) {
		BRect label_rect;
		label_rect.left = logo_right - 10;
		label_rect.top = curr_pos+5;
		label_rect.right = label_rect.left+60;
		label_rect.bottom = label_rect.top + plain_height;
		m_credits_label = new BStringView( label_rect, "label", "Credits:");
		curr_pos += 5+label_rect.Height();
		BRect credits_rect;
		credits_rect.left = logo_right - 10;
		credits_rect.top = curr_pos+2;
		credits_rect.right = credits_rect.left+string_widths+26;
		credits_rect.bottom = credits_rect.top + 5*plain_height+12;
		curr_pos += credits_rect.Height();
		BRect text_rect = credits_rect;
		text_rect.OffsetTo(0,0);
		text_rect.InsetBy(4,4);
		m_credits_view = new BTextView( credits_rect, "credits", text_rect,
												  B_FOLLOW_NONE, B_WILL_DRAW);
		m_credits_view->MakeEditable( false);
		m_credits_view->MakeSelectable( false);
		m_credits_view->SetText( credits);
		BmSetViewUIColor( m_credits_view, B_UI_CONTROL_BACKGROUND_COLOR);
		BmSetLowUIColor( m_credits_view, B_UI_CONTROL_BACKGROUND_COLOR);
	}

	if(m_bitmap && curr_pos < m_logo_rect.bottom)
		curr_pos = m_logo_rect.bottom;

	//curr_pos is now at the bottom of the last line of text, or the bitmap, whichever is lower
	//Resize and move the window
	BRect bounds(0,0,logo_right+10+string_widths+10,curr_pos+6);
	m_left_of_logo.bottom = bounds.bottom;
	ResizeTo(bounds.right,bounds.bottom);
	BView* content_view = new AboutView(bounds,this);
	AddChild(content_view);
	content_view->AddChild( m_credits_label);
	content_view->AddChild( m_credits_view);
	BRect screen_limits = BScreen().Frame();
	MoveTo(screen_limits.left+floor((screen_limits.Width()-bounds.Width())/2),
		screen_limits.top+floor((screen_limits.Height()-bounds.Height())/2));
}


ImageAboutWindow::~ImageAboutWindow()
{
	delete[] m_title;
	delete[] m_version;
	while(m_num_lines > 0)
	{
		if(m_lines[m_num_lines-1])
			delete[] m_lines[m_num_lines-1];
		m_num_lines--;
	}
	if(m_lines)
		delete[] m_lines;
	if(m_text_rects)
		delete[] m_text_rects;
	delete[] m_web;
	delete[] m_email;
	be_app->PostMessage(c_about_window_closed);
}


void ImageAboutWindow::MessageReceived(BMessage *message)
{
	if(message->what == c_about_window_to_front)
	{
		Minimize(false);
		Activate(true);
	}
}

void ImageAboutWindow::MouseDown(BPoint point) { 
	if (m_email && m_email_rect.Contains( point)) {
		BMessage msg(c_about_window_url_invoked);
		msg.AddString("url", m_email);
		be_app->PostMessage(&msg);
	}
	if (m_web && m_web_rect.Contains( point)) {
		BMessage msg(c_about_window_url_invoked);
		msg.AddString("url", m_web);
		be_app->PostMessage(&msg);
	}
}

void ImageAboutWindow::ScrollCredits() {
	SetPulseRate(10*1000);
	float height = m_credits_view->Bounds().Height();
	float text_height = m_credits_view->TextHeight(0,100000);
	if (++m_credits_pos > text_height-height)
		m_credits_pos = 0;
	m_credits_view->ScrollTo( 0, m_credits_pos);
}

void ImageAboutWindow::DrawContent(BView* view, BRect update_rect)
{
	view->SetHighColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 2));
	if(update_rect.Intersects(m_left_of_logo))
		view->FillRect(m_left_of_logo);
	if(update_rect.Intersects(m_logo_rect)) {
		view->SetDrawingMode(B_OP_ALPHA);
		view->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		view->DrawBitmap(m_bitmap,m_logo_rect.LeftTop());
		view->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		view->SetDrawingMode(B_OP_OVER);
	}
	view->SetHighColor(ui_color( B_UI_PANEL_TEXT_COLOR));
	if(update_rect.Intersects(m_title_rect))
	{
		view->SetFont(be_bold_font);
		view->DrawString(m_title,BPoint(m_title_rect.left,m_title_rect.top+m_bold_font_ascent));
		view->SetFont(be_plain_font);
	}
	if(m_version && update_rect.Intersects(m_version_rect))
		view->DrawString(m_version,BPoint(m_version_rect.left,m_version_rect.top+
			m_plain_font_ascent));
	if(m_email && update_rect.Intersects(m_email_rect)) {
		view->SetHighColor( ui_color( B_UI_CONTROL_HIGHLIGHT_COLOR));
		view->DrawString(m_email,BPoint(m_email_rect.left,m_email_rect.top+m_plain_font_ascent));
		view->StrokeLine(BPoint(m_email_rect.left,m_email_rect.top+m_plain_font_ascent+2), 
							  BPoint(m_email_rect.right,m_email_rect.top+m_plain_font_ascent+2));
	}
	if(m_web && update_rect.Intersects(m_web_rect)) {
		view->SetHighColor( ui_color( B_UI_CONTROL_HIGHLIGHT_COLOR));
		view->DrawString(m_web,BPoint(m_web_rect.left,m_web_rect.top+m_plain_font_ascent));
		view->StrokeLine(BPoint(m_web_rect.left,m_web_rect.top+m_plain_font_ascent+2), 
							  BPoint(m_web_rect.right,m_web_rect.top+m_plain_font_ascent+2));
	}
	view->SetHighColor( ui_color( B_UI_PANEL_TEXT_COLOR));
	for(int i=0; i<m_num_lines; i++)
		if(m_lines[i] && update_rect.Intersects(m_text_rects[i]))
			view->DrawString(m_lines[i],BPoint(m_text_rects[i].left,m_text_rects[i].top+
				m_plain_font_ascent));
}


//******************************************************************************************************
//**** AboutView
//******************************************************************************************************
AboutView::AboutView(BRect rect, ImageAboutWindow* parent)
: BView(rect,NULL,B_FOLLOW_ALL_SIDES,B_WILL_DRAW|B_PULSE_NEEDED)
{
	m_parent = parent;
	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	SetFont(be_plain_font);
}


void AboutView::Draw(BRect update_rect)
{
	m_parent->DrawContent(this,update_rect);
}

void AboutView::MouseDown(BPoint point) { 
	BView::MouseDown( point);
	m_parent->MouseDown( point);
}

void AboutView::Pulse(void) { 
	m_parent->ScrollCredits();
}
