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


#ifndef _MULTILINE_TEXT_CONTROL_H_
#define _MULTILINE_TEXT_CONTROL_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <Control.h>
#include <TextView.h>


#include "BmGuiBase.h"

//******************************************************************************************************
//**** Forward name declarations
//******************************************************************************************************
class MultiLineTextControlTextView;


//******************************************************************************************************
//**** MultiLineTextControl
//******************************************************************************************************
class IMPEXPBMGUIBASE MultiLineTextControl : public BControl
{
	public:
		MultiLineTextControl(BRect frame, const char* name, const char* label, bool inline_label,
			const char *text, BMessage *message, uint32 resizing_mode = B_FOLLOW_LEFT | B_FOLLOW_TOP, 
			uint32 flags = B_WILL_DRAW | B_NAVIGABLE);
		virtual ~MultiLineTextControl();

		virtual	void AttachedToWindow();
		virtual void Draw(BRect update_rect);
		virtual void FrameResized(float width, float heigh);
		virtual void MakeFocus(bool flag = true);
		virtual	void SetLabel(const char* text);
		virtual	void SetEnabled( bool enabled);

		virtual void SetText(const char* text);
		const char* Text() const;
		virtual void SetTabAllowed(bool allowed);
		bool TabAllowed() const;
		BTextView* TextView() const;
		virtual void SetDivider(float divider);
		inline float Divider() const {return m_divider;}
		inline bool IsLabelInline() const {return m_inline_label;}
		virtual	void SetModificationMessage(BMessage* message);
			//As with BTextControl, MultiLineTextControl takes ownership (and responsibility for
			//deleting) the message passed in.  When the text is modified, before sending the message,
			//the following entries are added to it:
			//"when"	B_INT64_TYPE	When the user modified the text, as measured by the number of
			//							microseconds since 12:00:00 AM January 1, 1970.
			//"source"	B_POINTER_TYPE	A pointer to the MultiLineTextControl object.
		BMessage* ModificationMessage() const;

		virtual void ResizeToWithChildren(float width, float height);

		void ResetTextRect();
		
		inline void SetTextMargin(float margin) {m_text_margin = margin; ResetTextRect();}
		inline float TextMargin() {return m_text_margin;}

	protected:
		MultiLineTextControlTextView* m_text_view;

	private:
		friend class MultiLineTextControlTextView;

		void ReevaluateLabelRect();
		void Modified();

		BRect m_entry_text_rect;
		BRect m_label_text_rect;
		rgb_color m_dark_1_color;
		rgb_color m_dark_2_color;
		rgb_color m_focus_color;
		float m_divider;
		float m_label_font_ascent;
		float m_text_margin;
		bool m_inline_label;
		bool m_enabled;
		BMessage* m_modification_message;
};


//******************************************************************************************************
//**** MultiLineTextControlTextView
//******************************************************************************************************
class MultiLineTextControlTextView : public BTextView
{
	public:
		MultiLineTextControlTextView(BRect frame, uint32 flags, uint32 res_mode);

		virtual void MakeFocus(bool flag = true);
		virtual void KeyDown(const char* bytes, int32 num_bytes);
		virtual	void InsertText(const char* text, int32 length, int32 offset,
			const text_run_array* runs);
		virtual	void DeleteText(int32 from_offset, int32 to_offset);

		inline void SetTabAllowed(bool allowed) {m_tab_allowed = allowed;}
		inline bool TabAllowed() const {return m_tab_allowed;}

	private:
		bool m_tab_allowed;
		bool m_modified;
};


#endif //_MULTILINE_TEXT_CONTROL_H_

