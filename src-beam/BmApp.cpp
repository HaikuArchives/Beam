/*
	BmApp.cpp
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
#include <AppFileInfo.h>
#include <Beep.h>
#include <Deskbar.h>
#include <MenuItem.h>
#include <Roster.h>
#include <Screen.h>

#include <PictureButton.h>

#include "regexx.hh"
using namespace regexx;

#include "BubbleHelper.h"
#include "ImageAboutWindow.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmDataModel.h"
#include "BmDeskbarView.h"
#include "BmEncoding.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMailMover.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMailViewWin.h"
#include "BmMainWindow.h"
#include "BmMsgTypes.h"
#include "BmNetUtil.h"
#include "BmPeople.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmPrefsWin.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

static const char* BM_BEEP_EVENT = "New E-mail";

const char* BM_APP_SIG = "application/x-vnd.zooey-beam";
const char* BM_TEST_APP_SIG = "application/x-vnd.zooey-testbeam";
const char* const BM_DeskbarItemName = "Beam_DeskbarItem";

const char* const BmApplication::MSG_MAILREF = 		"bm:mref";
const char* const BmApplication::MSG_STATUS = 		"bm:status";
const char* const BmApplication::MSG_WHO_TO = 		"bm:to";
const char* const BmApplication::MSG_OPT_FIELD = 	"bm:optf";
const char* const BmApplication::MSG_OPT_VALUE = 	"bm:optv";
const char* const BmApplication::MSG_SUBJECT = 		"bm:subj";
const char* const BmApplication::MSG_SELECTED_TEXT = 	"bm:seltext";
const char* const BmApplication::MSG_SENDING_REFVIEW = 	"bm:srefv";

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig)
	:	inherited( sig)
	,	mIsQuitting( false)
	,	mInitCheck( B_NO_INIT)
	,	mMailWin( NULL)
	,	mPrintSetup( NULL)
	,	mPrintJob( "Mail")
	,	mStartupLocker( new BLocker( "StartupLocker", false))
	,	mDeskbarItemIsOurs( false)
{
	if (InstanceCount > 0)
		throw BM_runtime_error( "Trying to initialize more than one instance "
										"of class Beam");

	if (BM_TEST_APP_SIG == sig)
		// set global flag, if in test-mode:
		BeamInTestMode = true;

	// find out if we are running on Dano (or newer):
	system_info sysInfo;
	get_system_info( &sysInfo);
	BmString kTimestamp(sysInfo.kernel_build_date);
	kTimestamp << " " << sysInfo.kernel_build_time;
	time_t kTime;
	ParseDateTime( kTimestamp, kTime);
	if (kTime >= 1005829579)
		BeamOnDano = true;

	bmApp = this;
	
	mStartupLocker->Lock();

	try {
		BmAppName = bmApp->Name();
		// set version info:
		app_info appInfo;
		BFile appFile;
		version_info vInfo;
		BAppFileInfo appFileInfo;
		bmApp->GetAppInfo( &appInfo); 
		appFile.SetTo( &appInfo.ref, B_READ_ONLY);
		appFileInfo.SetTo( &appFile);
		if (appFileInfo.GetVersionInfo( &vInfo, B_APP_VERSION_KIND) == B_OK) {
			BmAppVersion = vInfo.short_info;
		}
		BmAppNameWithVersion = BmAppName + " " + BmAppVersion;
		// note if we are running a devel-version:
		if (BmAppVersion.IFindFirst( "devel") >= 0)
			BeamInDevelMode = true;
		// store app-path for later use:
		node_ref nref;
		nref.device = appInfo.ref.device;
		nref.node = appInfo.ref.directory;
		BDirectory appDir( &nref);
		BEntry appDirEntry;
		appDir.GetEntry( &appDirEntry);
		BPath appPath;
		appDirEntry.GetPath( &appPath);
		mAppPath = appPath.Path();

		// create the log-handler:
		BmLogHandler::CreateInstance( 1, &nref);

		// load/determine all needed resources:
		BmResources::CreateInstance();
		time_t appModTime;
		appFile.GetModificationTime( &appModTime);
		TheResources->CheckMimeTypeFile( sig, appModTime);

		// create BubbleHelper:
		BubbleHelper::CreateInstance();

		// load the preferences set by user (if any):
		BmPrefs::CreateInstance();
		BM_LOG( BM_LogApp, BmString("App-initialization started..."));

		ColumnListView::SetExtendedSelectionPolicy( 
									ThePrefs->GetBool( "ListviewLikeTracker", false));
							// make sure this actually get's initialized...

		// init charset-tables:
		BmEncoding::InitCharsetMap();

		// create the node-monitor looper:
		BmMailMonitor::CreateInstance();

		// create the job status window:
		BmJobStatusWin::CreateInstance();
		TheJobStatusWin->Hide();
		TheJobStatusWin->Show();

		// create most of our list-models:
		BmSignatureList::CreateInstance();

		BmFilterList::CreateInstance();

		BmFilterChainList::CreateInstance();

		BmPeopleMonitor::CreateInstance();
		BmPeopleList::CreateInstance();

		BmMailFolderList::CreateInstance();

		BmIdentityList::CreateInstance();

		BmSmtpAccountList::CreateInstance( TheJobStatusWin);

		BmPopAccountList::CreateInstance( TheJobStatusWin);

		BM_LOG( BM_LogApp, BmString("...setting up foreign-keys..."));
		// now setup all foreign-key connections between these list-models:
		ThePopAccountList->AddForeignKey( BmIdentity::MSG_POP_ACCOUNT, 
													 TheIdentityList.Get());
		TheSmtpAccountList->AddForeignKey( BmIdentity::MSG_SMTP_ACCOUNT, 
													  TheIdentityList.Get());
		TheSignatureList->AddForeignKey( BmIdentity::MSG_SIGNATURE_NAME,
													TheIdentityList.Get());
		TheFilterChainList->AddForeignKey( BmPopAccount::MSG_FILTER_CHAIN,
													  ThePopAccountList.Get());
		ThePopAccountList->AddForeignKey( BmSmtpAccount::MSG_ACC_FOR_SAP,
													  TheSmtpAccountList.Get());
		TheMailFolderList->AddForeignKey( BmPopAccount::MSG_HOME_FOLDER,
													 ThePopAccountList.Get());
		TheMailFolderList->AddForeignKey( BmFilterAddon::FK_FOLDER,
													 TheFilterList.Get());
		TheIdentityList->AddForeignKey( BmFilterAddon::FK_IDENTITY,
												  TheFilterList.Get());

		add_system_beep_event( BM_BEEP_EVENT);

		BM_LOG( BM_LogApp, BmString("...creating main-window..."));
		BmMainWindow::CreateInstance();
		
		TheBubbleHelper->EnableHelp( ThePrefs->GetBool( "ShowTooltips", true));

		mInitCheck = B_OK;
		InstanceCount++;
		mStartupLocker->Unlock();
		BM_LOG( BM_LogApp, BmString("App-initialization done."));
	} catch (BM_error& err) {
		BM_SHOWERR( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BmApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmApplication::~BmApplication() {
	RemoveDeskbarItem();
	ThePeopleList = NULL;
	TheMailFolderList = NULL;
	TheIdentityList = NULL;
	ThePopAccountList = NULL;
	TheSmtpAccountList = NULL;
	TheSignatureList = NULL;
	TheFilterChainList = NULL;
	TheFilterList = NULL;
#ifdef BM_REF_DEBUGGING
	BmRefObj::PrintRefsLeft();
#endif
//	TheLogHandler->ShutDown();
	delete mPrintSetup;
	delete ThePrefs;
	delete TheResources;
	delete TheLogHandler;
	delete mStartupLocker;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	ReadyToRun()
		-	ensures that main-window is visible
		-	if Beam has been instructed to show a specific mail on startup, the
			mail-window is shown on top of the main-window
\*------------------------------------------------------------------------------*/
void BmApplication::ReadyToRun() {
	if (TheMainWindow->IsMinimized())
		TheMainWindow->Minimize( false);
	if (mMailWin) {
		TheMainWindow->SendBehind( mMailWin);
		mMailWin = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	AppActivated()
		-	shows Beam's main-window
\*------------------------------------------------------------------------------*/
void BmApplication::AppActivated( bool active) {
	if (active && TheMainWindow->IsMinimized())
		TheMainWindow->Minimize( false);
}

/*------------------------------------------------------------------------------*\
	Run()
		-	starts Beam
\*------------------------------------------------------------------------------*/
thread_id BmApplication::Run() {
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		if (BeamInTestMode) {
			// in test-mode the main-window will only be shown when neccessary:
			TheMainWindow->Hide();
			// Now wait until Test-thread allows us to start...
			snooze( 200*1000);
			mStartupLocker->Lock();
		}

		EnsureIndexExists( "MAIL:identity");
		EnsureIndexExists( "MAIL:account");

		if (ThePrefs->GetBool( "UseDeskbar"))
			InstallDeskbarItem();

		BM_LOG( BM_LogApp, BmString("Showing main-window."));
		TheMainWindow->Show();

		// start most of our list-models:
		BM_LOG( BM_LogApp, BmString("...querying people..."));
		ThePeopleList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading POP-accounts..."));
		ThePopAccountList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading identities..."));
		TheIdentityList->StartJobInThisThread();

		BM_LOG( BM_LogApp, BmString("...reading signatures..."));
		TheSignatureList->StartJobInThisThread();

		BM_LOG( BM_LogApp, BmString("...reading filters..."));
		TheFilterList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading filter-chains..."));
		TheFilterChainList->StartJobInNewThread();

		BM_LOG( BM_LogApp, BmString("...reading SMTP-accounts..."));
		TheSmtpAccountList->StartJobInThisThread();

		if (BeamInTestMode)
			mStartupLocker->Unlock();
		tid = inherited::Run();
	
		ThePopAccountList->Store();
							// always store pop-account-list since the list of 
							// received mails may have changed
		TheIdentityList->Store();
							// always store identity-list since the current identity
							// may have changed
#ifdef BM_DEBUG_MEM
		(new BAlert( "", "End of Beam(1), check mem-usage!!!", "OK"))->Go();
		ThePopAccountList = NULL;
		(new BAlert( "", "End of Beam(2), check mem-usage!!!", "OK"))->Go();
		TheSmtpAccountList = NULL;
		(new BAlert( "", "End of Beam(3), check mem-usage!!!", "OK"))->Go();
		TheMailFolderList = NULL;
		(new BAlert( "", "End of Beam(4), check mem-usage!!!", "OK"))->Go();
#endif
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		exit(10);
	}
	return tid;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::InstallDeskbarItem() {
	if (!mDeskbar.HasItem( BM_DeskbarItemName)) {
		status_t res;
		app_info appInfo;
		if ((res = bmApp->GetAppInfo( &appInfo)) == B_OK) {
			int32 id;
			appInfo.ref.set_name( BM_DeskbarItemName);
			if ((res = mDeskbar.AddItem( &appInfo.ref, &id)) != B_OK) {
				BM_SHOWERR( BmString("Unable to install Beam_DeskbarItem (")
								<< BM_DeskbarItemName<<").\nError: \n\t" 
								<< strerror( res));
			} else
				mDeskbarItemIsOurs = true;
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::RemoveDeskbarItem() {
	if (mDeskbarItemIsOurs && mDeskbar.HasItem( BM_DeskbarItemName))
		mDeskbar.RemoveItem( BM_DeskbarItemName);
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmApplication::QuitRequested() {
	BM_LOG( BM_LogApp, "App: quit requested, checking state...");
	mIsQuitting = true;
	TheMailMonitor->LockLooper();

	bool shouldQuit = true;
	int32 count = CountWindows();
	// first check if there are any active jobs:
	if (TheJobStatusWin && TheJobStatusWin->HasActiveJobs()) {
		BAlert* alert = new BAlert( "Active Jobs", 
			"There are still some jobs/connections active!"
				"\nDo you really want to quit now?",
			"Yes, Quit Now", "Cancel", NULL, B_WIDTH_AS_USUAL,
			B_WARNING_ALERT
		);
		alert->SetShortcut( 1, B_ESCAPE);
		if (alert->Go() == 1)
			shouldQuit = false;
	}
	if (shouldQuit) {
		// ask all windows if they are ready to quit, in which case we actually
		// do quit (only if *ALL* windows indicate that they are prepared to quit!):
		for( int32 i=count-1; shouldQuit && i>=0; --i) {
			BWindow* win = bmApp->WindowAt( i);
			win->Lock();
			if (win && !win->QuitRequested())
				shouldQuit = false;
			win->Unlock();
		}
	}
	
	if (!shouldQuit) {
		TheMailMonitor->UnlockLooper();
		mIsQuitting = false;
	} else {
		TheMailMonitor->Quit();
		for( int32 i=count-1; i>=0; --i) {
			BWindow* win = bmApp->WindowAt( i);
			if (win) {
				win->LockLooper();
				win->Quit();
			}
		}
	}
	BM_LOG( BM_LogApp, 
			  shouldQuit ? "ok, App is quitting" : "no, app isn't quitting");
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	ArgvReceived( argc, argv)
		-	first argument is interpreted to be a destination mail-address, so a 
			new mail is generated for if an argument has been provided
\*------------------------------------------------------------------------------*/
void BmApplication::ArgvReceived( int32 argc, char** argv) {
	if (argc>1 && !BeamInTestMode) {
		BmString to( argv[1]);
		if (to.ICompare("mailto:",7)==0)
			LaunchURL( to);
		else {
			BMessage msg(BMM_NEW_MAIL);
			msg.AddString( MSG_WHO_TO, to.String());
			PostMessage( &msg);
		}
	}
}

/*------------------------------------------------------------------------------*\
	RefsReceived( msg)
		-	every given reference is opened, draft and pending messages are shown
			in the mail-edit-window, while other mails are opened view-only.
\*------------------------------------------------------------------------------*/
void BmApplication::RefsReceived( BMessage* msg) {
	if (!msg)
		return;
	entry_ref inref;
	entry_ref eref;
	BEntry entry;
	struct stat st;
	for( int index=0; msg->FindRef( "refs", index, &inref) == B_OK; ++index) {
		if (entry.SetTo( &inref, true) != B_OK
		|| entry.GetRef( &eref) != B_OK
		|| entry.GetStat( &st) != B_OK)
			continue;
		BmRef<BmMailRef> ref = BmMailRef::CreateInstance( NULL, eref, st);
		if (!ref)
			continue;
		if (ref->Status() == BM_MAIL_STATUS_DRAFT
		|| ref->Status() == BM_MAIL_STATUS_PENDING) {
			BmMailEditWin* editWin = BmMailEditWin::CreateInstance( ref.Get());
			if (editWin) {
				editWin->Show();
				if (!mMailWin)
					mMailWin = editWin;
			}
		} else {
			BmMailViewWin* viewWin = BmMailViewWin::CreateInstance( ref.Get());
			if (viewWin) {
				viewWin->Show();
				if (!mMailWin)
					mMailWin = viewWin;
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	handles all actions on mailrefs, like replying, forwarding, and creating
			new mails
\*------------------------------------------------------------------------------*/
void BmApplication::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BMM_CHECK_ALL: {
				while( ThePopAccountList->IsJobRunning())
					snooze( 200*1000);
				ThePopAccountList->CheckMail( true);
				break;
			}
			case BMM_CHECK_MAIL: {
				while( ThePopAccountList->IsJobRunning())
					snooze( 200*1000);
				const char* key = NULL;
				msg->FindString( BmPopAccountList::MSG_ITEMKEY, &key);
				if (key) {
					bool isAutoCheck 
						= msg->FindBool( BmPopAccountList::MSG_AUTOCHECK);
					BM_LOG( BM_LogApp, 
							  BmString("PopAccount ") << key << " asks to check mail " 
							  	<< (isAutoCheck ? "(auto)" : "(manual)"));
					if (!isAutoCheck 
					|| !ThePrefs->GetBool( "AutoCheckOnlyIfPPPRunning", true) 
					|| IsPPPRunning()) {
						BM_LOG( BM_LogApp, 
								  BmString("PopAccount ") << key	
								  		<< ": mail is checked now");
						ThePopAccountList->CheckMailFor( key, isAutoCheck);
					} else
						BM_LOG( BM_LogApp, 
								  BmString("PopAccount ") << key	
								  		<< ": mail is not checked (PPP isn't running)");
				} else {
					BM_LOG( BM_LogApp, "Request to check mail for all accounts");
					ThePopAccountList->CheckMail();
				}
				break;
			}
			case BMM_NEW_MAIL: {
				BmRef<BmMail> mail = new BmMail( true);
				const char* to = NULL;
				if ((to = msg->FindString( MSG_WHO_TO))!=NULL)
					mail->SetFieldVal( BM_FIELD_TO, to);
				const char* optField = NULL;
				int32 i=0;
				for( ; msg->FindString( MSG_OPT_FIELD, i, &optField)==B_OK; ++i) {
					mail->SetFieldVal( optField, msg->FindString( MSG_OPT_VALUE, i));
				}
				BM_LOG( BM_LogApp, 
						  BmString("Asked to create new mail with ") << i 
						  	<< " options");
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance( mail.Get());
				if (editWin)
					editWin->Show();
				break;
			}
			case BMM_REDIRECT: {
				BmMailRef* mailRef = NULL;
				int index=0;
				while( msg->FindPointer( 
					MSG_MAILREF, index++, (void**)&mailRef
				) == B_OK) {
					BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
					if (mail) {
						BM_LOG( BM_LogApp, 
								  BmString("Asked to redirect mail <") 
								  		<< mailRef->TrackerName() << ">");
						if (mail->InitCheck() != B_OK)
							mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
						if (mail->InitCheck() != B_OK)
							continue;
						BmRef<BmMail> newMail;
						newMail = mail->CreateRedirect();
						if (newMail) {
							BmMailEditWin* editWin 
								= BmMailEditWin::CreateInstance( newMail.Get());
							if (editWin)
								editWin->Show();
						}
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case BMM_EDIT_AS_NEW: {
				BmMailRef* mailRef = NULL;
				int index=0;
				while( msg->FindPointer( 
					MSG_MAILREF, index++, (void**)&mailRef) == B_OK
				) {
					BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
					if (mail) {
						BM_LOG( BM_LogApp, 
								  BmString("Asked to edit mail <") 
								  		<< mailRef->TrackerName() << "> as new");
						if (mail->InitCheck() != B_OK)
							mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
						if (mail->InitCheck() != B_OK)
							continue;
						BmRef<BmMail> newMail;
						newMail = mail->CreateAsNew();
						if (newMail) {
							BmMailEditWin* editWin 
								= BmMailEditWin::CreateInstance( newMail.Get());
							if (editWin)
								editWin->Show();
						}
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case BMM_REPLY:
			case BMM_REPLY_LIST:
			case BMM_REPLY_ORIGINATOR:
			case BMM_REPLY_ALL: {
				int32 buttonPressed=0;
				type_code tc;
				int32 msgCount;
				status_t result = msg->GetInfo( MSG_MAILREF, &tc, &msgCount);
				if (result != B_OK)
					break;
				if (msgCount>1 
				&& msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about how to forward multiple messages:
					BmString s("You have selected more than one message.\n\n"
								  "Should Beam join the message-bodies into one "
								  "single mail and reply to that or would you prefer "
								  "to keep the messages separate?");
					BAlert* alert = new BAlert( 
						"Forwarding Multiple Mails", s.String(),
						"Keep Separate", "Join per Recipient", "Join All", 
						B_WIDTH_AS_USUAL,	B_EVEN_SPACING, B_IDEA_ALERT
					);
					alert->Go( new BInvoker( new BMessage(*msg), 
													 BMessenger( this)));
				} else {
					BM_LOG( BM_LogApp, 
							  BmString("Asked to reply to ") << msgCount << " mails.");
					ReplyToMails( msg, buttonPressed>0, buttonPressed==2);
				}
				break;
			}
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				int32 buttonPressed=1;
				type_code tc;
				int32 msgCount;
				status_t result = msg->GetInfo( MSG_MAILREF, &tc, &msgCount);
				if (result != B_OK)
					break;
				if (msgCount>1 
				&& msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about how to forward multiple messages:
					BmString s("You have selected more than one message.\n\n"
								  "Should Beam join the message-bodies into one "
								  "single mail and forward that or would you prefer "
								  "to keep the messages separate?");
					BAlert* alert = new BAlert( 
						"Forwarding Multiple Mails", s.String(),
					 	"Cancel", "Keep Separate", "Join", 
					 	B_WIDTH_AS_USUAL,	B_OFFSET_SPACING, B_IDEA_ALERT
					);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(*msg), 
													 BMessenger( this)));
				} else {
					if (buttonPressed > 0) {
						BM_LOG( BM_LogApp, 
							BmString("Asked to forward ") << msgCount << " mails.");
						ForwardMails( msg, buttonPressed==2);
					}
				}
				break;
			}
			case BMM_MARK_AS: {
				int32 buttonPressed=1;
				BmMailRef* mailRef = NULL;
				type_code tc;
				int32 msgCount;
				status_t result = msg->GetInfo( MSG_MAILREF, &tc, &msgCount);
				if (result != B_OK)
					break;
				int index=0;
				BmString newStatus = msg->FindString( MSG_STATUS);
				if (msgCount>1 
				&& msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, check if user has asked to mark as read and 
					// has selected messages with advanced statii (like replied
					// or forwarded). If so,  we ask the user about how to 
					// handle those:
					if (newStatus == BM_MAIL_STATUS_READ) {
						bool hasAdvanced = false;
						while( msg->FindPointer( 
							MSG_MAILREF, index++, (void**)&mailRef
						) == B_OK) {
							if (mailRef->Status() != BM_MAIL_STATUS_NEW
							&&  mailRef->Status() != BM_MAIL_STATUS_READ) {
								hasAdvanced = true;
								break;
							}
						}
						if (hasAdvanced) {
							BmString s("Some of the selected messages are not new."
										  "\n\nShould Beam mark only the new messages "
										  "as being read?");
							BAlert* alert = new BAlert( 
								"Mark Mails As", s.String(), "Cancel", 
								"Mark All Messages", "Just Mark New Messages", 
								B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_IDEA_ALERT
							);
							alert->SetShortcut( 0, B_ESCAPE);
							alert->Go( new BInvoker( new BMessage(*msg), 
															 BMessenger( this)));
							break;
						} else
							buttonPressed = 1;	// mark all messages
					} else
						buttonPressed = 1;	// mark all messages
				}
				if (buttonPressed == 0)
					break;
				index=0;
				while( msg->FindPointer( 
					MSG_MAILREF, index++, (void**)&mailRef
				) == B_OK) {
					if (buttonPressed==1 
					|| mailRef->Status() == BM_MAIL_STATUS_NEW) {
						BM_LOG( BM_LogApp, 
								  BmString("marking mail <") << mailRef->TrackerName()
								  		<< "> as " << newStatus);
						mailRef->MarkAs( newStatus.String());
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case BMM_MOVE: {
				static int jobNum = 1;
				BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
				BmMailRef* ref;
				for( int i=0; msg->FindPointer( 
					MSG_MAILREF, i, (void**)&ref)==B_OK; ++i
				) {
					if (ref) {
						tmpMsg.AddRef( BmMailMover::MSG_REFS, ref->EntryRefPtr());
						ref->RemoveRef();		// msg no longer refers to mail-ref
					}
				}
				BmString folderName = msg->FindString( BmListModel::MSG_ITEMKEY);
				BmRef<BmListModelItem> itemRef 
					= TheMailFolderList->FindItemByKey( folderName);
				BmMailFolder* folder = dynamic_cast< BmMailFolder*>( itemRef.Get());
				if (!folder)
					break;
				BmString jobName = folder->DisplayKey();
				jobName << jobNum++;
				tmpMsg.AddString( BmJobModel::MSG_JOB_NAME, jobName.String());
				tmpMsg.AddString( BmJobModel::MSG_MODEL, folder->Key().String());
				TheJobStatusWin->PostMessage( &tmpMsg);
				break;
			}
			case c_about_window_url_invoked: {
				const char* url;
				if (msg->FindString( "url", &url) == B_OK) {
					LaunchURL( url);
				}
				break;
			}
			case BMM_PAGE_SETUP: {
				PageSetup();
				break;
			}
			case BMM_PRINT: {
				if (!mPrintSetup)
					PageSetup();
				if (mPrintSetup)
					PrintMails( msg);
				break;
			}
			case BMM_PREFERENCES: {
				if (!ThePrefsWin) {
					BmPrefsWin::CreateInstance();
					ThePrefsWin->Show();
				} else  {
					if (ThePrefsWin->LockLooper()) {
						ThePrefsWin->Hide();
						ThePrefsWin->Show();
						ThePrefsWin->UnlockLooper();
					}
				}
				BmString subViewName = msg->FindString( "SubViewName");
				if (subViewName.Length())
					ThePrefsWin->PostMessage( msg);
				break;
			}
			case BMM_TRASH: {
				BmMailRef* mailRef = NULL;
				type_code typeFound;
				int32 countFound;
				msg->GetInfo( MSG_MAILREF, &typeFound, &countFound);
				if (countFound>0) {
					struct entry_ref *refs = new entry_ref [countFound];
					int index=0;
					for(  index=0; 
							msg->FindPointer( 
								MSG_MAILREF, index, (void**)&mailRef
							) == B_OK; 
							++index) {
						BM_LOG( BM_LogApp, 
								  BmString("Asked to trash mail <") 
								  		<< mailRef->TrackerName() << ">");
						refs[index] = mailRef->EntryRef();
						mailRef->MarkAs( BM_MAIL_STATUS_READ);
								// mark mail as read before moving to trash, so that
								// we don't confuse our new-mail counter
						mailRef->RemoveRef();	// msg is no more refering to mailRef
					}
					MoveToTrash( refs, index);
					delete [] refs;
				}
				break;
			}
			case BM_FILL_MENU_FROM_LIST: {
				BmString listName = msg->FindString( MSG_LIST_NAME);
				BMenu* menu;
				if (msg->FindPointer( MSG_MENU_POINTER, (void**)&menu) != B_OK)
					break;
				BHandler* target;
				if (msg->FindPointer( MSG_MENU_TARGET, (void**)&target) != B_OK)
					break;
				BMessage msgTempl;
				if (msg->FindMessage( MSG_MSG_TEMPLATE, &msgTempl) != B_OK)
					break;
				if (menu) {
					BFont font;
					menu->GetFont( &font);
					if (listName == BM_STATUSLIST_NAME) {
						const char* stats[] = {
							BM_MAIL_STATUS_DRAFT, BM_MAIL_STATUS_FORWARDED, 
							BM_MAIL_STATUS_NEW, 
							BM_MAIL_STATUS_PENDING, BM_MAIL_STATUS_READ, 
							BM_MAIL_STATUS_REDIRECTED,
							BM_MAIL_STATUS_REPLIED, BM_MAIL_STATUS_SENT,	NULL
						};
						for( int i=0; stats[i]; ++i) {
							BMenuItem* item 
								= new BMenuItem( stats[i], new BMessage(msgTempl));
							item->SetTarget( target);
							menu->AddItem( item);
						}
					} else if (listName == BM_FOLDERLIST_NAME) {
						AddListToMenu( TheMailFolderList.Get(), menu, &msgTempl, 
											target, &font, true);
					} else if (listName == BM_IDENTITYLIST_NAME) {
						AddListToMenu( TheIdentityList.Get(), menu, &msgTempl, 
											target, &font);
					}
				}
				break;
			}
			case B_SILENT_RELAUNCH: {
				BM_LOG( BM_LogApp, "App: silently relaunched");
				if (TheMainWindow->IsMinimized())
					TheMainWindow->Minimize( false);
				inherited::MessageReceived( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
				break;
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("BmApp: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ForwardMails( msg, join)
		-	forwards all mailref's contained in the given message.
		-	depending on the param join, multiple mails are joined into one single 
			forward or one forward is generated for each mail.
\*------------------------------------------------------------------------------*/
void BmApplication::ForwardMails( BMessage* msg, bool join) {
	if (!msg)
		return;
	// tell sending ref-view that we are doing work:
	BMessenger sendingRefView;
	msg->FindMessenger( MSG_SENDING_REFVIEW, &sendingRefView);
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_SET_BUSY);

	BmMailRef* mailRef = NULL;
	BmRef<BmMail> newMail;
	const char* selectedText = NULL;
	msg->FindString( MSG_SELECTED_TEXT, &selectedText);
	if (join) {
		// fetch mail-refs from message and sort them chronologically:
		typedef multimap< time_t, BmMailRef* >BmSortedRefMap;
		BmSortedRefMap sortedRefMap;
		for(  int index=0; 
				msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK;
				++index) {
			if (mailRef)
				sortedRefMap.insert( pair<const time_t, 
											BmMailRef*>( mailRef->When(), mailRef));
		}
		// now iterate over the sorted mail-refs:
		for(  BmSortedRefMap::const_iterator iter = sortedRefMap.begin(); 
				iter != sortedRefMap.end(); 
				++iter) {
			mailRef = iter->second;
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					continue;
				if (msg->what == BMM_FORWARD_ATTACHED) {
					if (iter == sortedRefMap.begin()) 
						newMail = mail->CreateAttachedForward();
					else
						newMail->AddAttachmentFromRef( mailRef->EntryRefPtr(),
																 mail->DefaultCharset());
				} else if (msg->what == BMM_FORWARD_INLINE) {
					if (iter == sortedRefMap.begin()) 
						newMail = mail->CreateInlineForward( false, selectedText);
					else
						newMail->AddPartsFromMail( mail, false, BM_IS_FORWARD, false);
				} else if (msg->what == BMM_FORWARD_INLINE_ATTACH) {
					if (iter == sortedRefMap.begin()) 
						newMail = mail->CreateInlineForward( true, selectedText);
					else
						newMail->AddPartsFromMail( mail, true, BM_IS_FORWARD, false);
				}
				if (iter == sortedRefMap.begin()) {
					// set subject for multiple forwards:
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString subject = newMail->CreateForwardSubjectFor( 
						oldSub + " (and more...)"
					);
					newMail->SetFieldVal( BM_FIELD_SUBJECT, subject);
				}
			}
			mailRef->RemoveRef();	// msg is no more refering to mailRef
		}
		if (newMail) {
			BmMailEditWin* editWin 
				= BmMailEditWin::CreateInstance( newMail.Get());
			if (editWin)
				editWin->Show();
		}
	} else {
		// create one forward per mail:
		for(  int index=0; 
				msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK;
				++index) {
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					continue;
				if (msg->what == BMM_FORWARD_ATTACHED) {
					newMail = mail->CreateAttachedForward();
				} else if (msg->what == BMM_FORWARD_INLINE) {
					newMail = mail->CreateInlineForward( 
						false, index==0 ? selectedText : NULL
					);
				} else if (msg->what == BMM_FORWARD_INLINE_ATTACH) {
					newMail = mail->CreateInlineForward( 
						true, index==0 ? selectedText : NULL
					);
				}
				if (newMail) {
					BmMailEditWin* editWin 
						= BmMailEditWin::CreateInstance( newMail.Get());
					if (editWin)
						editWin->Show();
				}
			}
			mailRef->RemoveRef();		// msg is no more refering to mailRef
		}
	}
	// now tell sending ref-view that we are finished:
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_UNSET_BUSY);
}

/*------------------------------------------------------------------------------*\
	ReplyToMails( msg, join)
		-	replies to all mailref's contained in the given message.
		-	depending on the param join, multiple mails are joined into one single 
			reply (one per originator) or one reply is generated for each mail.
\*------------------------------------------------------------------------------*/
void BmApplication::ReplyToMails( BMessage* msg, bool join, bool joinIntoOne) {
	if (!msg)
		return;
	bool replyGoesToPersonOnly = false;
	// tell sending ref-view that we are doing work:
	BMessenger sendingRefView;
	msg->FindMessenger( MSG_SENDING_REFVIEW, &sendingRefView);
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_SET_BUSY);

	BmMailRef* mailRef = NULL;
	BmString selectedText = msg->FindString( MSG_SELECTED_TEXT);
	if (join) {
		// fetch mail-refs from message and sort them chronologically:
		typedef multimap< time_t, BmMailRef* >BmSortedRefMap;
		BmSortedRefMap sortedRefMap;
		for(  int index=0; 
				msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK;
				++index) {
			if (mailRef)
				sortedRefMap.insert( pair<const time_t, 
											BmMailRef*>( mailRef->When(), mailRef));
		}
		// we file mails into the different originator-slots:
		typedef map< BmString, BmRef<BmMail> >BmNewMailMap;
		BmNewMailMap newMailMap;
		for(  BmSortedRefMap::const_iterator iter = sortedRefMap.begin(); 
				iter != sortedRefMap.end(); 
				++iter) {
			mailRef = iter->second;
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					continue;
				BmString replyAddr 
					= mail->DetermineReplyAddress( msg->what, true,
															 replyGoesToPersonOnly);
				BmString replyAddrSpec = BmAddress( replyAddr).AddrSpec(); 
				BmString index = joinIntoOne 
											? BmString("single") 
											: replyAddrSpec;
				BmRef<BmMail>& newMail = newMailMap[index];
				if (!newMail) {
					if (joinIntoOne)
						newMail = mail->CreateReply( msg->what, selectedText);
							// when joining all into one we need the names,
							// since there are multiple recipients
					else
						newMail = mail->CreateReply( msg->what, 
															  replyGoesToPersonOnly, 
															  selectedText);
				} else {
					newMail->AddPartsFromMail( mail, false, BM_IS_REPLY, 
														joinIntoOne 
															? false
															: replyGoesToPersonOnly);
					BmRef<BmMailHeader> hdr( newMail->Header());
					if (!hdr->AddressFieldContainsAddress( BM_FIELD_TO, replyAddr))
						hdr->AddFieldVal( BM_FIELD_TO, replyAddr);
				}
				if (iter == sortedRefMap.begin()) {
					// set subject for multiple replies:
					BmString oldSub = mail->GetFieldVal( BM_FIELD_SUBJECT);
					BmString subject 
						= newMail->CreateReplySubjectFor( oldSub + " (and more...)");
					newMail->SetFieldVal( BM_FIELD_SUBJECT, subject);
				}
			}
			mailRef->RemoveRef();	// msg is no more refering to mailRef
		}
		// ...and now show all the freshly generated mails:
		for(  BmNewMailMap::const_iterator iter=newMailMap.begin(); 
			   iter!=newMailMap.end(); 
			   ++iter) {
			BmMailEditWin* editWin 
				= BmMailEditWin::CreateInstance( iter->second.Get());
			if (editWin)
				editWin->Show();
		}
	} else {
		// create one reply per mail:
		BmRef<BmMail> newMail;
		for(  int index=0; 
				msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK;
				++index) {
			BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() != B_OK)
					continue;
				newMail = mail->CreateReply( msg->what, replyGoesToPersonOnly, 
													  index==0 
													  		? selectedText 
													  		: BM_DEFAULT_STRING);
				if (newMail) {
					BmMailEditWin* editWin 
						= BmMailEditWin::CreateInstance( newMail.Get());
					if (editWin)
						editWin->Show();
				}
			}
			mailRef->RemoveRef();		// msg is no more refering to mailRef
		}
	}
	// now tell sending ref-view that we are finished:
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_UNSET_BUSY);
}

/*------------------------------------------------------------------------------*\
	PrintMails( msg)
		-	prints all mailref's contained in the given message.
\*------------------------------------------------------------------------------*/
void BmApplication::PrintMails( BMessage* msg) {
	if (!mPrintSetup)
		PageSetup();
	if (!mPrintSetup)
		return;

	// tell sending ref-view that we are doing work:
	BMessenger sendingRefView;
	msg->FindMessenger( MSG_SENDING_REFVIEW, &sendingRefView);
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_SET_BUSY);

	mPrintJob.SetSettings( new BMessage( *mPrintSetup));
	status_t result = mPrintJob.ConfigJob();
	if (result == B_OK) {
		delete mPrintSetup;
		mPrintSetup = mPrintJob.Settings();
		int32 firstPage = mPrintJob.FirstPage();
		int32 lastPage = mPrintJob.LastPage();
		if (lastPage-firstPage+1 <= 0)
			return;
		BmMailRef* mailRef = NULL;
		// we create a hidden mail-view-window which is being used for printing:
		BmMailViewWin* mailWin = BmMailViewWin::CreateInstance();
		mailWin->Hide();
		mailWin->Show();
		BmMailView* mailView = mailWin->MailView();
		// now get printable rect...
		BRect printableRect = mPrintJob.PrintableRect();
		// ...and adjust mailview accordingly (to use the available space
		// effectively):
		mailView->LockLooper();
		mailWin->ResizeTo( printableRect.Width()+8, 600);
		mailView->UnlockLooper();
		mailView->BodyPartView()->IsUsedForPrinting( true);
		// now we start printing...
		mPrintJob.BeginJob();
		int32 page = 1;
		for(  int mailIdx=0; 
				msg->FindPointer( MSG_MAILREF, mailIdx, (void**)&mailRef) == B_OK
				&& page<=lastPage;
				++mailIdx) {
			mailView->LockLooper();
			mailView->BodyPartView()->SetViewColor( White);
			mailView->UnlockLooper();
			mailView->ShowMail( mailRef, false);
			while( !mailView->IsDisplayComplete())
				snooze( 50*1000);
			BRect currFrame = printableRect.OffsetToCopy( 0, 0);
			BRect textRect = mailView->TextRect();
			float totalHeight = textRect.top+mailView->TextHeight(0,100000);
			float height = currFrame.Height();
			BPoint topOfLine 
				= mailView->PointAt( mailView->OffsetAt( BPoint( 
					5, currFrame.bottom
				)));
			currFrame.bottom = topOfLine.y-1;
			while( page<=lastPage) {
				if (page >= firstPage) {
					mPrintJob.DrawView( mailView, currFrame, BPoint(0,0));
					mPrintJob.SpoolPage();
				}
				currFrame.top = currFrame.bottom+1;
				currFrame.bottom = currFrame.top + height-1;
				mailView->LockLooper();
				topOfLine 
					= mailView->PointAt( mailView->OffsetAt( BPoint( 
						5, currFrame.bottom
					)));
				mailView->UnlockLooper();
				currFrame.bottom = topOfLine.y-1;
				page++;
				if (currFrame.top >= totalHeight || currFrame.Height() <= 1)
					// end of current mail reached
					break;
			}
			mailRef->RemoveRef();	// msg is no more refering to mailRef
		}
		mPrintJob.CommitJob();
		mailWin->PostMessage( B_QUIT_REQUESTED);
	}
	// now tell sending ref-view that we are finished:
	if (sendingRefView.IsValid())
		sendingRefView.SendMessage( BMM_UNSET_BUSY);
}

/*------------------------------------------------------------------------------*\
	PageSetup()
		-	sets up the basic printing environment
\*------------------------------------------------------------------------------*/
void BmApplication::PageSetup() {
	if (mPrintSetup)
		mPrintJob.SetSettings( new BMessage( *mPrintSetup));
		
	status_t result = mPrintJob.ConfigPage();
	if (result == B_OK) {
		delete mPrintSetup;
		mPrintSetup = mPrintJob.Settings();
	}
}

/*------------------------------------------------------------------------------*\
	LaunchURL( url)
		-	launches the corresponding program for the given URL 
			(usually Netpositive)
		-	mailto: - URLs are handled internally
\*------------------------------------------------------------------------------*/
void BmApplication::LaunchURL( const BmString url) {
	char* urlStr = const_cast<char*>( url.String());
	status_t result;
	if (url.ICompare( "https:", 6) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.https", 1, &urlStr);
	else if (url.ICompare( "http:", 5) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.http", 1, &urlStr);
	else if (url.ICompare( "ftp:", 4) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.ftp", 1, &urlStr);
	else if (url.ICompare( "file:", 5) == 0)
		result = be_roster->Launch( "application/x-vnd.Be.URL.file", 1, &urlStr);
	else if (url.ICompare( "mailto:", 7) == 0) {
		BMessage msg(BMM_NEW_MAIL);
		BmString to( urlStr+7);
		int32 optPos = to.IFindFirst("?");
		if (optPos != B_ERROR) {
			BmString opts;
			to.MoveInto( opts, optPos, to.Length());
			Regexx rx;
			int32 optCount = rx.exec( opts, "[?&]([^&=]+)=([^&]+)", 
											  Regexx::global);
			for( int i=0; i<optCount; ++i) {
				BmString rawVal( rx.match[i].atom[1]);
				rawVal.DeUrlify();
				BmString field( rx.match[i].atom[0]);
				msg.AddString( MSG_OPT_FIELD, field.String());
				msg.AddString( MSG_OPT_VALUE, rawVal.String());
			}
		}
		to.DeUrlify();
		msg.AddString( BmApplication::MSG_WHO_TO, to.String());
		bmApp->PostMessage( &msg);
		return;
	}
	else result = B_ERROR;
	if (!(result == B_OK || result == B_ALREADY_RUNNING)) {
		(new BAlert( "", (BmString("Could not launch application for url\n\t") 
								<< url << "\n\nError:\n\t" 
								<< strerror(result)).String(),
					   "OK"))->Go();
	}
}

/*------------------------------------------------------------------------------*\
	AboutRequested()
		-	standard BeOS-behaviour, we show about-window
\*------------------------------------------------------------------------------*/
void BmApplication::AboutRequested() {
	ImageAboutWindow* aboutWin = new ImageAboutWindow(
		"About Beam",
		"Beam",
		TheResources->IconByName("AboutIcon"),
		15, 
		"BEware, Another Mailer\n(c) Oliver Tappe, Berlin, Germany",
		"mailto:beam@hirschkaefer.de",
		"http://beam.sourceforge.net",
		"\n\n\n\n\n\n\nThanks to:\n\
\n\
Heike Herfart \n\
	for understanding the geek, \n\
	the testing sessions \n\
	and many, many suggestions. \n\
\n\
\n\
...and (in alphabetical order):\n\
\n\
Adam McNutt\n\
Atillâ Öztürk\n\
Bernd Thorsten Korz\n\
Cedric Vincent\n\
Charlie Clark\n\
Eberhard Hafermalz\n\
Eugenia Loli-Queru\n\
Helmar Rudolph\n\
Ingo Weinhold\n\
Jace Cavacini\n\
Jon Hart\n\
Kevin Musick\n\
Koki\n\
Lars Müller\n\
Len G. Jacob\n\
Linus Almstrom\n\
Mathias Reitinger\n\
Max Hartmann\n\
MDR-team (MailDaemonReplacement)\n\
Mikhail Panasyuk\n\
Olivier Milla\n\
qwilk\n\
Paweł Lewicki\n\
Rainer Riedl\n\
Rob Lund\n\
Shard\n\
Stephan Assmus\n\
Stephan Buelling\n\
Stephen Butters\n\
Tyler Dauwalder\n\
Zach\n\
\n\
\n\n\n\n\
...and thanks to everyone I forgot, too!\n\
\n\
\n\n\n\n\n\n"
	);
	aboutWin->Show();
}

/*------------------------------------------------------------------------------*\
	ScreenFrame()
		-	returns the the current screen's frame
\*------------------------------------------------------------------------------*/
BRect BmApplication::ScreenFrame() {
	BScreen screen( TheMainWindow);
	if (!screen.IsValid())
		BM_SHOWERR( BmString("Could not initialize BScreen object !?!"));
	return screen.Frame();
}

/*------------------------------------------------------------------------------*\
	SetNewWorkspace( newWorkspace)
		-	ensures that main-window and job-status-window are always shown inside
			the same workspace
\*------------------------------------------------------------------------------*/
void BmApplication::SetNewWorkspace( uint32 newWorkspace) {
	if (TheMainWindow->Workspaces() != newWorkspace)
		TheMainWindow->SetWorkspaces( newWorkspace);
	if (TheJobStatusWin->Workspaces() != newWorkspace)
		TheJobStatusWin->SetWorkspaces( newWorkspace);
}

/*------------------------------------------------------------------------------*\
	CurrWorkspace()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmApplication::CurrWorkspace() {
	return TheMainWindow->Workspaces();
}

/*------------------------------------------------------------------------------*\
	HandlesMimetype( mimetype)
		-	determines whether or not Beam handles the given mimetype
		-	returns true for text/x-email and message/rfc822, false otherwise
\*------------------------------------------------------------------------------*/
bool BmApplication::HandlesMimetype( const BmString mimetype) {
	return mimetype.ICompare( "text/x-email")==0 
			 || mimetype.ICompare( "message/rfc822")==0;
}
