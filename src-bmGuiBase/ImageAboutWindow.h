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


#ifndef _ABOUT_WINDOW_H_
#define _ABOUT_WINDOW_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <Window.h>
#include <View.h>


//******************************************************************************************************
//**** Forward name declarations
//******************************************************************************************************
class AboutView;
class BTextView;
class BStringView;


//******************************************************************************************************
//**** Constants
//******************************************************************************************************
const uint32 c_about_window_to_front = 'AbTF';
const uint32 c_about_window_closed = 'AbCl';
const uint32 c_about_window_url_invoked = 'AbUr';


//******************************************************************************************************
//**** ImageAboutWindow
//******************************************************************************************************
class ImageAboutWindow : public BWindow
{
	public:
		//Constructor and destructor
		ImageAboutWindow(const char* window_title, const char* app_title, const BBitmap* bmap,
			float icon_sidebar_offset, const char* body_text, const char* email=NULL, 
			const char* web=NULL, const char* credits=NULL);
			//icon_resource_name specifies the name of a 'bits' resource holding a logo image file for
			//loading by BTranslationUtils.   The image should have dark grey (R184,G184,B184) on the
			//left hand side, and light grey (R216,G216,B216) on the right hand side of the center of
			//the logo image.  The icon_sidebar_offset is the offset into this image of the first
			//column of the lighter color.  The short version string from the application version
			//resource is loaded and displayed in the about window.
 		virtual ~ImageAboutWindow();
		void MouseDown(BPoint point);
		void ScrollCredits( void);

		//BWindow overrides
		virtual void MessageReceived(BMessage *message);
		
	private:
		//Draw method
		friend AboutView;
		void DrawContent(BView* view, BRect update_rect);

		BBitmap* m_bitmap;
		char* m_title;
		char* m_version;
		char* m_email;
		char* m_web;
		int32 m_num_lines;
		char** m_lines;

		BRect m_title_rect;
		BRect m_version_rect;
		BRect m_email_rect;
		BRect m_web_rect;
		BRect* m_text_rects;

		BRect m_logo_rect;
		BRect m_above_logo;
		BRect m_left_of_logo;
		BRect m_below_logo;

		BStringView* m_credits_label;
		BTextView* m_credits_view;
		float m_credits_pos;
		
		float m_bold_font_ascent;
		float m_plain_font_ascent;
};


//******************************************************************************************************
//**** AboutView
//******************************************************************************************************
class AboutView : public BView
{
	private:
		AboutView(BRect rect, ImageAboutWindow* parent);

		ImageAboutWindow* m_parent;

		friend ImageAboutWindow;

	public:
		//BView override
		void Draw(BRect update_rect);
		void MouseDown(BPoint point);
		void Pulse( void);
};


#endif //_ABOUT_WINDOW_H_
