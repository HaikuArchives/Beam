//Name:		ListSelectionAlert.h
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


#ifndef _LIST_SELECTION_ALERT_H_
#define _LIST_SELECTION_ALERT_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <List.h>
#include <TextView.h>
#include <Window.h>

#include "SantaPartsForBeam.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "DeepBevelView.h"

//******************************************************************************************************
//**** ListSelectionAlert
//******************************************************************************************************
class IMPEXPBMGUIBASE ListSelectionAlert : public BWindow
{
	public:
		ListSelectionAlert(const char* title, const char* info_text, BList& items, 
			const char* initial_selection,
			const char* button_0_label, const char* button_1_label = NULL,
			float min_list_box_width = 250, int min_list_box_height = 100,
			button_width width_style = B_WIDTH_AS_USUAL,
			const BRect* frame = NULL, window_look look = B_MODAL_WINDOW_LOOK,
			window_feel feel = B_MODAL_APP_WINDOW_FEEL,
			uint32 flags = B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS);
		~ListSelectionAlert();

		int32 Go(char* selection_buffer, int32 buffer_size);
			//Synchronous version: The function doesn't return until the user has clicked a button and
			//the panel has been removed from the screen.  The value it returns is the index of 
			//the clicked button (0 or 1, left-to-right).  The user-entered (or unchanged) text is
			//stored in text_entry_buffer.  The ListSelectionAlert is deleted before it returns, and should
			//be considered invalid after Go is called.  If the ListSelectionAlert is sent a
			//B_QUIT_REQUESTED message while the panel is still on-screen, it returns -1.
		status_t Go(BInvoker* invoker);
			//Asynchronous version: The function returns immediately (with B_OK) and the button index is
			//delivered as the int32 "which" field of the BMessage that's sent to the BInvoker's target,
			//and the text message is delivered as the string "selection_text" field of the BMessage.  The
			//ListSelectionBox will take posession of the BInvoker and delete it when finished.  The
			//ListSelectionAlert is deleted when the user hits any of the buttons, and should be considered
			//invalid after Go is called.  If the ListSelectionAlert is sent a B_QUIT_REQUESTED message
			//while the panel is still on-screen, it suppresses sending of the message.

		void SetShortcut(int32 index, char shortcut);
		char Shortcut(int32 index) const;

		BTextView* LabelView(void) const;
		ColumnListView* ListView(void) const;

		//BWindow overrides
		virtual void MessageReceived(BMessage* message);
		virtual void Quit();

	private:
		BTextView* m_label_view;
		ColumnListView* m_list_view;
		BButton* m_buttons[2];
		char m_shortcut[2];

		//For the synchronous version (pointers point to data areas owned by thread that called Go)
		sem_id m_done_mutex;		//Mutex to release when the user hits a button or the window closes
		char* m_selection_buffer;	//Buffer to store the user-entered selection when the user hits a button
		int32* m_button_pressed;	//Place to store the button index that the user hit
		int32 m_buffer_size;

		//For the asynchronous version
		BInvoker *m_invoker;		//I own this object and will delete it when done.
};


//******************************************************************************************************
//**** ListSelectionAlertBackgroundView
//******************************************************************************************************
class ListSelectionAlertBackgroundView : public DeepBevelView
{
	public:
		ListSelectionAlertBackgroundView(BRect frame, BRect list_rect);
		virtual void FrameResized(float width, float heigh);

	private:
		BRect m_list_rect;
};


#endif //_LIST_SELECTION_ALERT_H_
