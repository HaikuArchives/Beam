/*
	BmPrefs.cpp

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


#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <UTF8.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"


BmPrefs* BmPrefs::theInstance = NULL;

const char* const BmPrefs::PREFS_FILENAME = 	"General Settings";
const char* const BmPrefs::MSG_VERSION	 = 	"bm:version";

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, defaultVal prefs are used
\*------------------------------------------------------------------------------*/
BmPrefs* BmPrefs::CreateInstance() {
	BmPrefs *prefs = NULL;
	status_t err;
	BmString prefsFilename;
	BFile prefsFile;

	if (theInstance) 
		return theInstance;

	// try to open settings-file...
	prefsFilename = BmString( TheResources->SettingsPath.Path()) << "/" << PREFS_FILENAME;
	if ((err = prefsFile.SetTo( prefsFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, settings file found, we fetch our prefs from it:
		try {
			BMessage archive;
			(err = archive.Unflatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Could not fetch settings from file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
			prefs = new BmPrefs( &archive);
		} catch (exception &e) {
			BM_SHOWERR( e.what());
			prefs = NULL;
		}
	}
	if (!prefs) {
		// ...no settings file yet, we start with defaultVal settings...
		prefs = new BmPrefs;
		// ...and create a new and shiny settings file:
		create_directory( TheResources->SettingsPath.Path(), 0755);
		prefs->Store();
	}

	theInstance = prefs;
	return prefs;
}

/*------------------------------------------------------------------------------*\
	BmPrefs()
		-	defaultVal constructor
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( void)
	:	BArchivable() 
	,	mLocker( "PrefsLock")
{
	InitDefaults();
	mSavedPrefsMsg = mPrefsMsg = mDefaultsMsg;
	SetLoglevels();
	if (mPrefsMsg.FindMessage( "Shortcuts", &mShortcutsMsg) != B_OK)
		BM_SHOWERR("Prefs: Could not access shortcut info!");
}


/*------------------------------------------------------------------------------*\
	BmPrefs( archive)
		-	constructs a BmPrefs from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( BMessage* archive) 
	:	BArchivable( archive)
	,	mLocker( "PrefsLock")
{
	InitDefaults();
	mSavedPrefsMsg = mPrefsMsg = *archive;
	int16 version = 0;
	archive->FindInt16( MSG_VERSION, &version);
	int32 loglevels = BM_LOGLVL_VAL(archive->FindInt16("Loglevel_Pop"),BM_LogPop)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_JobWin"),BM_LogJobWin) 
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_MailParse"),BM_LogMailParse) 
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_Util"),BM_LogUtil) 
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_MailTracking"),BM_LogMailTracking)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_FolderView"),BM_LogFolderView)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_RefView"),BM_LogRefView)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_MainWindow"),BM_LogMainWindow)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_ModelController"),BM_LogModelController)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_MailEditWin"),BM_LogMailEditWin)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_Smtp"),BM_LogSmtp)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_PrefsWin"),BM_LogPrefsWin)
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_Filter"),BM_LogFilter);
	mPrefsMsg.RemoveName("Loglevels");
	mPrefsMsg.AddInt32("Loglevels", loglevels);
	SetLoglevels();
	
	status_t scStatus = mPrefsMsg.FindMessage( "Shortcuts", &mShortcutsMsg);
	if (version < 1) {
		// changes introduced with version 1:
		//
		// change default value of SignatureRX:
		mPrefsMsg.RemoveName("SignatureRX");
		mPrefsMsg.AddString( "SignatureRX", mDefaultsMsg.FindString( "SignatureRX"));
		mPrefsMsg.AddInt16( MSG_VERSION, nPrefsVersion);
		// remove BeMailStyle-flag, since we now have configurable shortcuts:
		mPrefsMsg.RemoveName("BeMailStyle");
	}
	if (version < 2) {
		// changes introduced with version 2:
		//
		// nothing to do...
	}
	if (version < 3 && scStatus==B_OK) {
		// changes introduced with version 3:
		//
		// remove trailing dots from shortcuts, if present:
		BMessage scMsg( mShortcutsMsg);
#ifdef B_BEOS_VERSION_DANO
		const char* name;
#else
		char* name;
#endif
		uint32 type;
		int32 pos=-1;
		for( int32 i=0; scMsg.GetInfo( B_STRING_TYPE, i, &name, &type)==B_OK; ++i) {
			BmString scName( name);
			if ((pos=scName.FindFirst( "...")) != B_ERROR) {
				BmString val = mShortcutsMsg.FindString( scName.String());
				mShortcutsMsg.RemoveName( scName.String());
				scName.Truncate( pos);
				mShortcutsMsg.AddString( scName.String(), val.String());
			}
		}
	}
	if (version < 4) {
		// changes introduced with version 4:
		//
		// replace int-field "" with corresponding string-field:
		mPrefsMsg.RemoveName("DefaultForwardType");
		mPrefsMsg.AddString( "DefaultForwardType", 
									mDefaultsMsg.FindString( "DefaultForwardType"));
	}
	if (scStatus == B_OK) {
		// add any missing (new) shortcuts:
		GetShortcutDefaults( &mShortcutsMsg);
	} else {
		// no shortcuts info yet, we add default settings:
		mPrefsMsg.AddMessage( "Shortcuts", GetShortcutDefaults( &mShortcutsMsg));
	}
}

/*------------------------------------------------------------------------------*\
	~BmPrefs()
		-	d'tor
		-	removes singleton
		-	deletes all BMessages contained in msg-cache
\*------------------------------------------------------------------------------*/
BmPrefs::~BmPrefs() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	ResetToSaved()
		-	resets preferences to last saved state
\*------------------------------------------------------------------------------*/
void BmPrefs::ResetToSaved() {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg = mSavedPrefsMsg;
	if (mPrefsMsg.FindMessage( "Shortcuts", &mShortcutsMsg) == B_OK) {
		// add any missing (new) shortcuts:
		GetShortcutDefaults( &mShortcutsMsg);
	} else {
		// no shortcuts info yet, we add default settings:
		mPrefsMsg.AddMessage( "Shortcuts", GetShortcutDefaults( &mShortcutsMsg));
	} 
}

/*------------------------------------------------------------------------------*\
	ResetToDefault()
		-	resets preferences to default state
\*------------------------------------------------------------------------------*/
void BmPrefs::ResetToDefault() {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg = mDefaultsMsg;
	mShortcutsMsg.MakeEmpty();
	GetShortcutDefaults( &mShortcutsMsg);
}

/*------------------------------------------------------------------------------*\
	InitDefaults( )
		-	constructs a BMessage containing all defaultVal values
\*------------------------------------------------------------------------------*/
void BmPrefs::InitDefaults() {
	mDefaultsMsg.MakeEmpty();
	mDefaultsMsg.AddInt16( MSG_VERSION, nPrefsVersion);
	int32 loglevels = BM_LOGLVL2(BM_LogPop)
							+ BM_LOGLVL2(BM_LogJobWin) 
							+ BM_LOGLVL2(BM_LogMailParse) 
							+ BM_LOGLVL2(BM_LogUtil) 
							+ BM_LOGLVL0(BM_LogMailTracking)
							+ BM_LOGLVL2(BM_LogFolderView)
							+ BM_LOGLVL0(BM_LogRefView)
							+ BM_LOGLVL0(BM_LogMainWindow)
							+ BM_LOGLVL0(BM_LogModelController)
							+ BM_LOGLVL0(BM_LogMailEditWin)
							+ BM_LOGLVL2(BM_LogSmtp)
							+ BM_LOGLVL2(BM_LogPrefsWin)
							+ BM_LOGLVL2(BM_LogFilter);

	mDefaultsMsg.AddBool( "AutoCheckOnlyIfPPPRunning", true);
	mDefaultsMsg.AddBool( "Allow8BitMime", false);
	mDefaultsMsg.AddBool( "BeepWhenNewMailArrived", true);
	mDefaultsMsg.AddBool( "CacheRefsInMem", false);
	mDefaultsMsg.AddBool( "CacheRefsOnDisk", true);
	mDefaultsMsg.AddInt32( "DefaultEncoding", B_ISO1_CONVERSION);
	mDefaultsMsg.AddString( "DefaultForwardType", "Inline");
	mDefaultsMsg.AddBool( "DoNotAttachVCardsToForward", true);
	mDefaultsMsg.AddBool( "DynamicStatusWin", true);
	mDefaultsMsg.AddString( "ForwardIntroStr", "On %D at %T, %F wrote:");
	mDefaultsMsg.AddString( "ForwardSubjectRX", "^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	mDefaultsMsg.AddString( "ForwardSubjectStr", "Fwd: %s");
	mDefaultsMsg.AddBool( "GenerateOwnMessageIDs", true);
	mDefaultsMsg.AddBool( "HardWrapMailText", true);
	mDefaultsMsg.AddString( "HeaderListLarge", "Subject,From,Date,To,Cc,User-Agent/X-Mailer");
	mDefaultsMsg.AddString( "HeaderListSmall", "Subject,From,Date");
	mDefaultsMsg.AddBool( "InOutAlwaysAtTop", true);
	mDefaultsMsg.AddInt32( "Loglevels", loglevels);
	mDefaultsMsg.AddInt16( "Loglevel_Pop", BM_LOGLVL_FOR(loglevels,BM_LogPop));
	mDefaultsMsg.AddInt16( "Loglevel_JobWin", BM_LOGLVL_FOR(loglevels,BM_LogJobWin));
	mDefaultsMsg.AddInt16( "Loglevel_MailParse", BM_LOGLVL_FOR(loglevels,BM_LogMailParse));
	mDefaultsMsg.AddInt16( "Loglevel_Util", BM_LOGLVL_FOR(loglevels,BM_LogUtil));
	mDefaultsMsg.AddInt16( "Loglevel_MailTracking", BM_LOGLVL_FOR(loglevels,BM_LogMailTracking));
	mDefaultsMsg.AddInt16( "Loglevel_FolderView", BM_LOGLVL_FOR(loglevels,BM_LogFolderView));
	mDefaultsMsg.AddInt16( "Loglevel_RefView", BM_LOGLVL_FOR(loglevels,BM_LogRefView));
	mDefaultsMsg.AddInt16( "Loglevel_MainWindow", BM_LOGLVL_FOR(loglevels,BM_LogMainWindow));
	mDefaultsMsg.AddInt16( "Loglevel_ModelController", BM_LOGLVL_FOR(loglevels,BM_LogModelController));
	mDefaultsMsg.AddInt16( "Loglevel_MailEditWin", BM_LOGLVL_FOR(loglevels,BM_LogMailEditWin));
	mDefaultsMsg.AddInt16( "Loglevel_Smtp", BM_LOGLVL_FOR(loglevels,BM_LogSmtp));
	mDefaultsMsg.AddInt16( "Loglevel_PrefsWin", BM_LOGLVL_FOR(loglevels,BM_LogPrefsWin));
	mDefaultsMsg.AddMessage( "MailRefLayout", new BMessage);
	mDefaultsMsg.AddString( "MailboxPath", "/boot/home/mail");
	mDefaultsMsg.AddBool( "MakeQPSafeForEBCDIC", false);
	mDefaultsMsg.AddInt32( "MarkAsReadDelay", 500);
	mDefaultsMsg.AddInt32( "MaxLineLen", 76);
	mDefaultsMsg.AddInt32( "MaxLineLenForHardWrap", 998);
	mDefaultsMsg.AddString( "MimeTypeTrustInfo", "<application/pdf:T><application/zip:T><application:W><:T>");
	mDefaultsMsg.AddInt32( "MSecsBeforeMailMoverShows", 500*1000);
	mDefaultsMsg.AddInt32( "MSecsBeforePopperRemove", 5000*1000);
	mDefaultsMsg.AddInt32( "MSecsBeforeSmtpRemove", 0*1000);
	mDefaultsMsg.AddInt32( "NetReceiveBufferSize", 65536);
	mDefaultsMsg.AddInt32( "NetSendBufferSize", 65536);
	mDefaultsMsg.AddString( "QuoteFormatting", "Push Margin");
	mDefaultsMsg.AddString( "QuotingLevelRX", "^((?:\\w?\\w?\\w?[>|]|[ \\t]*)*)(.*?)$");
	mDefaultsMsg.AddString( "QuotingString", "> ");
	mDefaultsMsg.AddBool( "PreferUserAgentOverX-Mailer", true);
	mDefaultsMsg.AddInt32( "ReceiveTimeout", 60);
	mDefaultsMsg.AddString( "ReplyIntroStr", "On %D at %T, you wrote:");
	mDefaultsMsg.AddString( "ReplyListIntroStr", "On %D at %T, %F wrote:");
	mDefaultsMsg.AddString( "ReplySubjectRX", "^\\s*(Re|Aw)(\\[\\d+\\])?:");
	mDefaultsMsg.AddString( "ReplySubjectStr", "Re: %s");
	mDefaultsMsg.AddBool( "RestoreFolderStates", true);
	mDefaultsMsg.AddMessage( "Shortcuts", GetShortcutDefaults());
	mDefaultsMsg.AddBool( "ShowDecodedLength", true);
	mDefaultsMsg.AddBool( "ShowToolbarBorder", false);
	mDefaultsMsg.AddBool( "ShowToolbarIcons", true);
	mDefaultsMsg.AddString( "ShowToolbarLabel", "Right");
	mDefaultsMsg.AddString( "SignatureRX", "^---?\\s*\\n");
	mDefaultsMsg.AddBool("SpecialHeaderForEachBcc", true);
	mDefaultsMsg.AddBool( "StripedListView", true);
	mDefaultsMsg.AddString( "TimeModeInHeaderView", "Local");
	mDefaultsMsg.AddBool( "UseDeskbar", true);
	mDefaultsMsg.AddBool( "UseDocumentResizer", true);
	mDefaultsMsg.AddBool( "UseSwatchTimeInRefView", false);
	mDefaultsMsg.AddString( "Workspace", "Current");
}

/*------------------------------------------------------------------------------*\
	GetShortcutFor( shortcutID)
		-	returns the shortcut for a given id (label):
\*------------------------------------------------------------------------------*/
BmString BmPrefs::GetShortcutFor( const char* shortcutID) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	const char* sc = mShortcutsMsg.FindString( shortcutID);
	return sc;
}

/*------------------------------------------------------------------------------*\
	SetShortcutIfNew( msg, name, val)
		-	adds a shortcut to the given message if that does not yet have it
		-	this method is used to copy new shortcuts over from the defaults-msg
			to the current prefs-message
\*------------------------------------------------------------------------------*/
void BmPrefs::SetShortcutIfNew( BMessage* msg, const char* name, const BmString val) {
	type_code tc;
	if (msg->GetInfo( name, &tc) != B_OK)
		msg->AddString( name, val.String());
}

/*------------------------------------------------------------------------------*\
	SetShortcutFor( name, val)
		-	updates a shortcut to the given value
\*------------------------------------------------------------------------------*/
void BmPrefs::SetShortcutFor( const char* name, const BmString val) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mShortcutsMsg.RemoveName( name);
	mShortcutsMsg.AddString( name, val.String());
}

/*------------------------------------------------------------------------------*\
	AddShortcutDefaults( msg)
		-	constructs a BMessage containing all shortcuts and adds that message to
			the given message:
\*------------------------------------------------------------------------------*/
BMessage* BmPrefs::GetShortcutDefaults( BMessage* shortcutsMsg) {
	if (!shortcutsMsg)
		shortcutsMsg = new BMessage;
	SetShortcutIfNew( shortcutsMsg, "About Beam", "");
	SetShortcutIfNew( shortcutsMsg, "Apply Filter", "");
	SetShortcutIfNew( shortcutsMsg, "Check Mail", "M");
	SetShortcutIfNew( shortcutsMsg, "Check All Accounts", "<SHIFT>M");
	SetShortcutIfNew( shortcutsMsg, "Close", "W");
	SetShortcutIfNew( shortcutsMsg, "Copy", "C");
	SetShortcutIfNew( shortcutsMsg, "Cut", "X");
	SetShortcutIfNew( shortcutsMsg, "Delete Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Find", "F");
	SetShortcutIfNew( shortcutsMsg, "Find Messages", "<SHIFT>F");
	SetShortcutIfNew( shortcutsMsg, "Find Next", "G");
	SetShortcutIfNew( shortcutsMsg, "Forward As Attachment", "<SHIFT>J");
	SetShortcutIfNew( shortcutsMsg, "Forward Inline", "J");
	SetShortcutIfNew( shortcutsMsg, "Forward Inline (With Attachments)", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsDraft", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsForwarded", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsNew", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsPending", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsRead", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsRedirected", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsReplied", "");
	SetShortcutIfNew( shortcutsMsg, "MarkAsSent", "");
	SetShortcutIfNew( shortcutsMsg, "Move To Trash", "T");
	SetShortcutIfNew( shortcutsMsg, "New Folder", "");
	SetShortcutIfNew( shortcutsMsg, "New Message", "N");
	SetShortcutIfNew( shortcutsMsg, "Page Setup", "");
	SetShortcutIfNew( shortcutsMsg, "Paste", "V");
	SetShortcutIfNew( shortcutsMsg, "Preferences", "<SHIFT>P");
	SetShortcutIfNew( shortcutsMsg, "Print Message", "");
	SetShortcutIfNew( shortcutsMsg, "Quit Beam", "Q");
	SetShortcutIfNew( shortcutsMsg, "Recache Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Redirect", "B");
	SetShortcutIfNew( shortcutsMsg, "Redo", "<SHIFT>Z");
	SetShortcutIfNew( shortcutsMsg, "Rename Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Reply", "R");
	SetShortcutIfNew( shortcutsMsg, "Reply To List", "");
	SetShortcutIfNew( shortcutsMsg, "Reply To Originator", "");
	SetShortcutIfNew( shortcutsMsg, "Reply To All", "<SHIFT>R");
	SetShortcutIfNew( shortcutsMsg, "SaveMail", "S");
	SetShortcutIfNew( shortcutsMsg, "Select All", "A");
	SetShortcutIfNew( shortcutsMsg, "Send Mail Now", "M");
	SetShortcutIfNew( shortcutsMsg, "Send Mail Later", "<SHIFT>M");
	SetShortcutIfNew( shortcutsMsg, "Send Pending Messages", "");
	SetShortcutIfNew( shortcutsMsg, "Show Raw Message", "<SHIFT>H");
	SetShortcutIfNew( shortcutsMsg, "Toggle Header Mode", "H");
	SetShortcutIfNew( shortcutsMsg, "Undo", "Z");
	return shortcutsMsg;
}

/*------------------------------------------------------------------------------*\
	SetLoglevels( )
		-	constructs a BMessage containing all defaultVal values
\*------------------------------------------------------------------------------*/
void BmPrefs::SetLoglevels() {
	// transfer loglevel-definitions to log-handler:
	int32 loglevels = GetInt("Loglevels");
	TheLogHandler->LogLevels( loglevels);
#ifdef BM_LOGGING
	BmString s;
	for( int i=31; i>=0; --i) {
		if (loglevels & (01UL<<i))
			s << "1";
		else
			s << "0";
	}
	BM_LOG3( BM_LogUtil, BmString("Initialized loglevels to binary value ") << s);
#endif
}


/*------------------------------------------------------------------------------*\
	Store()
		-	stores preferences into global Settings-file:
\*------------------------------------------------------------------------------*/
bool BmPrefs::Store() {
	BFile prefsFile;
	status_t err;
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");

	try {
		BmString prefsFilename = BmString( TheResources->SettingsPath.Path()) << "/" << PREFS_FILENAME;
		(err = prefsFile.SetTo( prefsFilename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Could not create settings file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		// in order to avoid storing loglevels as plain value, we take it out temporarily:
		int32 loglevels = mPrefsMsg.FindInt32("Loglevels");
		mPrefsMsg.RemoveName("Loglevels");
		// update shortcuts:
		mPrefsMsg.RemoveName("Shortcuts");
		mPrefsMsg.AddMessage("Shortcuts", &mShortcutsMsg);
		// store prefs-data inside file:		
		(err = mPrefsMsg.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Could not store settings into file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		// put loglevels back in:
		mPrefsMsg.AddInt32( "Loglevels", loglevels);
		// update saved state to current:
		mSavedPrefsMsg = mPrefsMsg;
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	GetString( name)
		-	returns the prefs-value (a string) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, an error message is shown
\*------------------------------------------------------------------------------*/
BmString BmPrefs::GetString( const char* name) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	const char* val;
	if (mPrefsMsg.FindString( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindString( name, &val) == B_OK) {
			mPrefsMsg.AddString( name, val);
			return val;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name << " of type string is unknown");
			return "";
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetString( name, defaultVal)
		-	returns the prefs-value (a string) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, the given default-value is returned
\*------------------------------------------------------------------------------*/
BmString BmPrefs::GetString( const char* name, const BmString defaultVal) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	const char* val;
	if (mPrefsMsg.FindString( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindString( name, &val) == B_OK) {
			mPrefsMsg.AddString( name, val);
			return val;
		} else
			return defaultVal;
	}
}

/*------------------------------------------------------------------------------*\
	GetBool( name)
		-	returns the prefs-value (a boolean) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, an error message is shown
\*------------------------------------------------------------------------------*/
bool BmPrefs::GetBool( const char* name) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	bool val;
	if (mPrefsMsg.FindBool( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindBool( name, &val) == B_OK) {
			mPrefsMsg.AddBool( name, val);
			return val;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name << " of type bool is unknown");
			return false;
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetBool( name, defaultVal)
		-	returns the prefs-value (a boolean) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, the given default-value is returned
\*------------------------------------------------------------------------------*/
bool BmPrefs::GetBool( const char* name, const bool defaultVal) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	bool val;
	if (mPrefsMsg.FindBool( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindBool( name, &val) == B_OK) {
			mPrefsMsg.AddBool( name, val);
			return val;
		} else
			return defaultVal;
	}
}

/*------------------------------------------------------------------------------*\
	GetInt( name)
		-	returns the prefs-value (an integer) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, an error message is shown
\*------------------------------------------------------------------------------*/
int32 BmPrefs::GetInt( const char* name) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	int32 val;
	if (mPrefsMsg.FindInt32( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindInt32( name, &val) == B_OK) {
			mPrefsMsg.AddInt32( name, val);
			return val;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name << " of type int32 is unknown");
			return 0;
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetInt( name, defaultVal)
		-	returns the prefs-value (an integer) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, the given default-value is returned
\*------------------------------------------------------------------------------*/
int32 BmPrefs::GetInt( const char* name, const int32 defaultVal) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	int32 val;
	if (mPrefsMsg.FindInt32( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindInt32( name, &val) == B_OK) {
			mPrefsMsg.AddInt32( name, val);
			return val;
		} else
			return defaultVal;
	}
}

/*------------------------------------------------------------------------------*\
	GetMsg( name)
		-	returns the prefs-value (a BMessage*) for the given name
		-	if the current prefs do not contain such a value, the value is copied
			over from the defaults-msg
		-	if neither the current prefs nor the defaults-msg contain the specified
			value, an error message is shown
		-	N.B.: The BMessage belongs to the caller, so delete it if you are finished
\*------------------------------------------------------------------------------*/
BMessage* BmPrefs::GetMsg( const char* name) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	BMessage* msg = new BMessage();
	if (mPrefsMsg.FindMessage( name, msg) == B_OK) {
		return msg;
	} else {
		if (mDefaultsMsg.FindMessage( name, msg) == B_OK) {
			mPrefsMsg.AddMessage( name, msg);
			return msg;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name << " of type message is unknown");
			delete msg;
			return NULL;
		}
	}
}

/*------------------------------------------------------------------------------*\
	SetBool( name, val)
		-	sets the prefs-val (a boolean) specified by name to the given val
		-	if a value for the given name exists already, it is replaced by the 
			new value
\*------------------------------------------------------------------------------*/
void BmPrefs::SetBool( const char* name, const bool val) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddBool( name, val);
}

/*------------------------------------------------------------------------------*\
	SetInt()
		-	sets the prefs-val (an integer) specified by name to the given val
		-	if a value for the given name exists already, it is replaced by the 
			new value
\*------------------------------------------------------------------------------*/
void BmPrefs::SetInt( const char* name, const int32 val) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddInt32( name, val);
}

/*------------------------------------------------------------------------------*\
	SetMsg()
		-	sets the prefs-val (a BMessage*) specified by name to the given val
		-	if a value for the given name exists already, it is replaced by the 
			new value
		-	the prefs-object copies the given message, so the caller can delete it
\*------------------------------------------------------------------------------*/
void BmPrefs::SetMsg( const char* name, const BMessage* val) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddMessage( name, val);
}

/*------------------------------------------------------------------------------*\
	SetString()
		-	sets the prefs-val (a BmString) specified by name to the given val
		-	if a value for the given name exists already, it is replaced by the 
			new value
\*------------------------------------------------------------------------------*/
void BmPrefs::SetString( const char* name, const BmString val) {
	BmAutolock lock( mLocker);
	lock.IsLocked()	 						|| BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddString( name, val.String());
}
