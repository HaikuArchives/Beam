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
#include <Roster.h>
#include <Screen.h>

#include "ImageAboutWindow.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmDataModel.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmMailViewWin.h"
#include "BmMainWindow.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig)
	:	inherited( sig)
	,	mIsQuitting( false)
	,	mInitCheck( B_NO_INIT)
{
	if (InstanceCount > 0)
		throw BM_runtime_error("Trying to initialize more than one instance of class Beam");

	bmApp = this;

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

		// create the log-handler:
		node_ref nref;
		nref.device = appInfo.ref.device;
		nref.node = appInfo.ref.directory;
		BmLogHandler::CreateInstance( 1, &nref);

		// load/determine all needed resources:
		BmResources::CreateInstance();
		time_t appModTime;
		appFile.GetModificationTime( &appModTime);
		TheResources->CheckMimeTypeFile( sig, appModTime);

		// load the preferences set by user (if any):
		BmPrefs::CreateInstance();
		
		// create the node-monitor looper:
		BmNodeMonitor::CreateInstance();

		// create the job status window:
		BmJobStatusWin::CreateInstance();
		TheJobStatusWin->Hide();
		TheJobStatusWin->Show();

		BmSignatureList::CreateInstance();
		TheSignatureList->StartJobInThisThread();

		BmMailFolderList::CreateInstance();
		BmSmtpAccountList::CreateInstance( TheJobStatusWin);
		BmPopAccountList::CreateInstance( TheJobStatusWin);

		BmMainWindow::CreateInstance();

		mInitCheck = B_OK;
		InstanceCount++;
	} catch (exception& err) {
		ShowAlert( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BmApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmApplication::~BmApplication() {
	TheMailFolderList = NULL;
	ThePopAccountList = NULL;
	TheSmtpAccountList = NULL;
	TheSignatureList = NULL;
#ifdef BM_REF_DEBUGGING
	BmRefObj::PrintRefsLeft();
#endif
	delete ThePrefs;
	delete TheResources;
	delete TheLogHandler;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	ReadyToRun()
		-	
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
	Run()
		-	
\*------------------------------------------------------------------------------*/
thread_id BmApplication::Run() {
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		TheSmtpAccountList->StartJobInThisThread();
		TheMainWindow->Show();
		tid = inherited::Run();
		ThePopAccountList->Store();
		TheSmtpAccountList->Store();
#ifdef BM_DEBUG_MEM
		(new BAlert( "", "End of Beam(1), check mem-usage!!!", "OK"))->Go();
		ThePopAccountList = NULL;
		(new BAlert( "", "End of Beam(2), check mem-usage!!!", "OK"))->Go();
		TheSmtpAccountList = NULL;
		(new BAlert( "", "End of Beam(3), check mem-usage!!!", "OK"))->Go();
		TheMailFolderList = NULL;
		(new BAlert( "", "End of Beam(4), check mem-usage!!!", "OK"))->Go();
#endif
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		exit(10);
	}
	return tid;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmApplication::QuitRequested() {
	mIsQuitting = true;
	TheNodeMonitor->LockLooper();
	bool shouldQuit = inherited::QuitRequested();
	if (!shouldQuit) {
		mIsQuitting = false;
		TheNodeMonitor->UnlockLooper();
	} else
		TheNodeMonitor->Quit();
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	ArgvReceived( )
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::ArgvReceived( int32 argc, char** argv) {
	if (argc>1) {
		BMessage msg(BMM_NEW_MAIL);
		msg.AddString( MSG_WHO_TO, argv[1]);
		PostMessage( &msg);
	}
}

/*------------------------------------------------------------------------------*\
	RefsReceived( )
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::RefsReceived( BMessage* msg) {
	if (!msg)
		return;
	entry_ref eref;
	BEntry entry;
	struct stat st;
	for( int index=0; msg->FindRef( "refs", index, &eref) == B_OK; ++index) {
		if (entry.SetTo( &eref) != B_OK)
			continue;
		if (entry.GetStat( &st) != B_OK)
			continue;
		BmRef<BmMailRef> ref = BmMailRef::CreateInstance( NULL, eref, st);
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
	MessageReceived( )
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BMM_CHECK_ALL: {
				ThePopAccountList->CheckMail( true);
				break;
			}
			case BMM_CHECK_MAIL: {
				const char* key = NULL;
				msg->FindString( BmPopAccountList::MSG_ITEMKEY, &key);
				if (key)
					ThePopAccountList->CheckMailFor( key);
				else
					ThePopAccountList->CheckMail();
				break;
			}
			case BMM_NEW_MAIL: {
				BmRef<BmMail> mail = new BmMail( true);
				const char* to = NULL;
				if ((to = msg->FindString( MSG_WHO_TO))) {
					BString toAddr( to);
					if (toAddr.ICompare( "mailto:",7) == 0)
						toAddr.Remove( 0,7);
					mail->SetFieldVal( BM_FIELD_TO, toAddr);
				}
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance( mail.Get());
				if (editWin)
					editWin->Show();
				break;
			}
			case BMM_REDIRECT: {
				BmMailRef* mailRef = NULL;
				int index=0;
				while( msg->FindPointer( MSG_MAILREF, index++, (void**)&mailRef) == B_OK) {
					BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
					if (mail) {
						mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
						if (mail->InitCheck() != B_OK)
							continue;
						BmRef<BmMail> newMail;
						newMail = mail->CreateRedirect();
						if (newMail) {
							BmMailEditWin* editWin = BmMailEditWin::CreateInstance( newMail.Get());
							if (editWin)
								editWin->Show();
						}
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case BMM_REPLY:
			case BMM_REPLY_ALL: {
				BmMailRef* mailRef = NULL;
				int index=0;
				const char* selectedText = NULL;
				msg->FindString( MSG_SELECTED_TEXT, &selectedText);
				while( msg->FindPointer( MSG_MAILREF, index++, (void**)&mailRef) == B_OK) {
					BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
					if (mail) {
						mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
						if (mail->InitCheck() != B_OK)
							continue;
						BmRef<BmMail> newMail;
						if (msg->what == BMM_REPLY)
							newMail = mail->CreateReply( false, selectedText);
						else if (msg->what == BMM_REPLY_ALL)
							newMail = mail->CreateReply( true, selectedText);
						if (newMail) {
							BmMailEditWin* editWin = BmMailEditWin::CreateInstance( newMail.Get());
							if (editWin)
								editWin->Show();
						}
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case BMM_FORWARD_ATTACHED:
			case BMM_FORWARD_INLINE:
			case BMM_FORWARD_INLINE_ATTACH: {
				BmMailRef* mailRef = NULL;
				BmRef<BmMail> newMail;
				const char* selectedText = NULL;
				msg->FindString( MSG_SELECTED_TEXT, &selectedText);
				for(  int index=0; 
						msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK;
						++index) {
					BmRef<BmMail> mail = BmMail::CreateInstance( mailRef);
					if (mail) {
						mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
						if (mail->InitCheck() != B_OK)
							continue;
						if (msg->what == BMM_FORWARD_ATTACHED) {
							if (index == 0) 
								newMail = mail->CreateAttachedForward();
							else
								newMail->AddAttachmentFromRef( mailRef->EntryRefPtr());
						} else if (msg->what == BMM_FORWARD_INLINE) {
							if (index == 0) 
								newMail = mail->CreateInlineForward( false, selectedText);
							else
								newMail->AddPartsFromMail( mail, false);
						} else if (msg->what == BMM_FORWARD_INLINE_ATTACH) {
							if (index == 0) 
								newMail = mail->CreateInlineForward( true, selectedText);
							else
								newMail->AddPartsFromMail( mail, true);
						}
						if (index == 1)
							// set simple subject for multiple forwards:
							newMail->SetFieldVal( BM_FIELD_SUBJECT, "Fwd: (multiple messages)");
					}
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				if (newMail) {
					BmMailEditWin* editWin = BmMailEditWin::CreateInstance( newMail.Get());
					if (editWin)
						editWin->Show();
				}
				break;
			}
			case BMM_MARK_AS: {
				BmMailRef* mailRef = NULL;
				int index=0;
				while( msg->FindPointer( MSG_MAILREF, index++, (void**)&mailRef) == B_OK) {
					mailRef->MarkAs( msg->FindString( MSG_STATUS));
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				break;
			}
			case c_about_window_url_invoked: {
				const char* url;
				if (msg->FindString( "url", &url) == B_OK) {
					LaunchURL( url);
				}
				break;
			}
			case BMM_TRASH: {
				BmMailRef* mailRef = NULL;
				int index=0;
				while( msg->FindPointer( MSG_MAILREF, index++, (void**)&mailRef) == B_OK);
				entry_ref* refs = new entry_ref [index];
				index=0;
				for( index=0; msg->FindPointer( MSG_MAILREF, index, (void**)&mailRef) == B_OK; ++index) {
					refs[index] = mailRef->EntryRef();
					mailRef->RemoveRef();	// msg is no more refering to mailRef
				}
				MoveToTrash( refs, index);
				delete [] refs;
			}
			case B_SILENT_RELAUNCH: {
				BM_LOG2( BM_LogAll, "App: silently relaunched");
				if (TheMainWindow->IsMinimized())
					TheMainWindow->Minimize( false);
				inherited::MessageReceived( msg);
				break;
			}
			case B_QUIT_REQUESTED: 
				BM_LOG2( BM_LogAll, "App: quit requested");
			default:
				inherited::MessageReceived( msg);
				break;
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BmApp: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	LaunchURL()
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::LaunchURL( const BString url) {
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
		msg.AddString( BmApplication::MSG_WHO_TO, urlStr+7);
		bmApp->PostMessage( &msg);
		return;
	}
	else result = B_ERROR;
	if (!(result == B_OK || result == B_ALREADY_RUNNING)) {
		(new BAlert( "", (BString("Could not launch application for url\n\t") 
								<< url << "\n\nError:\n\t" << strerror(result)).String(),
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
		"http://www.hirschkaefer.de/beam"
	);
	aboutWin->Show();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( )
		-	
\*------------------------------------------------------------------------------*/
BRect BmApplication::ScreenFrame() {
	BScreen screen;
	if (!screen.IsValid())
		BM_SHOWERR( BString("Could not initialize BScreen object !?!"));
	return screen.Frame();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::SetNewWorkspace( uint32 newWorkspace) {
	if (TheMainWindow->Workspaces() != newWorkspace)
		TheMainWindow->SetWorkspaces( newWorkspace);
	if (TheJobStatusWin->Workspaces() != newWorkspace)
		TheJobStatusWin->SetWorkspaces( newWorkspace);
}

/*------------------------------------------------------------------------------*\
	HandlesMimetype( )
		-	
\*------------------------------------------------------------------------------*/
bool BmApplication::HandlesMimetype( const BString mimetype) {
	return mimetype.ICompare( "text/x-email")==0 
			 || mimetype.ICompare( "message/rfc822")==0;
}
