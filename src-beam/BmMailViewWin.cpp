/*
	BmMailViewWin.cpp
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "PrefilledBitmap.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailHeaderView.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
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

float BmMailViewWin::nNextXPos = 300;
float BmMailViewWin::nNextYPos = 100;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-view window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailViewWin* BmMailViewWin::CreateInstance( BmMailRef* mailRef) {
	BmMailViewWin* win = new BmMailViewWin( mailRef);
	win->ReadStateInfo();
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewWin::BmMailViewWin( BmMailRef* mailRef)
	:	inherited( "MailViewWin", BRect(50,50,800,600), "View Mail", 
					  ThePrefs->GetBool( "UseDocumentResizer", false) 
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
{
	CreateGUI();
	ShowMail( mailRef);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailViewWin::UnarchiveState( BMessage* archive) {
	status_t ret = inherited::UnarchiveState( archive);
	if (ret == B_OK) {
		BRect frame = Frame();
		if (nNextXPos != frame.left || nNextYPos != frame.top) {
			nNextXPos = frame.left;
			nNextYPos = frame.top;
		} else {
			nNextXPos += 10;
			nNextYPos += 16;
			if (nNextYPos > 300) {
				nNextXPos = 300;
				nNextYPos = 100;
			}
		}
		BRect scrFrame = bmApp->ScreenFrame();
		frame.bottom = MIN( frame.bottom, scrFrame.bottom-5);
		frame.right = MIN( frame.right, scrFrame.right-5);
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( frame.Width(), frame.Height());
		WriteStateInfo();
	} else {
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( 400, 400);
		WriteStateInfo();
	}
	return ret;
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
																	"Print this messages"),
					mTrashButton = new BmToolbarButton( "Delete", 
																	TheResources->IconByName("Button_Trash"), 
																	new BMessage(BMM_TRASH), this, 
																	"Move this message to Trash"),
					new Space(),
					0
				)
			),
			new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);
		
	mMailView->StartWatching( this, BM_NTFY_MAIL_VIEW);

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
	menu->AddItem( CreateMenuItem( "Page Setup...", BMM_PAGE_SETUP));
	menu->AddItem( CreateMenuItem( "Print Message...", BMM_PRINT));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), bmApp);
	menubar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( CreateMenuItem( "Cut", B_CUT));
	menu->AddItem( CreateMenuItem( "Copy", B_COPY));
	menu->AddItem( CreateMenuItem( "Select All", B_SELECT_ALL));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Find...", BMM_FIND));
	menu->AddItem( CreateMenuItem( "Find Next", BMM_FIND_NEXT));
	menubar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( CreateMenuItem( "New Message", BMM_NEW_MAIL));
	menu->AddSeparatorItem();
	BmMailRefView::AddMailRefMenu( menu);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Toggle Header Mode", BMM_SWITCH_HEADER));
	menu->AddItem( CreateMenuItem( "Show Raw Message", BMM_SWITCH_RAW));
	menubar->AddItem( menu);

	// temporary deactivations:
	menubar->FindItem( BMM_FIND)->SetEnabled( false);
	menubar->FindItem( BMM_FIND_NEXT)->SetEnabled( false);
	menubar->FindItem( BMM_FILTER)->SetEnabled( false);

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
void BmMailViewWin::BeginLife() {
	BMenuBar* menuBar = KeyMenuBar();
	if (menuBar) {
		menuBar->FindItem( BMM_SWITCH_RAW)->SetTarget( mMailView);
		menuBar->FindItem( BMM_SWITCH_RAW)->SetMarked( mMailView->ShowRaw());
		menuBar->FindItem( BMM_SWITCH_HEADER)->SetTarget( mMailView->HeaderView());
	}
	// temporarily disabled:
	mPrintButton->SetEnabled( false);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BMM_NEW_MAIL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_REDIRECT:
			case BMM_REPLY:
			case BMM_REPLY_ALL:
			case BMM_TRASH:
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail) {
					BString selectedText;
					int32 start, finish;
					mMailView->GetSelection( &start, &finish);
					if (start < finish)
						selectedText.SetTo( mMailView->Text()+start, finish-start);
					const BmRef<BmMailRef> mailRef = mail->MailRef();
					if (mailRef) {
						BMessage msg2( msg->what);
						msg2.AddPointer( BmApplication::MSG_MAILREF, static_cast< void*>( mailRef.Get()));
						mailRef->AddRef();	// the message now refers to the mailRef, too
						if (selectedText.Length())
							msg2.AddString( BmApplication::MSG_SELECTED_TEXT, selectedText.String());
						be_app_messenger.SendMessage( &msg2, &msg2);
					}
					if (msg->what == BMM_TRASH)
						PostMessage( B_QUIT_REQUESTED);
				}
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_MAIL_VIEW: {
						bool hasMail = msg->FindBool( BmMailView::MSG_HAS_MAIL);
						// adjust menu:
						BMenuBar* menuBar = KeyMenuBar();
						menuBar->FindItem( BMM_FIND)->SetEnabled( false && hasMail);
						menuBar->FindItem( BMM_FIND_NEXT)->SetEnabled( false && hasMail);
						menuBar->FindItem( BMM_SWITCH_HEADER)->SetEnabled( hasMail);
						BMenuItem* item = menuBar->FindItem( BMM_SWITCH_RAW);
						item->SetEnabled( hasMail);
						item->SetMarked( mMailView->ShowRaw());
						break;
					}
				}
				break;
			}
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
void BmMailViewWin::ShowMail( BmMailRef* mailRef) {
	mMailView->ShowMail( mailRef);
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

