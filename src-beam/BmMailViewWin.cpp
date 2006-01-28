/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include "BmString.h"

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "UserResizeSplitView.h"

#include "BeamApp.h"
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailHeaderView.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmToolbarButton.h"

/********************************************************************************\
	BmMailViewWin
\********************************************************************************/

float BmMailViewWin::nNextXPos = 300;
float BmMailViewWin::nNextYPos = 100;

const char* const BmMailViewWin::MSG_HSPLITTER = "bm:hspl";

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
					  ThePrefs->GetBool( "UseDocumentResizer", true) 
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mFilterMenu( NULL)
{
	CreateGUI();
	if (mailRef) {
		mMailRefView->StartJob( mailRef->ListModel().Get());
		ShowMail( mailRef);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailViewWin::ArchiveState( BMessage* archive) const {
	status_t ret = inherited::ArchiveState( archive)
						|| archive->AddFloat( MSG_HSPLITTER, 
													 mHorzSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailViewWin::UnarchiveState( BMessage* archive) {
	float hDividerPos;
	status_t ret = inherited::UnarchiveState( archive)
						|| archive->FindFloat( MSG_HSPLITTER, &hDividerPos);
	if (ret == B_OK) {
		mHorzSplitter->SetPreferredDividerLeftOrTop( hDividerPos);
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
		BRect scrFrame = beamApp->ScreenFrame();
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
	// Get maximum button size
	float width=0, height=0;
	BmToolbarButton::CalcMaxSize(width, height, "New");
	BmToolbarButton::CalcMaxSize(width, height, "Reply");
	BmToolbarButton::CalcMaxSize(width, height, "Reply All");
	BmToolbarButton::CalcMaxSize(width, height, "Forward");
	BmToolbarButton::CalcMaxSize(width, height, "Print");
	BmToolbarButton::CalcMaxSize(width, height, "Trash");

	int32 defaultFwdMsgType = 
		ThePrefs->GetString( "DefaultForwardType")=="Inline"
			? BMM_FORWARD_INLINE
			: BMM_FORWARD_ATTACHED;
	mOuterGroup = 
		new VGroup(
			minimax( 500, 400, 1E5, 1E5),
			CreateMenu(),
			mToolbar = new BmToolbar( 
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mNewButton = new BmToolbarButton( "New", 
																 width, height,
																 new BMessage(BMM_NEW_MAIL), this, 
																 "Compose a new mail message"),
					mReplyButton = new BmToolbarButton( "Reply", 
																 	width, height,
																	new BMessage(BMM_REPLY), this, 
																	"Reply to person or list", true),
					mForwardButton = new BmToolbarButton( "Forward", 
																 	  width, height,
																	  new BMessage( defaultFwdMsgType), this, 
																	  "Forward mail to somewhere else", true),
					mPrintButton = new BmToolbarButton( "Print", 
																   width, height,
																	new BMessage(BMM_PRINT), this, 
																	"Print this messages"),
					mTrashButton = new BmToolbarButton( "Trash", 
																   width, height,
																	new BMessage(BMM_TRASH), this, 
																	"Move this message to Trash"),
					new BmToolbarSpace(),
					0
				)
			),
			mHorzSplitter = new UserResizeSplitView( 
				new BetterScrollView(
					minimax(200,50,1E5,1E5), 
					mMailRefView = BmMailRefView::CreateInstance( 400, 200),
					BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
					| BM_SV_BUSYVIEW | BM_SV_CAPTION,
					"99999 messages"
				),
				new BmMailViewContainer(
					minimax(200,80,1E5,1E5), 
					mMailView = BmMailView::CreateInstance( BRect(0,0,400,200), 
																		 false)
				),
				"hsplitter", 0, B_HORIZONTAL, true, true, true, true, 
				false, B_FOLLOW_NONE
			),
			0
		);
		
	mMailRefView->TeamUpWith( mMailView);
	mMailView->TeamUpWith( mMailRefView);
	mReplyButton->AddActionVariation( "Reply", new BMessage(BMM_REPLY));
	mReplyButton->AddActionVariation( "Reply To List", new BMessage(BMM_REPLY_LIST));
	mReplyButton->AddActionVariation( "Reply To Person", new BMessage(BMM_REPLY_ORIGINATOR));
	mReplyButton->AddActionVariation( "Reply To All", new BMessage(BMM_REPLY_ALL));
	mForwardButton->AddActionVariation( "Forward As Attachment", new BMessage(BMM_FORWARD_ATTACHED));
	mForwardButton->AddActionVariation( "Forward Inline", new BMessage(BMM_FORWARD_INLINE));
	mForwardButton->AddActionVariation( "Forward Inline (With Attachments)", new BMessage(BMM_FORWARD_INLINE_ATTACH));

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
	AddItemToMenu( menu, CreateMenuItem( "Preferences...", BMM_PREFERENCES), beamApp);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), beamApp);
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
	mMailRefView->AddMailRefMenu( menu, this, false);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Toggle Header Mode", BMM_SWITCH_HEADER));
	menu->AddItem( CreateMenuItem( "Show Raw Message", BMM_SWITCH_RAW));
	menubar->AddItem( menu);

	return menubar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::BeginLife() {
	mToolbar->UpdateLayout(true);
	BMenuBar* menuBar = KeyMenuBar();
	if (menuBar) {
		menuBar->FindItem( BMM_SWITCH_RAW)->SetTarget( mMailView);
		menuBar->FindItem( BMM_SWITCH_RAW)->SetMarked( mMailView->ShowRaw());
		menuBar->FindItem( BMM_SWITCH_HEADER)->SetTarget( mMailView->HeaderView());
		menuBar->FindItem( BMM_PAGE_SETUP)->SetTarget( beamApp);
	}
	mMailView->MakeFocus( true);
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
			case BMM_MARK_AS:
			case BMM_REDIRECT:
			case BMM_REPLY:
			case BMM_REPLY_LIST:
			case BMM_REPLY_ORIGINATOR:
			case BMM_REPLY_ALL:
			case BMM_TRASH:
			case BMM_PRINT:
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH:
			case BMM_EDIT_AS_NEW: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail) {
					BmString selectedText;
					int32 start, finish;
					mMailView->GetSelection( &start, &finish);
					if (start < finish)
						selectedText.SetTo( mMailView->Text()+start, finish-start);
					const BmRef<BmMailRef> mailRef = mail->MailRef();
					if (mailRef) {
						BmMailRefVect* refVect = new BmMailRefVect();
						refVect->push_back( mailRef);
						msg->AddPointer( BeamApplication::MSG_MAILREF_VECT, 
											  static_cast< void*>( refVect));
						if (selectedText.Length())
							msg->AddString( BeamApplication::MSG_SELECTED_TEXT, 
												 selectedText.String());
						be_app_messenger.SendMessage( msg);
					}
					if (mMailRefView->IsHidden()
					&& (msg->what == BMM_TRASH
						|| (ThePrefs->GetBool("CloseViewWinAfterMailAction", true)
							&& (msg->what == BMM_REDIRECT
								|| msg->what == BMM_FORWARD_INLINE
								|| msg->what == BMM_FORWARD_INLINE_ATTACH
								|| msg->what == BMM_FORWARD_ATTACHED
								|| msg->what == BMM_REPLY
								|| msg->what == BMM_REPLY_LIST
								|| msg->what == BMM_REPLY_ORIGINATOR
								|| msg->what == BMM_REPLY_ALL))))
					{
						PostMessage( B_QUIT_REQUESTED);
					}
				}
				break;
			}
			case BMM_FILTER: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail && mail->MailRef()) {
					static int32 jobNum = 1;
					BMessage tmpMsg( BM_JOBWIN_FILTER);
					BmMailRefVect* refVect = new BmMailRefVect();
					refVect->push_back( mail->MailRef());
					tmpMsg.AddPointer( BeamApplication::MSG_MAILREF_VECT, 
											  static_cast< void*>( refVect));
					BmString jobName = msg->FindString( BmListModel::MSG_ITEMKEY);
					tmpMsg.AddString( BmListModel::MSG_ITEMKEY, jobName.String());
					jobName << jobNum++;
					tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
					TheJobStatusWin->PostMessage( &tmpMsg);
				}
				break;
			}
			case BMM_CREATE_FILTER: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail && mail->MailRef()) {
					BmMailRefVect* refVect = new BmMailRefVect();
					refVect->push_back( mail->MailRef());
					msg->AddPointer( BeamApplication::MSG_MAILREF_VECT, 
										  static_cast< void*>( refVect));
					BMessage appMsg( BMM_PREFERENCES);
					appMsg.AddString( "SubViewName", "Filters");
					appMsg.AddMessage( "SubViewMsg", msg);
					be_app_messenger.SendMessage( &appMsg);
				}
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_MAIL_VIEW: {
						bool hasMail = msg->FindBool( BmMailView::MSG_HAS_MAIL);
						// adjust menu:
						BMenuBar* menuBar = KeyMenuBar();
						menuBar->FindItem( BMM_FIND)->SetEnabled( hasMail);
						menuBar->FindItem( BMM_FIND_NEXT)->SetEnabled( hasMail);
						menuBar->FindItem( BMM_SWITCH_HEADER)->SetEnabled( hasMail);
						BMenuItem* item = menuBar->FindItem( BMM_SWITCH_RAW);
						item->SetEnabled( hasMail);
						item->SetMarked( mMailView->ShowRaw());
						break;
					}
				}
				break;
			}
			case BMM_FIND:
			case BMM_FIND_NEXT: {
				PostMessage( msg, mMailView);
				break;
			}
			case BMM_PREVIOUS_MESSAGE:
			case BMM_NEXT_MESSAGE: {
				const char bytes
					= (msg->what == BMM_NEXT_MESSAGE) 
						? B_DOWN_ARROW
						: B_UP_ARROW;
				mMailRefView->KeyDown( &bytes, 1);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailViewWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewWin::ShowMail( BmMailRef* mailRef, bool async) {
	mMailView->ShowMail( mailRef, async);
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailViewWin::QuitRequested() {
	BM_LOG2( BM_LogGui, BmString("MailViewWin has been asked to quit"));
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMailViewWin::Quit() {
	mMailView->DetachModel();
	BM_LOG2( BM_LogGui, BmString("MailViewWin has quit"));
	inherited::Quit();
}

