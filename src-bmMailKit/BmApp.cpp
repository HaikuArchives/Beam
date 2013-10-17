/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <AppFileInfo.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFolderList.h"
#include "BmRecvAccount.h"
#include "BmPrefs.h"
#include "BmRoster.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	UpdateMimeTypeFile( sig, appModTime)
		-	checks age of our own mimetype-file 
			(.../settings/beos_mime/application/x-vnd.zooey-beam)
			and removes the file if it's older than the application file.
\*------------------------------------------------------------------------------*/
static void 
UpdateMimeTypeFile( const char* s, time_t appModTime) 
{
	BmString sig(s);
	BPath path;
	if (find_directory( B_SYSTEM_SETTINGS_DIRECTORY, &path) == B_OK) {
		sig.ToLower();
		BEntry mtEntry( (BmString(path.Path())<<"/beos_mime/"<<sig).String());
		if (mtEntry.InitCheck() == B_OK) {
			time_t modTime;
			if (mtEntry.GetModificationTime( &modTime) == B_OK) {
				if (appModTime > modTime) {
					// application is newer than mimetype-file, we simply remove
					// that and let BeOS recreate it when needed. The new version
					// will then contain all current icons, etc.
					mtEntry.Remove();
				}
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateRequiredIndices()
		-	
\*------------------------------------------------------------------------------*/
static void 
CreateRequiredIndices() 
{
	// TODO: actually no indices are *required* currently, they'll only
	// be required once Beam supports virtual folders.
	// For now, we only make sure that MAIL:status exists...
	EnsureIndexExists( BM_MAIL_ATTR_STATUS, B_STRING_TYPE);
	// ...and create indices for the Beam-only attributes:
	EnsureIndexExists( BM_MAIL_ATTR_ACCOUNT, B_STRING_TYPE);
	EnsureIndexExists( BM_MAIL_ATTR_IDENTITY, B_STRING_TYPE);
	EnsureIndexExists( BM_MAIL_ATTR_IMAP_UID, B_STRING_TYPE);

// [zooey]: Should we activate this? It's too ugly, isn't it?
/*
	// There's a bug in BFS which sometimes causes an index to be removed when
	// there's exactly one matching entry left. In order to avoid this, we
	// make sure we have at least two files (non-mails), that live on the mailbox-
	// volume:
	BmString mboxPath = ThePrefs->GetString( "MailboxPath");
	BDirectory mboxRoot( mboxPath.String());
	if (mboxRoot.InitCheck() == B_OK) {
		BmString txt(
			"This file is one in a set of two that have been created by Beam\n"
			"in order to circumvent a bug in BFS which may drop required indices\n"
			"if they only contain one single entry.\n"
		);
		for( int i=1; i<=2; ++i) {
			BmString nm("beam_mail_indices_anchor_");
			nm << i;
			BEntry entry( &mboxRoot, nm.String());
			if (entry.InitCheck() != B_OK || !entry.Exists()) {
				BFile f( &mboxRoot, nm.String(), B_WRITE_ONLY | B_CREATE_FILE);
				f.WriteAttr( BM_MAIL_ATTR_ACCOUNT, B_STRING_TYPE, 0, "", 1);
				f.WriteAttr( BM_MAIL_ATTR_IDENTITY, B_STRING_TYPE, 0, "", 1);
				f.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "", 1);
				f.Write( txt.String(), txt.Length());
			}
		}
	}
*/
}



int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig, bool testModeRequested)
	:	inherited( sig)
	,	mIsQuitting( false)
	,	mInitCheck( B_NO_INIT)
	,	mStartupLocker( new BLocker( "StartupLocker", false))
{
	if (InstanceCount > 0)
		throw BM_runtime_error( "Trying to initialize more than one instance "
										"of class BmApplication");

	// find out if we are running on Dano (or newer):
#ifndef __HAIKU__
	system_info sysInfo;
	get_system_info( &sysInfo);
	BmString kTimestamp(sysInfo.kernel_build_date);
	kTimestamp << " " << sysInfo.kernel_build_time;
	time_t kTime;
	ParseDateTime( kTimestamp, kTime);
	if (kTime >= 1005829579)
		BeamOnDano = true;
#endif

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
		// note if we are running in test-mode:
		if (testModeRequested)
			BeamInTestMode = true;
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

		// create the info-roster:
		BeamRoster = new BmRoster();
		time_t appModTime;
		appFile.GetModificationTime( &appModTime);
		UpdateMimeTypeFile( sig, appModTime);

		// load the preferences set by user (if any):
		BmPrefs::CreateInstance();

		// create most of our list-models:
		BmSignatureList::CreateInstance();

		BmFilterList::CreateInstance();

		BmFilterChainList::CreateInstance();

		BmMailFolderList::CreateInstance();

		BmIdentityList::CreateInstance();

		BmSmtpAccountList::CreateInstance();

		BmRecvAccountList::CreateInstance();

		mInitCheck = B_OK;
		InstanceCount++;
	} catch (BM_error& err) {
		BM_SHOWERR( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BmApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmApplication::~BmApplication() 
{
	TheSignatureList = NULL;
	TheIdentityList = NULL;
	TheSmtpAccountList = NULL;
	TheRecvAccountList = NULL;
	TheMailFolderList = NULL;
	TheFilterChainList = NULL;
	TheFilterList = NULL;

#ifdef BM_REF_DEBUGGING
	BmRefObj::PrintRefsLeft();
#endif
	BmRefObj::CleanupObjectLists();

	delete ThePrefs;
	BmLogHandler::Shutdown();
	delete TheLogHandler;
	delete mStartupLocker;
	delete BeamRoster;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	Run()
		-	starts Beam
\*------------------------------------------------------------------------------*/
thread_id 
BmApplication::Run() 
{
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		CreateRequiredIndices();

		if (BeamInTestMode)
			mStartupLocker->Unlock();
		tid = inherited::Run();

	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		exit(10);
	}
	return tid;
}
