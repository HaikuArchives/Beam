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
#include "BmString.h"

#include <layout-all.h>

#include "UserResizeSplitView.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmLogWindow.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMainWindow.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRosterBase.h"
#include "BmSmtpAccount.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"

BmMainWindow* BmMainWindow::theInstance = NULL;

const char* const BmMainWindow::MSG_VSPLITTER = "bm:vspl";
const char* const BmMainWindow::MSG_HSPLITTER = "bm:hspl";

/*------------------------------------------------------------------------------*\
	flag and access-function that indicate a user's request-to-stop:
\*------------------------------------------------------------------------------*/
bool BmMainWindow::nIsAlive = false;
bool BmMainWindow::IsAlive() {
	return BmMainWindow::nIsAlive;
}

void UpdateAccounts( BmToolbarButton* button)
{
	BmAutolockCheckGlobal lock( ThePopAccountList->ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "UpdateAccounts: Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( 	iter = ThePopAccountList->begin(); 
			iter != ThePopAccountList->end(); ++iter) {
		BMessage* msg = new BMessage( BMM_CHECK_MAIL);
		msg->AddString( BmPopAccountList::MSG_ITEMKEY, 
							 iter->second->Key().String());
		button->AddActionVariation( iter->second->DisplayKey(), msg);
	}
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
		BmString wspc = ThePrefs->GetString( "Workspace", "Current");
		TheMainWindow->SetWorkspaces( wspc=="Current" 
													? B_CURRENT_WORKSPACE 
													: 1<<(atoi( wspc.String())-1));
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
					  beamApp->BmAppName.String(),
					  ThePrefs->GetBool( "UseDocumentResizer", true)
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, 
					  B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
	,	mMailFolderView( NULL)
	,	mMailRefView( NULL)
	,	mVertSplitter( NULL)
	,	mErrLogWin( NULL)
{
	// Get maximum button size
	float width=0, height=0;
	BmToolbarButton::CalcMaxSize(width, height, "Check", true);
	BmToolbarButton::CalcMaxSize(width, height, "New");
	BmToolbarButton::CalcMaxSize(width, height, "Reply", true);
	BmToolbarButton::CalcMaxSize(width, height, "Forward", true);
	BmToolbarButton::CalcMaxSize(width, height, "Print");
	BmToolbarButton::CalcMaxSize(width, height, "Trash");

	int32 defaultFwdMsgType = 
		ThePrefs->GetString( "DefaultForwardType")=="Inline"
			? BMM_FORWARD_INLINE
			: BMM_FORWARD_ATTACHED;

	MView* mOuterGroup = 
		new VGroup(
			minimax( 600, 200, 1E5, 1E5),
			CreateMenu(),
			new BmToolbar(
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mCheckButton 
						= new BmToolbarButton( 
							"Check", 
							width, height,
							new BMessage(BMM_CHECK_MAIL), this, 
							"Check for new mail", true
						),
					mNewButton 
						= new BmToolbarButton( 
							"New", 
							width, height,
							new BMessage(BMM_NEW_MAIL), this, 
							"Compose a new mail message"
						),
					mReplyButton 
						= new BmToolbarButton( 
							"Reply", 
							width, height,
							new BMessage(BMM_REPLY), this, 
							"Reply to person or list", true
						),
					mForwardButton 
						= new BmToolbarButton( 
							"Forward", 
						   width, height,
						   new BMessage( defaultFwdMsgType), this, 
						   "Forward mail to somewhere else", true
						),
					mPrintButton 
						= new BmToolbarButton( 
							"Print", 
							width, height,
							new BMessage(BMM_PRINT), this,
							"Print selected messages(s)"
						),
					mTrashButton 
						= new BmToolbarButton( 
							"Trash", 
							width, height,
							new BMessage(BMM_TRASH), this, 
							"Move selected messages to Trash"
						),
					new BmToolbarSpace(),
					0
				)
			),
			new HGroup(
				mVertSplitter = new UserResizeSplitView( 
					new BetterScrollView( 
						minimax(50,100,1E5,1E5), 
						mMailFolderView = BmMailFolderView::CreateInstance( 200, 400),
						BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
						| BM_SV_BUSYVIEW | BM_SV_CAPTION,
						"999 folders"
					),
					mHorzSplitter = new UserResizeSplitView( 
						new BetterScrollView( 
							minimax(300,50,1E5,1E5), 
							mMailRefView = BmMailRefView::CreateInstance( 400, 200),
							BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
							| BM_SV_BUSYVIEW | BM_SV_CAPTION,
							"99999 messages"
						),
						new BmMailViewContainer(
							minimax(300,80,1E5,1E5), 
							mMailView = BmMailView::CreateInstance( BRect(0,0,400,200), 
																				 false)
						),
						"hsplitter", 200, B_HORIZONTAL, true, true, true, true, 
						false, B_FOLLOW_NONE
					),
					"vsplitter", 220, B_VERTICAL, true, true, true, false, false, 
					B_FOLLOW_NONE
				),
				0
			),
			0
		);

	mCheckButton->SetUpdateVariationsFunc( UpdateAccounts);
	mReplyButton->AddActionVariation( "Reply", new BMessage(BMM_REPLY));
	mReplyButton->AddActionVariation( "Reply To List", 
												 new BMessage(BMM_REPLY_LIST));
	mReplyButton->AddActionVariation( "Reply To Person", 
												 new BMessage(BMM_REPLY_ORIGINATOR));
	mReplyButton->AddActionVariation( "Reply To All", 
												 new BMessage(BMM_REPLY_ALL));
	mForwardButton->AddActionVariation( "Forward As Attachment", 
													new BMessage(BMM_FORWARD_ATTACHED));
	mForwardButton->AddActionVariation( "Forward Inline", 
													new BMessage(BMM_FORWARD_INLINE));
	mForwardButton->AddActionVariation( "Forward Inline (With Attachments)", 
													new BMessage(BMM_FORWARD_INLINE_ATTACH));

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
	mMainMenuBar = new MMenuBar();
	BMenu* menu = NULL;
	BmMenuController* subMenu;
	// File
	menu = new BMenu( "File");
	menu->AddItem( CreateMenuItem( "New Folder...", BMM_NEW_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Rename Folder...", BMM_RENAME_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Delete Folder", BMM_DELETE_MAILFOLDER));
	menu->AddItem( CreateMenuItem( "Recache Folder", BMM_RECACHE_MAILFOLDER));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Page Setup...", BMM_PAGE_SETUP));
	menu->AddItem( CreateMenuItem( "Print Message(s)...", BMM_PRINT, 
											 "Print Message..."));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, 
						CreateMenuItem( "Preferences...", BMM_PREFERENCES), 
						beamApp);
	menu->AddSeparatorItem();
	menu->AddItem( 
		new BmMenuController( "Show Log", this, 
									 new BMessage( BMM_SHOW_LOGFILE), 
									 &BmGuiRosterBase::RebuildLogMenu, BM_MC_MOVE_RIGHT)
	);
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
	menu->AddItem( CreateMenuItem( "Find", BMM_FIND));
