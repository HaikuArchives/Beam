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
#include <Roster.h>

#include <layout-all.h>

#include "Colors.h"

#include "BmApp.h"
#include "BmCheckControl.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderView.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmPrefsGeneralView.h"
#include "BmTextControl.h"
#include "BmUtil.h"



/********************************************************************************\
	BmPrefsGeneralView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::BmPrefsGeneralView() 
	:	inherited( "General Options")
{
	MView* view = 
		new VGroup(
			new Space( minimax(0,10,0,10)),
			new MBorder( M_LABELED_BORDER, 10, (char*)"Mail-Listview Layout",
				new VGroup(
					CreateMailRefLayoutView( minimax(500,80,1E5,80), 500, 80),
					new Space( minimax(0,4,0,4)),
					mStripedListViewControl = new BmCheckControl( "Use striped version of listview (needs restart)", 
																				 new BMessage(BM_STRIPED_LISTVIEW_CHANGED), 
																				 this, ThePrefs->GetBool("StripedListView")),
					new Space( minimax(0,4,0,4)),
					0
				)
			),
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
						mRestoreFolderStatesControl = new BmCheckControl( "Restore mailfolder-view state on startup", 
																					 	  new BMessage(BM_RESTORE_FOLDER_STATES_CHANGED), 
																					 	  this, ThePrefs->GetBool("RestoreFolderStates")),
						mInOutAtTopControl = new BmCheckControl( "Show in & out - folders at top of mailfolder-view", 
																					 	  new BMessage(BM_INOUT_AT_TOP_CHANGED), 
																					 	  this, ThePrefs->GetBool("InOutAlwaysAtTop", false)),
						new Space(),
						0
						),
						0
					)
				),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup( 
				new MBorder( M_LABELED_BORDER, 10, (char*)"Performance & Timing Options",
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
					)
				),
				new Space(),
				0
			),
			new HGroup( 
				new MButton( "Make Beam preferred-app for email...", new BMessage( BM_MAKE_BEAM_STD_APP), this, minimax(-1,-1,-1,-1)),
				new Space(),
				0
			),
			new Space(),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	mLayoutView->AvoidInvoke( true);
	
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
	
	BString val;
	val << ThePrefs->GetInt("MSecsBeforeMailMoverShows")/1000;
	mMailMoverShowControl->SetText( val.String());
	val = BString("") << ThePrefs->GetInt("MSecsBeforePopperRemove")/1000;
	mPopperRemoveControl->SetText( val.String());
	val = BString("") << ThePrefs->GetInt("MSecsBeforeSmtpRemove")/1000;
	mSmtpRemoveControl->SetText( val.String());
	val = BString("") << ThePrefs->GetInt("MSecsBeforeRemoveFailed", 5000*1000)/1000;
	mRemoveFailedControl->SetText( val.String());
	val = BString("") << ThePrefs->GetInt("NetSendBufferSize");
	mNetBufSizeSendControl->SetText( val.String());
	val = BString("") << ThePrefs->GetInt("ReceiveTimeout");
	mNetRecvTimeoutControl->SetText( val.String());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsGeneralView::~BmPrefsGeneralView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::Initialize() {
	inherited::Initialize();

	mFolder = BmMailFolder::CreateDummyInstance();
	mRefList = new BmMailRefList( mFolder.Get());
	BmRef<BmMailRef> mRef( BmMailRef::CreateDummyInstance( mRefList.Get(), 0));
	mRefList->AddItemToList( mRef.Get());
	mRef = BmMailRef::CreateDummyInstance( mRefList.Get(), 1);
	mRefList->AddItemToList( mRef.Get());
	mLayoutView->StartJob( mRefList.Get());

	mMailMoverShowControl->SetTarget( this);
	mPopperRemoveControl->SetTarget( this);
	mSmtpRemoveControl->SetTarget( this);
	mRemoveFailedControl->SetTarget( this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::WriteStateInfo() {
//	mLayoutView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::SaveData() {
	BMessage* prefsMsg = ThePrefs->PrefsMsg();
	BMessage layoutMsg;
	if (mLayoutView->Archive( &layoutMsg, false) == B_OK)
		prefsMsg->ReplaceMessage("MailRefLayout", &layoutMsg);
	ThePrefs->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsGeneralView::UndoChanges() {
	ThePrefs = NULL;
	BmPrefs::CreateInstance();
	mLayoutView->Unarchive( ThePrefs->GetMsg( "MailRefLayout"));
	TheMailFolderView->LockLooper();
	TheMailFolderView->SortItems();
	TheMailFolderView->UnlockLooper();
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
			case BM_STRIPED_LISTVIEW_CHANGED: {
				ThePrefs->SetBool("StripedListView", mStripedListViewControl->Value());
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
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsGeneralView::CreateMailRefLayoutView( minimax minmax, int32 width, int32 height) {
	mLayoutView = BmMailRefView::CreateInstance( minmax, width, height);
	return mLayoutView->ContainerView();
}
