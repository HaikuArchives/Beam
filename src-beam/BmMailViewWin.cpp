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
#include "BmLogHandler.h"
#include "BmMailRef.h"
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
	:	inherited( "MailViewWin", BRect(50,50,800,600), "View Mail", B_TITLED_WINDOW_LOOK, 
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
		
	// temporarily disabled:
	mRedirectButton->SetEnabled( false);
	mPrintButton->SetEnabled( false);

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
			case BMM_NEW_MAIL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_REPLY:
			case BMM_REPLY_ALL:
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail) {
					const BmRef<BmMailRef> mailRef = mail->MailRef();
					if (mailRef) {
						BMessage msg2( msg->what);
						msg2.AddPointer( BmApplication::MSG_MAILREF, static_cast< void*>( mailRef.Get()));
						mailRef->AddRef();	// the message now refers to the mailRef, too
						be_app_messenger.SendMessage( &msg2, &msg2);
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

