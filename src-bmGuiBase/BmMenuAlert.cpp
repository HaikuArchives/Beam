//Name:		BmMenuAlert.cpp
//Author:	Oliver Tappe, heavily based on TextEntryAlert by Brian Tietz


//******************************************************************************************************
//**** System header files
//******************************************************************************************************
#include <string.h>
#include <Button.h>
#include <MenuItem.h>
#include <Menu.h>
#include <Screen.h>

#ifdef __POWERPC__
#define BM_BUILDING_SANTAPARTSFORBEAM 1
#endif

//******************************************************************************************************
//**** Project header files
//******************************************************************************************************
#include "Colors.h"
#include "NewStrings.h"

#include <HGroup.h>
#include <MButton.h>
#include <MTextView.h>
#include <Space.h>
#include <VGroup.h>

#include "BmMenuAlert.h"
#include "BmMenuControl.h"
#include "BmMenuControllerBase.h"

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
//**** BmMenuAlert
//******************************************************************************************************


BmMenuAlert::BmMenuAlert(float width, float height, const char* title, const char* info_text, 
	BmMenuControl* menu, const char* button_0_label, const char* button_1_label,
	button_width width_style, window_look look, window_feel feel, uint32 flags)
: MWindow(BRect(1,1,width,height),title,look,feel,flags)
, m_menu_field( menu)
{
	VGroup * vg = 
		new VGroup(
			m_label_view = new MTextView(),
			menu,
			new Space(minimax(0,5,1e5,5)),
			new HGroup(
				new Space(),
				m_buttons[0] = new MButton( button_0_label, (ulong)0, minimax( 74, -1, 74, -1)),
				m_buttons[1] = new MButton( button_1_label, (ulong)1, minimax( 74, -1, 74, -1)),
				0
			),
			0
		);

	m_label_view->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	m_label_view->SetText(info_text);
	m_label_view->MakeEditable(false);
	m_label_view->MakeSelectable(false);
	m_label_view->SetWordWrap(false);
	float th = m_label_view->TextHeight(0,10000);
	m_label_view->ct_mpm = minimax(width,th,width,th);
	
	AddChild( dynamic_cast<BView*>(vg));

	// position window on screen:
	BScreen screen( this);
	BRect screenFrame = screen.Frame();
	MoveTo( (screenFrame.Width()-width)/2.0, (screenFrame.Height()-height)/2.0);

/*
	BmMenuControllerBase* mc = dynamic_cast<BmMenuControllerBase*>( menu->Menu());
	if (mc)
		mc->MsgTarget( this);
*/

	//Complete the setup
	m_invoker = NULL;
	m_done_mutex = B_ERROR;
	m_selection_buffer = NULL;
	m_button_pressed = NULL;
	AddCommonFilter(new BMessageFilter(B_KEY_DOWN,KeyDownFilterStatic));
	m_shortcut[0] = 0;
	m_shortcut[1] = 0;
	m_selection = menu->MenuItem()->Label();
}


BmMenuAlert::~BmMenuAlert()
{
	if(m_invoker)
		delete m_invoker;
}


int32 BmMenuAlert::Go(char* selection_buffer, int32 buffer_size)
{
	int32 button_pressed = -1;
	m_selection_buffer = selection_buffer;
	m_button_pressed = &button_pressed;
	m_buffer_size = buffer_size;
	sem_id done_mutex = create_sem(0,"menu_alert_done");
	m_done_mutex = done_mutex;
	if(done_mutex < B_NO_ERROR)
	{
		Quit();
		return -1;
	}
	m_menu_field->MakeFocus(true);
	Show();
	acquire_sem(done_mutex);
	delete_sem(done_mutex);
	return button_pressed;
}


status_t BmMenuAlert::Go(BInvoker *invoker)
{
	m_invoker = invoker;
	m_menu_field->MakeFocus(true);
	Show();
	return B_NO_ERROR;
}


void BmMenuAlert::SetShortcut(int32 index, char shortcut)
{
	m_shortcut[index] = shortcut;
}


char BmMenuAlert::Shortcut(int32 index) const
{
	return m_shortcut[index];
}


filter_result BmMenuAlert::KeyDownFilterStatic(BMessage* message, BHandler**,
	BMessageFilter* filter)
{
	return ((BmMenuAlert*)filter->Looper())->KeyDownFilter(message);
}


filter_result BmMenuAlert::KeyDownFilter(BMessage *message)
{
	if(message->what == B_KEY_DOWN)
	{
		char byte;
		if(message->FindInt8("byte",(int8*)&byte) == B_NO_ERROR && !message->HasInt8("byte",1))
		{
			char space = B_SPACE;
			if(m_shortcut[0] && byte == m_shortcut[0])
			{
				m_buttons[0]->KeyDown(&space,1);
				return B_SKIP_MESSAGE;
			}
			else if(byte == B_ENTER || m_shortcut[1] && byte == m_shortcut[1])
			{
				m_buttons[1]->KeyDown(&space,1);
				return B_SKIP_MESSAGE;
			}
		}
	}
	return B_DISPATCH_MESSAGE;
}


void BmMenuAlert::MessageReceived(BMessage* message)
{
	switch(message->what)
	{
		case M_BUTTON_SELECTED: {
			int32 which;
			if(message->FindInt32(M_BUTTON_ID,&which) == B_NO_ERROR)
			{
				if(m_done_mutex < B_NO_ERROR)
				{
					//Asynchronous version: add the necessary fields
					BMessage* message = m_invoker->Message();
					if(message && (message->AddInt32("which",which) == B_NO_ERROR ||
						message->ReplaceInt32("which",which) == B_NO_ERROR) &&
						(message->AddString("selection",m_selection.String()) == B_NO_ERROR ||
						message->ReplaceString("selection",m_selection.String()) == B_NO_ERROR))
						m_invoker->Invoke();
				}
				else
				{
					//Synchronous version: set the result button and text buffer, then release the thread
					//that created me
					*m_button_pressed = which;
					if(m_selection_buffer)
						Strtcpy(m_selection_buffer,m_selection.String(),m_buffer_size);
					release_sem(m_done_mutex);
					m_done_mutex = B_ERROR;
				}
				PostMessage(B_QUIT_REQUESTED);
			}
			break;
		}
		case BM_MENUITEM_SELECTED: {
			BView* srcView = NULL;
			message->FindPointer( "source", (void**)&srcView);
			BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
			if (item) {
				BMenuItem* currItem = item;
				BMenu* currMenu = item->Menu();
				BmString path;
				while( currMenu && currItem 
				&& currItem != m_menu_field->MenuItem()) {
					if (!path.Length())
						path.Prepend( BmString(currItem->Label()));
					else
						path.Prepend( BmString(currItem->Label()) << "/");
					currItem = currMenu->Superitem();
					currMenu = currMenu->Supermenu();
				}
				m_selection = path;
				item->SetMarked( true);
				m_menu_field->MenuItem()->SetLabel( path.String());
			} else {
				m_selection = "";
				m_menu_field->ClearMark();
			}
			break;
		} 
		default: {
			MWindow::MessageReceived( message);
		}
	}
}

void BmMenuAlert::Quit()
{
	//Release the mutex if I'm synchronous and I haven't released it yet
	if(m_done_mutex >= B_NO_ERROR)
		release_sem(m_done_mutex);
	BWindow::Quit();
}


