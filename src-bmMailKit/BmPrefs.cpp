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

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, defaultVal prefs are used
\*------------------------------------------------------------------------------*/
BmPrefs* BmPrefs::CreateInstance() {
	BmPrefs *prefs = NULL;
	status_t err;
	BString prefsFilename;
	BFile prefsFile;

	if (theInstance) 
		return theInstance;

	// try to open settings-file...
	prefsFilename = BString( TheResources->SettingsPath.Path()) << "/" << PREFS_FILENAME;
	if ((err = prefsFile.SetTo( prefsFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, settings file found, we fetch our prefs from it:
		try {
			BMessage archive;
			(err = archive.Unflatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch settings from file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
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
{
	InitDefaults();
	mPrefsMsg = mDefaultsMsg;
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
	: BArchivable( archive)
{
	InitDefaults();
	mPrefsMsg = *archive;
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
							+ BM_LOGLVL_VAL(archive->FindInt16("Loglevel_PrefsWin"),BM_LogPrefsWin);
	mPrefsMsg.RemoveName("Loglevels");
	mPrefsMsg.AddInt32("Loglevels", loglevels);
	SetLoglevels();
	
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
	
	if (mPrefsMsg.FindMessage( "Shortcuts", &mShortcutsMsg) == B_OK) {
		// add any missing (new) shortcuts:
		GetShortcutDefaults( &mShortcutsMsg);
	} else {
		// no shortcuts info yet, we add default settings:
		mPrefsMsg.AddMessage( "Shortcuts", GetShortcutDefaults( &mShortcutsMsg));
	} 
}

/*------------------------------------------------------------------------------*\
	~BmPrefs()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPrefs::~BmPrefs() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	InitDefaults( )
		-	constructs a BMessage containing all defaultVal values
\*------------------------------------------------------------------------------*/
void BmPrefs::InitDefaults() {
	mDefaultsMsg.MakeEmpty();
	mDefaultsMsg.AddInt16( MSG_VERSION, nPrefsVersion);
	mDefaultsMsg.AddBool( "DynamicStatusWin", true);
	mDefaultsMsg.AddInt32( "ReceiveTimeout", 60);
	int32 loglevels = BM_LOGLVL2(BM_LogPop)
							+ BM_LOGLVL2(BM_LogJobWin) 
							+ BM_LOGLVL2(BM_LogMailParse) 
							+ BM_LOGLVL2(BM_LogUtil) 
							+ BM_LOGLVL2(BM_LogMailTracking)
							+ BM_LOGLVL2(BM_LogFolderView)
							+ BM_LOGLVL2(BM_LogRefView)
							+ BM_LOGLVL2(BM_LogMainWindow)
							+ BM_LOGLVL2(BM_LogModelController)
							+ BM_LOGLVL2(BM_LogMailEditWin)
							+ BM_LOGLVL2(BM_LogSmtp)
							+ BM_LOGLVL2(BM_LogPrefsWin);
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
	mDefaultsMsg.AddString( "MailboxPath", "/boot/home/mail");
	mDefaultsMsg.AddBool( "CacheRefsInMem", false);
	mDefaultsMsg.AddBool( "CacheRefsOnDisk", true);
	mDefaultsMsg.AddInt32( "DefaultEncoding", B_ISO1_CONVERSION);
	mDefaultsMsg.AddBool( "StripedListView", true);
	mDefaultsMsg.AddMessage( "MailRefLayout", new BMessage);
	mDefaultsMsg.AddBool( "RestoreFolderStates", true);
	mDefaultsMsg.AddBool( "ShowDecodedLength", true);
	mDefaultsMsg.AddBool( "GenerateOwnMessageIDs", true);
	mDefaultsMsg.AddString( "HeaderListLarge", "Subject,From,Date,To,Cc,User-Agent/X-Mailer");
	mDefaultsMsg.AddString( "HeaderListSmall", "Subject,From,Date");
	mDefaultsMsg.AddInt32( "MSecsBeforeMailMoverShows", 500*1000);
	mDefaultsMsg.AddInt32( "MSecsBeforePopperRemove", 5000*1000);
	mDefaultsMsg.AddInt32( "MSecsBeforeSmtpRemove", 0*1000);
	mDefaultsMsg.AddString( "QuotingString", "> ");
	mDefaultsMsg.AddInt32( "MaxLineLen", 76);
	mDefaultsMsg.AddInt32( "NetSendBufferSize", 10*1500);
	mDefaultsMsg.AddBool( "MakeQPSafeForEBCDIC", false);
	mDefaultsMsg.AddBool("SpecialHeaderForEachBcc", true);
	mDefaultsMsg.AddBool( "PreferUserAgentOverX-Mailer", true);
	mDefaultsMsg.AddInt32( "DefaultForwardType", BMM_FORWARD_INLINE);
	mDefaultsMsg.AddString( "ForwardIntroStr", "On %D at %T, %F wrote:");
	mDefaultsMsg.AddString( "ForwardSubjectRX", "^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	mDefaultsMsg.AddString( "ForwardSubjectStr", "Fwd: %s");
	mDefaultsMsg.AddBool( "DoNotAttachVCardsToForward", true);
	mDefaultsMsg.AddString( "ReplyIntroStr", "On %D at %T, %F wrote:");
	mDefaultsMsg.AddString( "ReplySubjectRX", "^\\s*(Re|Aw)(\\[\\d+\\])?:");
	mDefaultsMsg.AddString( "ReplySubjectStr", "Re: %s");
	mDefaultsMsg.AddString( "SignatureRX", "^---?\\s*\\n");
	mDefaultsMsg.AddString( "MimeTypeTrustInfo", "<application/pdf:T><application:W><:T>");
	mDefaultsMsg.AddBool( "InOutAlwaysAtTop", false);
	mDefaultsMsg.AddMessage( "Shortcuts", GetShortcutDefaults());
	mDefaultsMsg.AddString( "QuoteFormatting", "Push Margin");
	mDefaultsMsg.AddString( "QuotingLevelRX", "^((?:\\w?\\w?\\w?[>|]|[ \\t]*)*)(.*?)$");
	mDefaultsMsg.AddBool( "AutoCheckOnlyIfPPPRunning", true);
}

/*------------------------------------------------------------------------------*\
	GetShortcutFor( shortcutID)
		-	returns the shortcut for a given id (label):
\*------------------------------------------------------------------------------*/
BString BmPrefs::GetShortcutFor( const char* shortcutID) {
	const char* sc = mShortcutsMsg.FindString( shortcutID);
	return sc;
}

/*------------------------------------------------------------------------------*\
	SetShortcutIfNew()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetShortcutIfNew( BMessage* msg, const char* name, const BString val) {
	type_code tc;
	if (msg->GetInfo( name, &tc) != B_OK)
		msg->AddString( name, val);
}

/*------------------------------------------------------------------------------*\
	AddShortcutDefaults( msg)
		-	constructs a BMessage containing all shortcuts and adds that message to
			the given message:
\*------------------------------------------------------------------------------*/
BMessage* BmPrefs::GetShortcutDefaults( BMessage* shortcutsMsg) {
	if (!shortcutsMsg)
		shortcutsMsg = new BMessage;
	SetShortcutIfNew( shortcutsMsg, "About Beam...", "");
	SetShortcutIfNew( shortcutsMsg, "Apply Filter", "");
	SetShortcutIfNew( shortcutsMsg, "Check Mail", "M");
	SetShortcutIfNew( shortcutsMsg, "Check All Accounts", "<SHIFT>M");
	SetShortcutIfNew( shortcutsMsg, "Close", "W");
	SetShortcutIfNew( shortcutsMsg, "Copy", "C");
	SetShortcutIfNew( shortcutsMsg, "Cut", "X");
	SetShortcutIfNew( shortcutsMsg, "Delete Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Find...", "F");
	SetShortcutIfNew( shortcutsMsg, "Find Messages...", "<SHIFT>F");
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
	SetShortcutIfNew( shortcutsMsg, "Preferences...", "");
	SetShortcutIfNew( shortcutsMsg, "Print Message...", "");
	SetShortcutIfNew( shortcutsMsg, "Quit Beam", "Q");
	SetShortcutIfNew( shortcutsMsg, "Recache Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Redirect", "B");
	SetShortcutIfNew( shortcutsMsg, "Rename Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Reply", "R");
	SetShortcutIfNew( shortcutsMsg, "Reply To All", "<SHIFT>R");
	SetShortcutIfNew( shortcutsMsg, "SaveMail", "S");
	SetShortcutIfNew( shortcutsMsg, "Select All", "A");
	SetShortcutIfNew( shortcutsMsg, "Send Mail Now", "M");
	SetShortcutIfNew( shortcutsMsg, "Send Mail Later", "<SHIFT>M");
	SetShortcutIfNew( shortcutsMsg, "Send Pending Messages...", "");
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
	BString s;
	for( int i=31; i>=0; --i) {
		if (loglevels & (01UL<<i))
			s << "1";
		else
			s << "0";
	}
	BM_LOG3( BM_LogUtil, BString("Initialized loglevels to binary value ") << s);
#endif
}


/*------------------------------------------------------------------------------*\
	Store()
		-	stores preferences into global Settings-file:
\*------------------------------------------------------------------------------*/
bool BmPrefs::Store() {
	BFile prefsFile;
	status_t err;

	try {
		BString prefsFilename = BString( TheResources->SettingsPath.Path()) << "/" << PREFS_FILENAME;
		(err = prefsFile.SetTo( prefsFilename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create settings file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		// in order to avoid storing loglevels as plain value, we take it out temporarily:
		int32 loglevels = mPrefsMsg.FindInt32("Loglevels");
		mPrefsMsg.RemoveName("Loglevels");
		// update shortcuts:
		mPrefsMsg.RemoveName("Shortcuts");
		mPrefsMsg.AddMessage("Shortcuts", &mShortcutsMsg);
		// store prefs-data inside file:		
		(err = mPrefsMsg.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		// put loglevels back in:
		mPrefsMsg.AddInt32( "Loglevels", loglevels);
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	GetString()
		-	
\*------------------------------------------------------------------------------*/
BString BmPrefs::GetString( const char* name) {
	const char* val;
	if (mPrefsMsg.FindString( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindString( name, &val) == B_OK) {
			mPrefsMsg.AddString( name, val);
			return val;
		} else {
			BM_SHOWERR( BString("The Preferences-field ") << name << " of type string is unknown");
			return "";
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetString()
		-	
\*------------------------------------------------------------------------------*/
BString BmPrefs::GetString( const char* name, const BString defaultVal) {
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
	GetBool()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefs::GetBool( const char* name) {
	bool val;
	if (mPrefsMsg.FindBool( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindBool( name, &val) == B_OK) {
			mPrefsMsg.AddBool( name, val);
			return val;
		} else {
			BM_SHOWERR( BString("The Preferences-field ") << name << " of type bool is unknown");
			return false;
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetBool()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefs::GetBool( const char* name, const bool defaultVal) {
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
	Get()
		-	
\*------------------------------------------------------------------------------*/
int32 BmPrefs::GetInt( const char* name) {
	int32 val;
	if (mPrefsMsg.FindInt32( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindInt32( name, &val) == B_OK) {
			mPrefsMsg.AddInt32( name, val);
			return val;
		} else {
			BM_SHOWERR( BString("The Preferences-field ") << name << " of type int32 is unknown");
			return 0;
		}
	}
}

/*------------------------------------------------------------------------------*\
	Get()
		-	
\*------------------------------------------------------------------------------*/
int32 BmPrefs::GetInt( const char* name, const int32 defaultVal) {
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
	GetMsg()
		-	
\*------------------------------------------------------------------------------*/
const BMessage* BmPrefs::GetMsg( const char* name) {
	BMessage* msg = mMsgCache[name];
	if (msg)
		return msg;
	msg = new BMessage();
	if (mPrefsMsg.FindMessage( name, msg) == B_OK) {
		mMsgCache[name] = msg;
		return msg;
	} else {
		if (mDefaultsMsg.FindMessage( name, msg) == B_OK) {
			mPrefsMsg.AddMessage( name, msg);
			return msg;
		} else {
			BM_SHOWERR( BString("The Preferences-field ") << name << " of type message is unknown");
			return msg;
		}
	}
}

/*------------------------------------------------------------------------------*\
	GetMsg()
		-	
\*------------------------------------------------------------------------------*/
const BMessage* BmPrefs::GetMsg( const char* name, const BMessage* defaultVal) {
	BMessage* msg = mMsgCache[name];
	if (msg)
		return msg;
	msg = new BMessage();
	if (mPrefsMsg.FindMessage( name, msg) == B_OK) {
		mMsgCache[name] = msg;
		return msg;
	} else {
		if (mDefaultsMsg.FindMessage( name, msg) == B_OK) {
			mPrefsMsg.AddMessage( name, msg);
			return msg;
		} else
			return defaultVal;
	}
}

/*------------------------------------------------------------------------------*\
	SetBool()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetBool( const char* name, const bool val) {
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddBool( name, val);
}

/*------------------------------------------------------------------------------*\
	SetInt()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetInt( const char* name, const int32 val) {
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddInt32( name, val);
}

/*------------------------------------------------------------------------------*\
	SetMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetMsg( const char* name, const BMessage* val) {
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddMessage( name, val);
}

/*------------------------------------------------------------------------------*\
	SetString()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetString( const char* name, const BString val) {
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddString( name, val.String());
}
