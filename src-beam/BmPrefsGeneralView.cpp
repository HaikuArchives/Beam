/*
	BmPrefsGeneralView.cpp
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

#include <Alert.h>
#include <Entry.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <Roster.h>

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BPopUpMenu;
	class BRect;
#endif
#include <layout-all.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmApp.h"
#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMenuControl.h"
#include "BmPrefs.h"
#include "BmPrefsGeneralView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsGeneralView
\********************************************************************************/

const char* const BmPrefsGeneralView::MSG_WORKSPACE = "wspace";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::BmPrefsGeneralView() 
	:	inherited( "General Options")
	,	mMailboxPanel( NULL)
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Status Window",
					new HGroup( 
						new VGroup(
							mDynamicStatusWinControl = new BmCheckControl( "Only show active jobs", 
																						  new BMessage(BM_DYNAMIC_STATUS_WIN_CHANGED), 
																						  this, ThePrefs->GetBool("DynamicStatusWin")),
							new Space( minimax(0,4,0,4)),
							mMailMoverShowControl = new BmTextControl( "Time before mail-moving-job will be shown (ms):", false, 5),
							mPopperRemoveControl = new BmTextControl( "Time before mail-receiving-job will be removed (ms):", false, 5),
							mSmtpRemoveControl = new BmTextControl( "Time before mail-sending-job will be removed (ms):", false, 5),
							mRemoveFailedControl = new BmTextControl( "Time before a failed job will be removed (ms):", false, 5),
							new Space( minimax(0,4,0,4)),
							0
						),
						new Space(),
						0
					)
				),
				new MBorder( M_LABELED_BORDER, 10, (char*)"General GUI Options",
					new HGroup(
						new VGroup(
							mWorkspaceControl = new BmMenuControl( "Workspace to start in:", new BPopUpMenu("")),
							new Space( minimax(0,10,0,10)),
							mRestoreFolderStatesControl = new BmCheckControl( "Restore mailfolder-view state on startup", 
																						 	  new BMessage(BM_RESTORE_FOLDER_STATES_CHANGED), 
																						 	  this, ThePrefs->GetBool("RestoreFolderStates")),
							mInOutAtTopControl = new BmCheckControl( "Show in & out - folders at top of mailfolder-view", 
																						 	  new BMessage(BM_INOUT_AT_TOP_CHANGED), 
																						 	  this, ThePrefs->GetBool("InOutAlwaysAtTop", false)),
							new Space( minimax(0,10,0,10)),
							mUseDeskbarControl = new BmCheckControl( "Show Deskbar Icon", 
																				  new BMessage(BM_USE_DESKBAR_CHANGED), 
																				  this, ThePrefs->GetBool("UseDeskbar", true)),
							mBeepNewMailControl = new BmCheckControl( "Beep when new mail has arrived", 
																				  new BMessage(BM_BEEP_NEW_MAIL_CHANGED), 
																				  this, ThePrefs->GetBool("BeepWhenNewMailArrived", true)),
							new Space( minimax(0,10,0,10)),
							mShowTooltipsControl = new BmCheckControl( "Show Tooltips for Toolbar-Buttons and Prefs", 
																				  new BMessage(BM_SHOW_TOOLTIPS_CHANGED), 
																				  this, ThePrefs->GetBool("ShowTooltips", true)),
							new Space(),
							0
						),
						new Space(),
						0
					)
				),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Performance & Timing Options",
					new HGroup( 
						new VGroup(
							mCacheRefsOnDiskControl = new BmCheckControl( "Cache mailfolders (on disk)", 
																				 		new BMessage(BM_CACHE_REFS_DISK_CHANGED), 
																				 		this, ThePrefs->GetBool("CacheRefsOnDisk")),
							mCacheRefsInMemControl = new BmCheckControl( "Keep mailfolders in memory once loaded", 
																				 		new BMessage(BM_CACHE_REFS_MEM_CHANGED), 
																				 		this, ThePrefs->GetBool("CacheRefsInMem")),
							mNetBufSizeSendControl = new BmTextControl( "Network buffer size when sending mail (bytes):", false, 5),
							mNetRecvTimeoutControl = new BmTextControl( "Timeout for network-connections (seconds):", false, 5),
							0
						),
						new Space(),
						0
					)
				),
				new Space(),
				0
			),
			new HGroup( 
				mMailboxButton = new MButton( MailboxButtonLabel().String(), new BMessage( BM_SELECT_MAILBOX), this, minimax(-1,-1,-1,-1)),
				new Space( minimax(10,0,10,0)),
				new MButton( "Make Beam preferred-app for email...", new BMessage( BM_MAKE_BEAM_STD_APP), this, minimax(-1,-1,-1,-1)),
				new Space(),
				0
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	float divider = mMailMoverShowControl->Divider();
	divider = MAX( divider, mPopperRemoveControl->Divider());
	divider = MAX( divider, mSmtpRemoveControl->Divider());
	divider = MAX( divider, mRemoveFailedControl->Divider());
	mMailMoverShowControl->SetDivider( divider);
	mPopperRemoveControl->SetDivider( divider);
	mSmtpRemoveControl->SetDivider( divider);
	mRemoveFailedControl->SetDivider( divider);

	divider = mNetBufSizeSendControl->Divider();
	divider = MAX( divider, mNetRecvTimeoutControl->Divider());
	mNetBufSizeSendControl->SetDivider( divider);
	mNetRecvTimeoutControl->SetDivider( divider);
	
	BmString val;
	val << ThePrefs->GetInt("MSecsBeforeMailMoverShows")/1000;
	mMailMoverShowControl->SetText( val.String());
	val = BmString("") << ThePrefs->GetInt("MSecsBeforePopperRemove")/1000;
	mPopperRemoveControl->SetText( val.String());
	val = BmString("") << ThePrefs->GetInt("MSecsBeforeSmtpRemove")/1000;
	mSmtpRemoveControl->SetText( val.String());
	val = BmString("") << ThePrefs->GetInt("MSecsBeforeRemoveFailed", 5000*1000)/1000;
	mRemoveFailedControl->SetText( val.String());
	val = BmString("") << ThePrefs->GetInt("NetSendBufferSize");
	mNetBufSizeSendControl->SetText( val.String());
	val = BmString("") << ThePrefs->GetInt("ReceiveTimeout");
	mNetRecvTimeoutControl->SetText( val.String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::~BmPrefsGeneralView() {
	delete mMailboxPanel;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmString BmPrefsGeneralView::MailboxButtonLabel() {
	BmString label( "Set mailbox path (currently '");
	label << ThePrefs->GetString( "MailboxPath") << "')...";
	return label;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::Initialize() {
	inherited::Initialize();
	
	TheBubbleHelper.SetHelp( mDynamicStatusWinControl, "Determines the layout of the job-status window.\n\tif checked, the job-status window will only display currently active jobs (it will grow/shrink accordingly)\n\tif unchecked, the job-status window will be static and show all known jobs, even if they are not active.");
	TheBubbleHelper.SetHelp( mMailMoverShowControl, "Here you can enter the time (in ms) Beam will let pass before it \nwill show the GUI for a mail-moving operation inside the job-window.");
	TheBubbleHelper.SetHelp( mPopperRemoveControl, "Here you can enter the time (in ms) Beam will let a finished POP3-job linger inside the job-window \n(this way you can check the results before the job is removed).");
	TheBubbleHelper.SetHelp( mSmtpRemoveControl, "Here you can enter the time (in ms) Beam will let a finished SMTP-job linger inside the job-window \n(this way you can check the results before the job is removed).");
	TheBubbleHelper.SetHelp( mRemoveFailedControl, "Here you can enter the time (in ms) Beam will keep any failed job inside the job-status window\n (this way you can see that a problem occurred before the job is removed).");
	TheBubbleHelper.SetHelp( mWorkspaceControl, "In this menu you can select the workspace that Beam should live in.");
	TheBubbleHelper.SetHelp( mRestoreFolderStatesControl, "Checking this makes Beam remember the state of the mailfolder-view \n(which of the folders are expanded/collapsed).\nIf unchecked, Beam will always start with a collapsed mailfolder-view.");
	TheBubbleHelper.SetHelp( mInOutAtTopControl, "Determines whether the in- and out-folder will be shown \nat the top of the mailfolder-list or if they \nwill be sorted in alphabetically.");
	TheBubbleHelper.SetHelp( mUseDeskbarControl, "Checking this makes Beam show an icon \nin the Deskbar.");
	TheBubbleHelper.SetHelp( mBeepNewMailControl, "Checking this makes Beam play the 'New E-mail' beep-event\nwhen new mail has arrived.\nYou can change the corresponding sound in the BeOS Sound-preferences.");
	TheBubbleHelper.SetHelp( mShowTooltipsControl, "Checking this makes Beam show a small \ninfo-window (just like this one) when the \nmouse-pointer lingers over a GUI-item.");
	TheBubbleHelper.SetHelp( mCacheRefsOnDiskControl, "Checking this will cause Beam to cache \nmail-folder contents on disk.\n\nDoing this speeds up the display \nof a mail-folder's contents quite a lot.");
	TheBubbleHelper.SetHelp( mCacheRefsInMemControl, "Checking this will cause Beam to keep \nany mail-folder's contents in memory even\nif the user selects another folder.\n\nThis gives best performance, but \nmay use *A LOT* of memory.");
	TheBubbleHelper.SetHelp( mNetBufSizeSendControl, "Here you can enter the network buffer size (in bytes)\nBeam will use for outgoing connections.\n\nIf sending seems slow, try a larger value in here.");
	TheBubbleHelper.SetHelp( mNetRecvTimeoutControl, "Here you can enter the time (in ms) Beam\n will wait for an answer from a remote network-server.");
	
	// add workspaces:
	int32 count = count_workspaces();
	for( int32 i=0; i<=count; ++i) {
		BMessage* msg = new BMessage( BM_WORKSPACE_SELECTED);
		msg->AddInt32( MSG_WORKSPACE, i);
		BmString label;
		if (i)
			label << i;
		else
			label = "Current";
		AddItemToMenu( mWorkspaceControl->Menu(), new BMenuItem( label.String(), msg), this);
	}
	mWorkspaceControl->MarkItem( ThePrefs->GetString("Workspace", "Current").String());

	mMailMoverShowControl->SetTarget( this);
	mPopperRemoveControl->SetTarget( this);
	mSmtpRemoveControl->SetTarget( this);
	mRemoveFailedControl->SetTarget( this);
	mNetBufSizeSendControl->SetTarget( this);
	mNetRecvTimeoutControl->SetTarget( this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::WriteStateInfo() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::SaveData() {
	ThePrefs->Store();
	bool deskbar = mUseDeskbarControl->Value();
	if (deskbar)
		bmApp->InstallDeskbarItem();
	else
		bmApp->RemoveDeskbarItem();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::UndoChanges() {
	bool lastInOutAtTop = ThePrefs->GetBool( "InOutAlwaysAtTop");
	ThePrefs->ResetToSaved();
	if (lastInOutAtTop != ThePrefs->GetBool( "InOutAlwaysAtTop")) {
		if (TheMailFolderView->LockLooper()) {
			TheMailFolderView->SortItems();
			TheMailFolderView->UnlockLooper();
		}
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_TEXTFIELD_MODIFIED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
				if ( source == mMailMoverShowControl)
					ThePrefs->SetInt("MSecsBeforeMailMoverShows", 1000*atoi(mMailMoverShowControl->Text()));
				else if ( source == mPopperRemoveControl)
					ThePrefs->SetInt("MSecsBeforePopperRemove", 1000*atoi(mPopperRemoveControl->Text()));
				else if ( source == mSmtpRemoveControl)
					ThePrefs->SetInt("MSecsBeforeSmtpRemove", 1000*atoi(mSmtpRemoveControl->Text()));
				else if ( source == mRemoveFailedControl)
					ThePrefs->SetInt("MSecsBeforeRemoveFailed", 1000*atoi(mRemoveFailedControl->Text()));
				else if ( source == mNetBufSizeSendControl)
					ThePrefs->SetInt("NetSendBufferSize", atoi(mNetBufSizeSendControl->Text()));
				else if ( source == mNetRecvTimeoutControl)
					ThePrefs->SetInt("ReceiveTimeout", atoi(mNetRecvTimeoutControl->Text()));
				break;
			}
			case BM_RESTORE_FOLDER_STATES_CHANGED: {
				ThePrefs->SetBool("RestoreFolderStates", mRestoreFolderStatesControl->Value());
				break;
			}
			case BM_DYNAMIC_STATUS_WIN_CHANGED: {
				ThePrefs->SetBool("DynamicStatusWin", mDynamicStatusWinControl->Value());
				break;
			}
			case BM_CACHE_REFS_DISK_CHANGED: {
				ThePrefs->SetBool("CacheRefsOnDisk", mCacheRefsOnDiskControl->Value());
				break;
			}
			case BM_CACHE_REFS_MEM_CHANGED: {
				ThePrefs->SetBool("CacheRefsInMem", mCacheRefsInMemControl->Value());
				break;
			}
			case BM_INOUT_AT_TOP_CHANGED: {
				ThePrefs->SetBool("InOutAlwaysAtTop", mInOutAtTopControl->Value());
				TheMailFolderView->LockLooper();
				TheMailFolderView->SortItems();
				TheMailFolderView->UnlockLooper();
				break;
			}
			case BM_USE_DESKBAR_CHANGED: {
				bool val = mUseDeskbarControl->Value();
				ThePrefs->SetBool("UseDeskbar", val);
				break;
			}
			case BM_BEEP_NEW_MAIL_CHANGED: {
				ThePrefs->SetBool("BeepWhenNewMailArrived", mBeepNewMailControl->Value());
				break;
			}
			case BM_SHOW_TOOLTIPS_CHANGED: {
				ThePrefs->SetBool("ShowTooltips", mShowTooltipsControl->Value());
				TheBubbleHelper.EnableHelp( mShowTooltipsControl->Value());
				break;
			}
			case BM_WORKSPACE_SELECTED: {
				BMenuItem* item = mWorkspaceControl->Menu()->FindMarked();
				if (item)
					ThePrefs->SetString( "Workspace", item->Label());
				break;
			}
			case BM_MAKE_BEAM_STD_APP: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Set Beam As Preferred App", 
														 "This will make Beam the preferred Application for the following mimetypes:\n\n\tEmail (text/x-email)\n\tInternet-messages (message/rfc822).",
													 	 "Ok, do it", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_MAKE_BEAM_STD_APP), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						app_info appInfo;
						if (be_app->GetAppInfo( &appInfo) != B_OK)
							break;
						BMimeType mt("text/x-email");
						if (mt.InitCheck() == B_OK)
							mt.SetPreferredApp( appInfo.signature);
						mt.SetTo("message/rfc822");
						if (mt.InitCheck() == B_OK)
							mt.SetPreferredApp( appInfo.signature);
						BAlert* alert = new BAlert( "Set Beam As Preferred App", 
															 "Done, Beam is now the preferred Application for email.",
														 	 "Good", NULL, NULL, B_WIDTH_AS_USUAL,
														 	 B_INFO_ALERT);
						alert->Go();
					}
				}
				break;
			}
			case BM_SELECT_MAILBOX: {
				entry_ref mailboxRef;
				if (msg->FindRef( "refs", 0, &mailboxRef) != B_OK) {
					// first step, let user select new mailbox:
					if (!mMailboxPanel) {
						mMailboxPanel = new BFilePanel( B_OPEN_PANEL, new BMessenger(this), NULL,
																  B_DIRECTORY_NODE, false, msg);
					}
					mMailboxPanel->Show();
				} else {
					// second step, set mailbox accordingly:
					BEntry entry( &mailboxRef);
					BPath path;
					if (entry.GetPath( &path) == B_OK) {
						ThePrefs->SetString( "MailboxPath", path.Path());
						mMailboxButton->SetLabel( MailboxButtonLabel().String());
						TheMailFolderList->MailboxPathHasChanged( true);
						BAlert* alert = new BAlert( "Mailbox Path", 
															 "Done, Beam will use the new mailbox after a restart",
														 	 "Ok", NULL, NULL, B_WIDTH_AS_USUAL,
														 	 B_INFO_ALERT);
						alert->Go();
					}
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}