//	menu->AddItem( CreateMenuItem( "Find Messages", BMM_FIND_MESSAGES));
	menu->AddItem( CreateMenuItem( "Find Next", BMM_FIND_NEXT));
	mMainMenuBar->AddItem( menu);

	// Network
	menu = new BMenu( "Network");
	menu->AddItem( CreateMenuItem( "Check Mail", BMM_CHECK_MAIL));
	mAccountMenu = new BmMenuController( 
		"Check Mail For", this,
		new BMessage( BMM_CHECK_MAIL), 
		&BmGuiRosterBase::RebuildPopAccountMenu,
		BM_MC_MOVE_RIGHT
	);
	menu->AddItem( mAccountMenu);
	menu->AddItem( CreateMenuItem( "Check All Accounts", BMM_CHECK_ALL));
	menu->AddSeparatorItem();
	subMenu = new BmMenuController( 
		"Send Pending Messages For", this,
		new BMessage( BMM_SEND_PENDING), 
		&BmGuiRosterBase::RebuildSmtpAccountMenu,
		BM_MC_MOVE_RIGHT
	);
	menu->AddItem( subMenu);
	menu->AddItem( CreateMenuItem( "Send All Pending Messages...",
											 BMM_SEND_PENDING));
	mMainMenuBar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( CreateMenuItem( "New Message...", BMM_NEW_MAIL));
	menu->AddSeparatorItem();
	mMailRefView->AddMailRefMenu( menu, this, false);
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
						|| archive->AddFloat( MSG_VSPLITTER, 
													 mVertSplitter->DividerLeftOrTop())
						|| archive->AddFloat( MSG_HSPLITTER, 
													 mHorzSplitter->DividerLeftOrTop());
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
		mMainMenuBar->FindItem( BMM_RENAME_MAILFOLDER)
			->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_DELETE_MAILFOLDER)
			->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_RECACHE_MAILFOLDER)
			->SetTarget( mMailFolderView);
		mMainMenuBar->FindItem( BMM_PAGE_SETUP)->SetTarget( beamApp);
		mMainMenuBar->FindItem( BMM_SWITCH_RAW)->SetTarget( mMailView);
		mMainMenuBar->FindItem( BMM_SWITCH_RAW)->SetMarked( mMailView->ShowRaw());
		mMainMenuBar->FindItem( BMM_SWITCH_HEADER)
			->SetTarget( (BHandler*)mMailView->HeaderView());

		// populate pop-account menu in order to activate shortcuts:
		while( !ThePopAccountList->IsJobCompleted())
			snooze( 200*1000);
		mAccountMenu->Shortcuts( "1234567890");

		// create and hide error-log
		mErrLogWin 
			= BmLogWindow::CreateAndStartInstanceFor( "Errors", true, true);

		mMailFolderView->StartWatching( this, BM_NTFY_MAILFOLDER_SELECTION);
		mMailRefView->StartWatching( this, BM_NTFY_MAILREF_SELECTION);
		mMailView->StartWatching( this, BM_NTFY_MAIL_VIEW);
		mMailFolderView->StartJob( TheMailFolderList.Get());
		BM_LOG2( BM_LogGui, BmString("MainWindow begins life"));
	} catch(...) {
		nIsAlive = false;
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::WorkspacesChanged( uint32, uint32 newWorkspaces) {
	beamApp->SetNewWorkspace( newWorkspaces);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MessageReceived( BMessage* msg) {
	try {
		static int32 jobNum=1;
		switch( msg->what) {
			case BMM_NEW_MAIL: 
			case BMM_CHECK_MAIL:
			case BMM_CHECK_ALL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_TRASH: {
				mMailRefView->TrashSelectedMessages();
				break;
			}
			case BMM_MOVE:
			case BMM_MARK_AS:
			case BMM_PRINT:
			case BMM_REDIRECT:
			case BMM_REPLY:
			case BMM_REPLY_LIST:
			case BMM_REPLY_ORIGINATOR:
			case BMM_REPLY_ALL:
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: 
			case BMM_EDIT_AS_NEW: {
				mMailRefView->AddSelectedRefsToMsg( msg);
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BMM_FILTER: {
				BMessage tmpMsg( BM_JOBWIN_FILTER);
				mMailRefView->AddSelectedRefsToMsg( &tmpMsg);
				BmString jobName = msg->FindString( BmListModel::MSG_ITEMKEY);
				if (jobName.Length()) {
					tmpMsg.AddString( BmListModel::MSG_ITEMKEY, jobName.String());
					jobName << jobNum++;
				} else
					jobName << "Filter_" << jobNum++;
				tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
				TheJobStatusWin->PostMessage( &tmpMsg);
				break;
			}
			case BMM_LEARN_AS_SPAM: 
			case BMM_LEARN_AS_TOFU: {
				BMessage tmpMsg( BM_JOBWIN_FILTER);
				mMailRefView->AddSelectedRefsToMsg( &tmpMsg);
				BmString jobName 
					= (msg->what == BMM_LEARN_AS_SPAM)
							? BmFilterList::LEARN_AS_SPAM_NAME 
							: BmFilterList::LEARN_AS_TOFU_NAME;
				tmpMsg.AddString( BmListModel::MSG_ITEMKEY, jobName.String());
				jobName << jobNum++;
				tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
				TheJobStatusWin->PostMessage( &tmpMsg);
				break;
			}
			case BMM_CREATE_FILTER: {
				mMailRefView->AddSelectedRefsToMsg( msg);
				BMessage appMsg( BMM_PREFERENCES);
				appMsg.AddString( "SubViewName", "Filters");
				appMsg.AddMessage( "SubViewMsg", msg);
				be_app_messenger.SendMessage( &appMsg);
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_MAILFOLDER_SELECTION: {
						MailFolderSelectionChanged( 
							msg->FindBool( 
								BmMailFolderView::MSG_HAVE_SELECTED_FOLDER
							)
						);
						break;
					}
					case BM_NTFY_MAILREF_SELECTION: {
						MailRefSelectionChanged( 
							msg->FindBool( BmMailRefView::MSG_MAILS_SELECTED));
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
			case BMM_SHOW_LOGFILE: {
				BmString logName = msg->FindString( "logfile");
				BmLogWindow::CreateAndStartInstanceFor( logName.String());
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
			case BMM_FIND:
			case BMM_FIND_NEXT: {
				PostMessage( msg, mMailView);
				break;
			}
			case BMM_SEND_PENDING: {
				BmString key = msg->FindString( BmSmtpAccountList::MSG_ITEMKEY);
				if (key.Length())
					TheSmtpAccountList->SendPendingMailsFor( key);
				else
					TheSmtpAccountList->SendPendingMails();
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MainWindow: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMainWindow::QuitRequested() {
	BM_LOG2( BM_LogGui, BmString("MainWindow has been asked to quit"));
	if (beamApp->IsQuitting()) {
		Hide();			// to hide a possible delay in WriteStateInfo()
							// from the user
		return true;
	} else {
		beamApp->PostMessage( B_QUIT_REQUESTED);
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
	BM_LOG2( BM_LogGui, BmString("MainWindow has quit"));
#ifdef BM_DEBUG_MEM
	(new BAlert( "", "End of MainWindow, check mem-usage!!!", "OK"))->Go();
#endif
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	Minimize()
		-	standard BeOS-behaviour, but we minimize our error-window, too.
\*------------------------------------------------------------------------------*/
void BmMainWindow::Minimize( bool minimize) {
	inherited::Minimize( minimize);
	if (minimize && mErrLogWin)
		mErrLogWin->Minimize( true);
}

/*------------------------------------------------------------------------------*\
	MailFolderSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailFolderSelectionChanged( bool haveSelectedFolder) {
	// adjust menu:
	mMainMenuBar->FindItem( BMM_NEW_MAILFOLDER)
		->SetEnabled( haveSelectedFolder);
	mMainMenuBar->FindItem( BMM_RENAME_MAILFOLDER)
		->SetEnabled( haveSelectedFolder);
	mMainMenuBar->FindItem( BMM_DELETE_MAILFOLDER)
		->SetEnabled( haveSelectedFolder);
	mMainMenuBar->FindItem( BMM_RECACHE_MAILFOLDER)
		->SetEnabled( haveSelectedFolder);
}

/*------------------------------------------------------------------------------*\
	MailRefSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailRefSelectionChanged( bool haveSelectedRef) {
	// adjust buttons:
	mReplyButton->SetEnabled( haveSelectedRef);
	mForwardButton->SetEnabled( haveSelectedRef);
	mTrashButton->SetEnabled( haveSelectedRef);
	mPrintButton->SetEnabled( haveSelectedRef);
	// adjust menu:
	mMainMenuBar->FindItem( "Reply")
			->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( "Forward")
			->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_REDIRECT)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_EDIT_AS_NEW)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BmMailRefView::MENU_MARK_AS)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BmMailRefView::MENU_MOVE)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_FILTER)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_CREATE_FILTER)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BmMailRefView::MENU_FILTER)
		->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_PRINT)->SetEnabled( haveSelectedRef);
	mMainMenuBar->FindItem( BMM_TRASH)->SetEnabled( haveSelectedRef);
}

/*------------------------------------------------------------------------------*\
	MailViewChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailViewChanged( bool hasMail) {
	// adjust menu:
	mMainMenuBar->FindItem( BMM_FIND)->SetEnabled( hasMail);
	mMainMenuBar->FindItem( BMM_FIND_NEXT)->SetEnabled( hasMail);
	BMenuItem* item = mMainMenuBar->FindItem( BMM_SWITCH_RAW);
	item->SetEnabled( hasMail);
	item->SetMarked( mMailView->ShowRaw());
	item = mMainMenuBar->FindItem( BMM_SWITCH_HEADER);
	item->SetEnabled( hasMail);
}
