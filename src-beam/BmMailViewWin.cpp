/*
	BmMailViewWin.cpp
		$Id$
*/

#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "PrefilledBitmap.h"

#include "Beam.h"
#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailViewWin
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-view window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailViewWin* BmMailViewWin::CreateInstance() 
{
	BmMailViewWin* win = new BmMailViewWin;
	win->ReadStateInfo();
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewWin::BmMailViewWin()
	:	inherited( "MailViewWin", BRect(50,50,800,600), "View Mail", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
{
	CreateGUI();
}

/*------------------------------------------------------------------------------*\
	CreateGUI()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::CreateGUI() {
	mOuterGroup = 
		new VGroup(
			minimax( 500, 400, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mNewButton = new BmToolbarButton( "New", 
																 TheResources->IconByName("Button_New"), 
																 new BMessage(BMM_NEW_MAIL), this, 
																 "Compose a new mail message"),
					mReplyButton = new BmToolbarButton( "Reply", 
																	TheResources->IconByName("Button_Reply"), 
																	new BMessage(BMM_REPLY), this, 
																	"Reply to sender only"),
					mReplyAllButton = new BmToolbarButton( "Reply All", 
																		TheResources->IconByName("Button_ReplyAll"), 
																		new BMessage(BMM_REPLY_ALL), this, 
																		"Reply to sender and all recipients"),
					mForwardButton = new BmToolbarButton( "Forward", 
																	  TheResources->IconByName("Button_Forward"), 
																	  new BMessage(ThePrefs->GetInt( "DefaultForwardType", BMM_FORWARD_INLINE)), this, 
																	  "Forward mail to somewhere else"),
					mRedirectButton = new BmToolbarButton( "Redirect", 
																	 TheResources->IconByName("Button_Redirect"), 
																	 new BMessage(BMM_REDIRECT), this, 
																	 "Redirect message to somewhere else (preserves original message)"),
					mPrintButton = new BmToolbarButton( "Print", 
																	TheResources->IconByName("Button_Print"), 
																	new BMessage(BMM_PRINT), this, 
																	"Print selected messages(s)"),
					mTrashButton = new BmToolbarButton( "Delete", 
																	TheResources->IconByName("Button_Trash"), 
																	new BMessage(BMM_TRASH), this, 
																	"Move selected messages to Trash"),
					new Space(),
					0
				)
			),
			new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);
		
	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewWin::~BmMailViewWin() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
MMenuBar* BmMailViewWin::CreateMenu() {
	MMenuBar* menubar = new MMenuBar();
	BMenu* menu = NULL;
	// File
	menu = new BMenu( "File");
	menu->AddItem( new BMenuItem( "Page Setup...", new BMessage( BMM_PAGE_SETUP)));
	menu->AddItem( new BMenuItem( "Print Message...", new BMessage( BMM_PRINT)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Quit Beam", new BMessage( B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( new BMenuItem( "Cut", new BMessage( B_CUT), 'X'));
	menu->AddItem( new BMenuItem( "Copy", new BMessage( B_COPY), 'C'));
	menu->AddItem( new BMenuItem( "Select All", new BMessage( B_SELECT_ALL), 'A'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Find...", new BMessage( BMM_FIND), 'F'));
	menu->AddItem( new BMenuItem( "Find Next", new BMessage( BMM_FIND_NEXT), 'G'));
	menubar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( new BMenuItem( "New Message", new BMessage( BMM_NEW_MAIL), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Reply", new BMessage( BMM_REPLY), 'R'));
	menu->AddItem( new BMenuItem( "Reply To All", new BMessage( BMM_REPLY_ALL), 'R', B_SHIFT_KEY));
	if (ThePrefs->GetInt( "DefaultForwardType", BMM_FORWARD_INLINE) == BMM_FORWARD_INLINE) {
		menu->AddItem( new BMenuItem( "Forward As Attachment", new BMessage( BMM_FORWARD_ATTACHED), 'J', B_SHIFT_KEY));
		menu->AddItem( new BMenuItem( "Forward Inline", new BMessage( BMM_FORWARD_INLINE), 'J'));
		menu->AddItem( new BMenuItem( "Forward Inline (With Attachments)", new BMessage( BMM_FORWARD_INLINE_ATTACH)));
	} else {
		menu->AddItem( new BMenuItem( "Forward As Attachment", new BMessage( BMM_FORWARD_ATTACHED), 'J'));
		menu->AddItem( new BMenuItem( "Forward Inline", new BMessage( BMM_FORWARD_INLINE), 'J', B_SHIFT_KEY));
		menu->AddItem( new BMenuItem( "Forward Inline (With Attachments)", new BMessage( BMM_FORWARD_INLINE_ATTACH)));
	}
	menu->AddItem( new BMenuItem( "Redirect", new BMessage( BMM_REDIRECT), 'B'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Apply Filter", new BMessage( BMM_FILTER)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Move To Trash", new BMessage( BMM_TRASH), 'T'));
	menubar->AddItem( menu);

	return menubar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMailViewWin::CreateMailView( minimax minmax, BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, false);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_COPY:
			case B_CUT: 
			case B_PASTE: 
			case B_UNDO: 
			case B_SELECT_ALL: {
				BView* focusView = CurrentFocus();
				if (focusView)
					PostMessage( msg, focusView);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailViewWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::ShowMail( BmMailRef* ref) {
	mMailView->ShowMail( ref);
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailViewWin::QuitRequested() {
	BM_LOG2( BM_LogMainWindow, BString("MailViewWin has been asked to quit"));
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMailViewWin::Quit() {
	mMailView->DetachModel();
	BM_LOG2( BM_LogMainWindow, BString("MailViewWin has quit"));
	inherited::Quit();
}

