/*
	BmMainWindow.cpp
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


#ifdef BM_DEBUG_MEM
#include <Alert.h>
#endif
#include <Autolock.h>
#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "UserResizeSplitView.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMainWindow.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmPrefsWin.h"
#include "BmResources.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"


/********************************************************************************\
	BmMainMenuBar
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMainMenuBar()
		-	
\*------------------------------------------------------------------------------*/
BmMainMenuBar::BmMainMenuBar()
	:	inherited()
	,	inheritedController( "PopAccountListController")
	,	mAccountMenu( NULL)
{
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMainMenuBar::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOB_DONE:
			case BM_LISTMODEL_ADD:
			case BM_LISTMODEL_UPDATE:
			case BM_LISTMODEL_REMOVE: {
				if (!IsMsgFromCurrentModel( msg)) 
					break;
				const char* oldKey;
				if (msg->what == BM_LISTMODEL_UPDATE
				&& msg->FindString( BmListModel::MSG_OLD_KEY, &oldKey) != B_OK) 
					break;
				JobIsDone( true);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString(ControllerName()) << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmMainMenuBar::JobIsDone( bool completed) {
	if (completed && mAccountMenu) {
		BmAutolock lock( DataModel()->ModelLocker());
		lock.IsLocked()	 						|| BM_THROW_RUNTIME( BString() << ControllerName() << ":AddAllModelItems(): Unable to lock model");
		BMenuItem* old;
		while( (old = mAccountMenu->RemoveItem( (int32)0)))
			delete old;
		BmListModel *model = dynamic_cast<BmListModel*>( DataModel());
		BmModelItemMap::const_iterator iter;
		int i=0;
		for( iter = model->begin();  iter != model->end();  ++iter, ++i) {
			BmListModelItem* item = iter->second.Get();
			BMessage* msg = new BMessage( BMM_CHECK_MAIL);
			msg->AddString( BmPopAccountList::MSG_ITEMKEY, item->Key());
			if (item) {
				if (i<10)
					mAccountMenu->AddItem( new BMenuItem( item->Key().String(), msg, '0'+i));
				else
					mAccountMenu->AddItem( new BMenuItem( item->Key().String(), msg, i));
			}
		}
	}
}



/********************************************************************************\
	BmMainWindow
\********************************************************************************/

BmMainWindow* BmMainWindow::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	flag and access-function that indicate a user's request-to-stop:
\*------------------------------------------------------------------------------*/
bool BmMainWindow::nIsAlive = false;
bool BmMainWindow::IsAlive() {
	return BmMainWindow::nIsAlive;
}

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates the app's main window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMainWindow* BmMainWindow::CreateInstance() 
{
	if (!theInstance) {
		theInstance = new BmMainWindow;
		theInstance->ReadStateInfo();
	}
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMainWindow::BmMainWindow()
	:	inherited( "MainWindow", BRect(50,50,800,600), 
					  bmApp->BmAppNameWithVersion.String(),
					  B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 
					  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
	,	mMailFolderView( NULL)
	,	mMailRefView( NULL)
	,	mVertSplitter( NULL)
{
	CreateMailFolderView( minimax(0,100,300,1E5), 120, 100);
	CreateMailRefView( minimax(200,100,1E5,1E5), 400, 200);
	CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200));

	MView* mOuterGroup = 
		new VGroup(
			minimax( 600, 200, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mCheckButton = new BmToolbarButton( "Check", 
																	TheResources->IconByName("Button_Check"), 
																	new BMessage(BMM_CHECK_MAIL), this, 
																	"Check for new mail"),
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
																	 "Redirect message to somewhere else (preserves original sender)"),
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
			new Space(minimax(-1,2,-1,2)),
			new HGroup(
				mVertSplitter = new UserResizeSplitView( 
					mMailFolderView->ContainerView(),
					mHorzSplitter = new UserResizeSplitView( 
						mMailRefView->ContainerView(),
						mMailView->ContainerView(),
						"hsplitter", 150, B_HORIZONTAL, true, true, false, B_FOLLOW_NONE
					),
					"vsplitter", 120, B_VERTICAL, true, true, false, B_FOLLOW_NONE
				),
				0
			),
			0
		);

	mMailFolderView->TeamUpWith( mMailRefView);
	mMailRefView->TeamUpWith( mMailView);
	mMailView->TeamUpWith( mMailRefView);
	MailFolderSelectionChanged( 0);
	MailRefSelectionChanged( 0);
	MailViewChanged( false);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMainWindow::~BmMainWindow() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
MMenuBar* BmMainWindow::CreateMenu() {
	mMainMenuBar = new BmMainMenuBar();
	BMenu* menu = NULL;
	// File
	menu = new BMenu( "File");
	menu->AddItem( CreateMenuItem( "New Folder", BMM_NEW_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Rename Folder", BMM_RENAME_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Delete Folder", BMM_DELETE_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Recache Folder", BMM_RECACHE_MAILFOLDER));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Page Setup...", BMM_PAGE_SETUP));
	menu->AddItem( CreateMenuItem( "Print Message(s)...", BMM_PRINT, "Print Message..."));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Preferences...", BMM_PREFERENCES));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "About Beam...", B_ABOUT_REQUESTED));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED));
	mMainMenuBar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( CreateMenuItem( "Cut", B_CUT));
	menu->AddItem( CreateMenuItem( "Copy", B_COPY));
	menu->AddItem( CreateMenuItem( "Select All", B_SELECT_ALL));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Find...", BMM_FIND));
	menu->AddItem( CreateMenuItem( "Find Messages...", BMM_FIND_MESSAGES));
	menu->AddItem( CreateMenuItem( "Find Next", BMM_FIND_NEXT));
	mMainMenuBar->AddItem( menu);

	// Network
	BMenu* accMenu = new BMenu( "Check Mail For");
	mMainMenuBar->SetAccountMenu( accMenu);
	menu = new BMenu( "Network");
	menu->AddItem( CreateMenuItem( "Check Mail", BMM_CHECK_MAIL));
	menu->AddItem( accMenu);
	menu->AddItem( CreateMenuItem( "Check All Accounts", BMM_CHECK_ALL));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Send Pending Messages...", BMM_SEND_PENDING));
	mMainMenuBar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( CreateMenuItem( "New Message", BMM_NEW_MAIL));
	menu->AddSeparatorItem();
	mMailRefView->AddMailRefMenu( menu);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Toggle Header Mode", BMM_SWITCH_HEADER));
	menu->AddItem( CreateMenuItem( "Show Raw Message", BMM_SWITCH_RAW));
	mMainMenuBar->AddItem( menu);

	// Help
	menu = new BMenu( "Help");
	mMainMenuBar->AddItem( menu);

	return mMainMenuBar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::ArchiveState( BMessage* archive) const {
	status_t ret = inherited::ArchiveState( archive)
						|| archive->AddFloat( MSG_VSPLITTER, mVertSplitter->DividerLeftOrTop())
						|| archive->AddFloat( MSG_HSPLITTER, mHorzSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::UnarchiveState( BMessage* archive) {
	float vDividerPos, hDividerPos;
	status_t ret = inherited::UnarchiveState( archive)
						|| archive->FindFloat( MSG_VSPLITTER, &vDividerPos)
						|| archive->FindFloat( MSG_HSPLITTER, &hDividerPos);
	if (ret == B_OK) {
		mVertSplitter->SetPreferredDividerLeftOrTop( vDividerPos);
		mHorzSplitter->SetPreferredDividerLeftOrTop( hDividerPos);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::BeginLife() {
	nIsAlive = true;
	try {
		// set target for foreign handlers:
		mMainMenuBar->FindItem( BMM_NEW_MAILFOLDER)->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_RENAME_MAILFOLDER)->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_DELETE_MAILFOLDER)->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_RECACHE_MAILFOLDER)->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_SWITCH_RAW)->SetTarget( mMailView);
		mMainMenuBar->FindItem( BMM_SWITCH_RAW)->SetMarked( mMailView->ShowRaw());
		mMainMenuBar->FindItem( BMM_SWITCH_HEADER)->SetTarget( mMailView->HeaderView());
		// temporary deactivation:
		mMainMenuBar->FindItem( BMM_PAGE_SETUP)->SetEnabled( false);
		mMainMenuBar->FindItem( BMM_PRINT)->SetEnabled( false);
		mMainMenuBar->FindItem( BMM_FIND_MESSAGES)->SetEnabled( false);
		mMainMenuBar->FindItem( BMM_SEND_PENDING)->SetEnabled( false);

		mMailFolderView->StartWatching( this, BM_NTFY_MAILFOLDER_SELECTION);
		mMailRefView->StartWatching( this, BM_NTFY_MAILREF_SELECTION);
		mMailView->StartWatching( this, BM_NTFY_MAIL_VIEW);
		mMailFolderView->StartJob( TheMailFolderList.Get());
		mMainMenuBar->StartJob( ThePopAccountList.Get());
		BM_LOG2( BM_LogMainWindow, BString("MainWindow begins life"));
	} catch(...) {
		nIsAlive = false;
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::WorkspacesChanged( uint32 oldWorkspaces, uint32 newWorkspaces) {
	bmApp->SetNewWorkspace( newWorkspaces);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMainWindow::CreateMailFolderView( minimax minmax, int32 width, int32 height) {
	mMailFolderView = BmMailFolderView::CreateInstance( minmax, width, height);
	return mMailFolderView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMainWindow::CreateMailRefView( minimax minmax, int32 width, int32 height) {
	mMailRefView = BmMailRefView::CreateInstance( minmax, width, height);
	return mMailRefView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMainWindow::CreateMailView( minimax minmax, BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, false);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MessageReceived( BMessage* msg) {
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
			case BMM_NEW_MAIL: 
			case BMM_CHECK_MAIL:
			case BMM_CHECK_ALL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_MARK_AS:
			case BMM_TRASH:
			case BMM_REDIRECT:
			case BMM_REPLY:
			case BMM_REPLY_ALL:
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				mMailRefView->AddSelectedRefsToMsg( msg, BmApplication::MSG_MAILREF);
				be_app_messenger.SendMessage( msg);
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_MAILFOLDER_SELECTION: {
						MailFolderSelectionChanged( msg->FindInt32( BmMailFolderView::MSG_FOLDERS_SELECTED));
						break;
					}
					case BM_NTFY_MAILREF_SELECTION: {
						MailRefSelectionChanged( msg->FindInt32( BmMailRefView::MSG_MAILS_SELECTED));
						break;
					}
					case BM_NTFY_MAIL_VIEW: {
						MailViewChanged( msg->FindBool( BmMailView::MSG_HAS_MAIL));
						break;
					}
				}
				break;
			}
			case B_ABOUT_REQUESTED: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_PREFERENCES: {
				if (!ThePrefsWin) {
					BmPrefsWin::CreateInstance();
					ThePrefsWin->Show();
				} else  {
					ThePrefsWin->LockLooper();
					ThePrefsWin->Hide();
					ThePrefsWin->Show();
					ThePrefsWin->UnlockLooper();
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MainWindow: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMainWindow::QuitRequested() {
	BM_LOG2( BM_LogMainWindow, BString("MainWindow has been asked to quit"));
	if (bmApp->IsQuitting()) {
		// ask all other windows if they are ready to quit, in which case we
		// quit ourselves (only if ALL other windows will quit, too):
		int32 count = bmApp->CountWindows();
		for( int32 i=count-1; i>=0; --i) {
			BWindow* win = bmApp->WindowAt( i);
			if (win && win != this) {
				if (win->QuitRequested()) {
					win->LockLooper();
					win->Quit();
				} else {
					return false;
				}
			}
		}
		Hide();			// to hide a possible delay in WriteStateInfo() from the user
		return true;
	} else {
		bmApp->PostMessage( B_QUIT_REQUESTED);
		return false;
	}
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMainWindow::Quit() {
	mMailView->WriteStateInfo();
	mMailView->DetachModel();
	mMailRefView->DetachModel();
	mMailFolderView->DetachModel();
	BM_LOG2( BM_LogMainWindow, BString("MainWindow has quit"));
#ifdef BM_DEBUG_MEM
	(new BAlert( "", "End of MainWindow, check mem-usage!!!", "OK"))->Go();
#endif
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	MailFolderSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailFolderSelectionChanged( int32 numSelected) {
	// adjust menu:
	mMainMenuBar->FindItem( BMM_NEW_MAILFOLDER)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_RENAME_MAILFOLDER)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_DELETE_MAILFOLDER)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_RECACHE_MAILFOLDER)->SetEnabled( numSelected > 0);
}

/*------------------------------------------------------------------------------*\
	MailRefSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailRefSelectionChanged( int32 numSelected) {
	// adjust buttons:
	mReplyButton->SetEnabled( numSelected > 0);
	mReplyAllButton->SetEnabled( numSelected > 0);
	mForwardButton->SetEnabled( numSelected > 0);
	mRedirectButton->SetEnabled( numSelected > 0);
	mPrintButton->SetEnabled( 0 * numSelected > 0);
	mTrashButton->SetEnabled( numSelected > 0);
	// adjust menu:
	mMainMenuBar->FindItem( BMM_REPLY)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_REPLY_ALL)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_FORWARD_ATTACHED)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_FORWARD_INLINE)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_FORWARD_INLINE_ATTACH)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_REDIRECT)->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( "Mark Message As")->SetEnabled( numSelected > 0);
	mMainMenuBar->FindItem( BMM_FILTER)->SetEnabled( 0 * numSelected > 0);
	mMainMenuBar->FindItem( BMM_TRASH)->SetEnabled( numSelected > 0);
}

/*------------------------------------------------------------------------------*\
	MailViewChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailViewChanged( bool hasMail) {
	// adjust menu:
	mMainMenuBar->FindItem( BMM_FIND)->SetEnabled( false && hasMail);
	mMainMenuBar->FindItem( BMM_FIND_NEXT)->SetEnabled( false && hasMail);
	BMenuItem* item = mMainMenuBar->FindItem( BMM_SWITCH_RAW);
	item->SetEnabled( hasMail);
	item->SetMarked( mMailView->ShowRaw());
	item = mMainMenuBar->FindItem( BMM_SWITCH_HEADER);
	item->SetEnabled( hasMail);
}
