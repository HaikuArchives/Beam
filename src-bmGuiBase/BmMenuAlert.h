//Name:		BmMenuAlert.h
//Author:	Oliver Tappe, heavily based on TextEntryAlert by Brian Tietz


#ifndef _TEXT_ENTRY_ALERT_H_
#define _TEXT_ENTRY_ALERT_H_


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <MessageFilter.h>

#include <MWindow.h>

#include "SantaPartsForBeam.h"

#include "BmString.h"

class MTextView;
class BmMenuControl;

//******************************************************************************************************
//**** BmMenuAlert
//******************************************************************************************************
class IMPEXPBMGUIBASE BmMenuAlert : public MWindow
{
	public:
		BmMenuAlert(float width, float height, const char* title, const char* info_text, 
			BmMenuControl* menu, const char* button_0_label, const char* button_1_label = NULL,
			button_width width_style = B_WIDTH_AS_USUAL,
			window_look look = B_MODAL_WINDOW_LOOK,
			window_feel feel = B_MODAL_APP_WINDOW_FEEL,
			uint32 flags = B_NOT_RESIZABLE|B_ASYNCHRONOUS_CONTROLS);
		~BmMenuAlert();

		int32 Go(char* selection_buffer, int32 buffer_size);
			//Synchronous version: The function doesn't return until the user has clicked a button and
			//the panel has been removed from the screen.  The value it returns is the index of 
			//the clicked button (0 or 1, left-to-right).  The user-entered (or unchanged) text is
			//stored in text_entry_buffer.  The BmMenuAlert is deleted before it returns, and should
			//be considered invalid after Go is called.  If the BmMenuAlert is sent a
			//B_QUIT_REQUESTED message while the panel is still on-screen, it returns -1.
		status_t Go(BInvoker* invoker);
			//Asynchronous version: The function returns immediately (with B_OK) and the button index is
			//delivered as the int32 "which" field of the BMessage that's sent to the BInvoker's target,
			//and the selection is delivered as the string "selection" field of the BMessage.  The
			//TextEntryBox will take posession of the BInvoker and delete it when finished.  The
			//BmMenuAlert is deleted when the user hits any of the buttons, and should be considered
			//invalid after Go is called.  If the BmMenuAlert is sent a B_QUIT_REQUESTED message
			//while the panel is still on-screen, it suppresses sending of the message.

		void SetShortcut(int32 index, char shortcut);
		char Shortcut(int32 index) const;
			//These functions set and return the shortcut character that's mapped to the button at
			//index. A given button can have only one shortcut except for the rightmost button, which, 
			//in addition to the shortcut that you give it here, is always mapped to B_ENTER.  If you 
			//create a "Cancel" button, you should give it a shortcut of B_ESCAPE.   If multi_line was
			//specified as true in the BmMenuAlert constructor, no shortcuts are allowed, including
			//the built-in B_ENTER shortcut on the rightmost button.

		//BWindow overrides
		virtual void MessageReceived(BMessage* message);
		virtual void Quit();

	private:
		MTextView* m_label_view;
		BmMenuControl* m_menu_field;
		static filter_result KeyDownFilterStatic(BMessage* message, BHandler** target,
			BMessageFilter* filter);
		filter_result KeyDownFilter(BMessage* message);
		MButton* m_buttons[2];
		char m_shortcut[2];
		BmString m_selection;

		//For the synchronous version (pointers point to data areas owned by thread that called Go)
		sem_id m_done_mutex;		//Mutex to release when the user hits a button or the window closes
		char* m_selection_buffer;	//Buffer to store the selection when the user hits a button
		int32* m_button_pressed;	//Place to store the button index that the user hit
		int32 m_buffer_size;

		//For the asynchronous version
		BInvoker *m_invoker;		//I own this object and will delete it when done.
};


#endif //_TEXT_ENTRY_ALERT_H_
