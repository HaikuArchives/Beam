/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Path.h>
#include <UTF8.h>

#include "BmBasics.h"
#include "BmEncoding.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"


BmPrefs* BmPrefs::theInstance = NULL;

const char* const BmPrefs::PREFS_FILENAME = 	"General Settings";
const char* const BmPrefs::MSG_VERSION	 = 	"bm:version";

const char* const BmPrefs::LOG_LVL_0 = "Don't log";
const char* const BmPrefs::LOG_LVL_1 = "Log";
const char* const BmPrefs::LOG_LVL_2 = "Log more";
const char* const BmPrefs::LOG_LVL_3 = "Log everything";

const BmString BmPrefs::nListSeparator = ",";

const BmString BmPrefs::nDefaultIconset = "/Icons/iconset 22 nuvola grey-red";
const int16 BmPrefs::nPrefsVersion = 13;

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
	prefsFilename 
		= BmString( BeamRoster->SettingsPath()) << "/" << PREFS_FILENAME;
	if ((err = prefsFile.SetTo( prefsFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, settings file found, we fetch our prefs from it:
		try {
			BMessage archive;
			InitDefaults(archive);
			char name[B_ATTR_NAME_LENGTH];
			int bufSize = 4096;
			char* buf = (char*)malloc(bufSize);
			if (!buf)
				BM_THROW_RUNTIME( 
					BmString("Could not alloc attribute buffer for prefs-file\n\t<")
						<< prefsFilename << ">\n"
				);
			
			attr_info info;
			int count = 0;
			// fetch settings from attributes:
			while (prefsFile.GetNextAttrName(name) == B_OK) {
				if (strncmp(name, "BEOS:", 5) == 0)
					continue;
				err = prefsFile.GetAttrInfo(name, &info);
				if (err == B_OK) {
					if (bufSize < info.size)
						buf = (char*)realloc(buf, info.size);
					err = prefsFile.ReadAttr(name, info.type, 0, buf, info.size);
				}
				if (err > 0) {
					archive.RemoveName(name);
					err = archive.AddData(name, info.type, buf, info.size);
				}
				if (err != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not fetch setting <")
							<< name << "> from file\n\t<" 
							<< prefsFilename << ">\n\n Result: " << strerror(err)
					);
				count++;
			}
			free(buf);

			if (!count) {
				// probably old format (flattened message), so we fall back:
				if ((err = archive.Unflatten( &prefsFile)) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not fetch settings from file\n\t<") 
							<< prefsFilename << ">\n\n Result: " << strerror(err)
					);
			}

			prefs = new BmPrefs( &archive);
		} catch (BM_error &e) {
			BM_SHOWERR( e.what());
			prefs = NULL;
		}
	}
	if (!prefs) {
		// ...no settings file yet, we start with defaultVal settings...
		prefs = new BmPrefs;
		// ...and create a new and shiny settings file:
		create_directory( BeamRoster->SettingsPath(), 0755);
		prefs->Store();
	}

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
	theInstance = this;
	InitDefaults(mDefaultsMsg);
	mSavedPrefsMsg = mPrefsMsg = mDefaultsMsg;
	SetLoglevels();
	SetupMailboxVolume();
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
	theInstance = this;
	InitDefaults(mDefaultsMsg);
	mPrefsMsg = *archive;
	int16 version = 0;
	archive->FindInt16( MSG_VERSION, &version);

	status_t scStatus = mPrefsMsg.FindMessage( "Shortcuts", &mShortcutsMsg);
	if (version < 1) {
		// changes introduced with version 1:
		//
		// change default value of SignatureRX:
		mPrefsMsg.RemoveName("SignatureRX");
		mPrefsMsg.AddString( "SignatureRX", 
									mDefaultsMsg.FindString( "SignatureRX"));
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
		for( 	int32 i=0; 
				scMsg.GetInfo( B_STRING_TYPE, i, &name, &type)==B_OK; ++i) {
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
		// replace int-field "DefaultForwardType" with corresponding string-field:
		mPrefsMsg.RemoveName("DefaultForwardType");
		mPrefsMsg.AddString( "DefaultForwardType", 
									mDefaultsMsg.FindString( "DefaultForwardType"));
	}
	if (version < 5) {
		// changes introduced with version 5:
		//
		// replace int-field "DefaultEncoding" with corresponding string-field:
		int32 encoding = mPrefsMsg.FindInt32( "DefaultEncoding");
		BmString charset = BmEncoding::ConvertFromBeosToLibiconv( encoding);
		mPrefsMsg.RemoveName("DefaultEncoding");
		mPrefsMsg.AddString( "DefaultCharset", charset.String());
	}
	if (version < 6) {
		// changes introduced with version 6:
		//
		// remove field "SeparatorCharsForUndo" (is now called "UndoMode"):
		mPrefsMsg.RemoveName("SeparatorCharsForUndo");
	}
	if (version < 7) {
		// changes introduced with version 7:
		//
		// replace int16-loglevels by int32-versions:
		const char* ll[] = {
			"Loglevel_Pop", 
			"Loglevel_JobWin", 
			"Loglevel_MailParse",
			"Loglevel_App",
			"Loglevel_MailTracking",
			"Loglevel_Gui",
			"Loglevel_ModelController",
			"Loglevel_Smtp",
			"Loglevel_Filter",
			"Loglevel_RefCount",
			"Loglevel_Util",
			"Loglevel_FolderView",
			"Loglevel_RefView",
			"Loglevel_MailEditWin",
			NULL
		};
		for( int i=0; ll[i]; ++i) {
			mPrefsMsg.RemoveName( ll[i]);
			int32 level = 0;
			if (mDefaultsMsg.FindInt32( ll[i], &level) == B_OK)
				mPrefsMsg.AddInt32( ll[i], level);
		}
		// remove multiple default-charset entries:
		BmString defCharset = mPrefsMsg.FindString( "DefaultCharset");
		if (!defCharset.Length())
			defCharset = mDefaultsMsg.FindString( "DefaultCharset");
		mPrefsMsg.RemoveName( "DefaultCharset");
		mPrefsMsg.AddString( "DefaultCharset", defCharset.String());
	}
	if (version < 8) {
		// changes introduced with version 8:
		//
		// replace field "MaxLineLenForHardWrap" by "NeverExceed78Chars":
		int32 mlen = mPrefsMsg.FindInt32( "MaxLineLenForHardWrap");
		mPrefsMsg.RemoveName( "MaxLineLenForHardWrap");
		mPrefsMsg.AddBool( "NeverExceed78Chars", mlen < 100);
	}
	if (version < 10) {
		// changes introduced with version 10:
		//
		// add list-fields:
		mPrefsMsg.AddString( "ListFields", "Mail-Followup-To,Reply-To");
	}
	if (version < 11) {
		// changes introduced with version 11:
		//
		// replace <>-separators by simple commas:
		const char *fields[] = { "MimeTypeTrustInfo", "StandardCharsets", NULL };
		BmString s;
		for( const char** f = fields; *f; f++) {
			BmString s = mPrefsMsg.FindString( *f);
			s.RemoveAll( "<");
			s.ReplaceAll( ">", ",");
			s.RemoveLast( ",");
			mPrefsMsg.ReplaceString( *f, s.String());
		}
	}
	if (version < 12) {
		// changes introduced with version 12:
		//
		// remove unneeded accumulated loglevels entry:
		mPrefsMsg.RemoveName("Loglevels");
	}
	if (version < 13) {
		// changes introduced with version 13:
		//
		// rename Loglevel_Pop to Loglevel_Recv:
		int32 level = 0;
		mPrefsMsg.FindInt32( "Loglevel_Pop", &level);
		mPrefsMsg.AddInt32( "Loglevel_Recv", level);
		mPrefsMsg.RemoveName("Loglevel_Pop");
	}
	mSavedPrefsMsg = mPrefsMsg;
	
	SetLoglevels();
	SetupMailboxVolume();

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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg = mSavedPrefsMsg;
	SetLoglevels();
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg = mDefaultsMsg;
	SetLoglevels();
	mShortcutsMsg.MakeEmpty();
	GetShortcutDefaults( &mShortcutsMsg);
}

/*------------------------------------------------------------------------------*\
	InitDefaults( )
		-	constructs a BMessage containing all defaultVal values
\*------------------------------------------------------------------------------*/
void BmPrefs::InitDefaults(BMessage& defaultsMsg) {
	defaultsMsg.MakeEmpty();
	defaultsMsg.AddInt16( MSG_VERSION, nPrefsVersion);
	int32 loglevels = BM_LOGLVL1(BM_LogRecv)
							+ BM_LOGLVL0(BM_LogJobWin) 
							+ BM_LOGLVL0(BM_LogMailParse) 
							+ BM_LOGLVL1(BM_LogApp) 
							+ BM_LOGLVL0(BM_LogMailTracking)
							+ BM_LOGLVL0(BM_LogGui)
							+ BM_LOGLVL0(BM_LogModelController)
							+ BM_LOGLVL1(BM_LogSmtp)
							+ BM_LOGLVL1(BM_LogFilter)
							+ BM_LOGLVL0(BM_LogRefCount);
	defaultsMsg.AddInt32( "Loglevel_Recv", 
								  BM_LOGLVL_FOR(loglevels,BM_LogRecv));
	defaultsMsg.AddInt32( "Loglevel_JobWin", 
								  BM_LOGLVL_FOR(loglevels,BM_LogJobWin));
	defaultsMsg.AddInt32( "Loglevel_MailParse", 
								  BM_LOGLVL_FOR(loglevels,BM_LogMailParse));
	defaultsMsg.AddInt32( "Loglevel_App", 
								  BM_LOGLVL_FOR(loglevels,BM_LogApp));
	defaultsMsg.AddInt32( "Loglevel_MailTracking", 
								  BM_LOGLVL_FOR(loglevels,BM_LogMailTracking));
	defaultsMsg.AddInt32( "Loglevel_Gui", 
								  BM_LOGLVL_FOR(loglevels,BM_LogGui));
	defaultsMsg.AddInt32( "Loglevel_ModelController", 
								  BM_LOGLVL_FOR(loglevels,BM_LogModelController));
	defaultsMsg.AddInt32( "Loglevel_Smtp", 
								  BM_LOGLVL_FOR(loglevels,BM_LogSmtp));
	defaultsMsg.AddInt32( "Loglevel_Filter", 
								  BM_LOGLVL_FOR(loglevels,BM_LogFilter));
	defaultsMsg.AddInt32( "Loglevel_RefCount", 
								  BM_LOGLVL_FOR(loglevels,BM_LogRefCount));

	defaultsMsg.AddBool( "AddPeopleNameToMailAddr", true);
	defaultsMsg.AddBool( "Allow8BitMime", false);
	defaultsMsg.AddBool( "Allow8BitMimeInHeader", false);
	defaultsMsg.AddBool( "AutoCharsetDetectionInbound", true);
	defaultsMsg.AddBool( "AutoCharsetDetectionOutbound", true);
	defaultsMsg.AddString( "AutoCharsetsInbound", 
									"us-ascii,utf-8,default");
	defaultsMsg.AddString( "AutoCharsetsOutbound", 
									"us-ascii,default,iso-2022-jp,utf-8");
	defaultsMsg.AddBool( "AutoCheckOnlyIfPPPRunning", true);
	defaultsMsg.AddBool( "AvoidPrefsSanityChecks", false);
	defaultsMsg.AddBool( "BeepWhenNewMailArrived", true);
	defaultsMsg.AddBool( "CacheRefsInMem", false);
	defaultsMsg.AddBool( "CacheRefsOnDisk", true);
	defaultsMsg.AddBool( "CloseViewWinAfterMailAction", true);
	defaultsMsg.AddString( "DefaultCharset", 
									BmEncoding::DefaultCharset.String());
	defaultsMsg.AddString( "DefaultForwardType", "Inline");
	defaultsMsg.AddBool( "DoNotAttachVCardsToForward", false);
	defaultsMsg.AddBool( "DynamicStatusWin", true);
	defaultsMsg.AddInt32( "ExpandCollapseDelay", 1000);
	defaultsMsg.AddInt32( "FeedbackTimeout", 200);
	defaultsMsg.AddString( "ForwardIntroStr", "On %d at %t, %f wrote:");
	defaultsMsg.AddString( "ForwardSubjectRX", 
									"^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	defaultsMsg.AddString( "ForwardSubjectStr", "Fwd: %s");
	defaultsMsg.AddBool( "GenerateOwnMessageIDs", true);
	defaultsMsg.AddBool( "HardWrapMailText", true);
	defaultsMsg.AddString( "HeaderListLarge", 
									"Subject,From,Date,To,Cc,User-Agent/X-Mailer");
	defaultsMsg.AddString( "HeaderListSmall", "Subject,From,Date");
	BmString defaultIconPath = BeamRoster->AppPath() + nDefaultIconset;
	defaultsMsg.AddString( "IconPath", defaultIconPath.String());
	defaultsMsg.AddBool( "InOutAlwaysAtTop", true);
	defaultsMsg.AddBool( "ImportExportTextAsUtf8", true);
	defaultsMsg.AddString( "ListFields", "Mail-Followup-To,Reply-To");
	defaultsMsg.AddBool( "ListviewLikeTracker", false);
	defaultsMsg.AddInt32( "ListviewFlatMinItemHeight", 16);
	defaultsMsg.AddInt32( "ListviewHierarchicalMinItemHeight", 16);
	defaultsMsg.AddBool( "ListviewUsesStringSpacing", false);
	defaultsMsg.AddBool( "LookForPeopleOnlyInPeopleFolder", true);
	// standard mail-box:
	defaultsMsg.AddString( "MailboxPath", "/boot/home/mail");
	defaultsMsg.AddBool( "MakeQPSafeForEBCDIC", true);
	defaultsMsg.AddBool( "MapClassificationGenuineToTofu", true);
	defaultsMsg.AddInt32( "MarkAsReadDelay", 500);
	defaultsMsg.AddInt32( "MaxLineLen", 76);
	defaultsMsg.AddInt32( "MaxLineLenForHardWrap", 998);
	defaultsMsg.AddInt32( "MinLogfileSize", 50*1024);
	defaultsMsg.AddInt32( "MaxLogfileSize", 200*1024);
	defaultsMsg.AddBool( "NeverExceed78Chars", false);
	defaultsMsg.AddString( 
		"MimeTypeTrustInfo", 
		"application/pdf:T,application/zip:T,application:W,:T"
	);
	defaultsMsg.AddInt32( "MSecsBeforeImapRemove", 5000*1000);
	defaultsMsg.AddInt32( "MSecsBeforeMailFilterShows", 500*1000);
	defaultsMsg.AddInt32( "MSecsBeforeMailMoverShows", 500*1000);
	defaultsMsg.AddInt32( "MSecsBeforePopperRemove", 5000*1000);
	defaultsMsg.AddInt32( "MSecsBeforeRemoveFailed", 5000*1000);
	defaultsMsg.AddInt32( "MSecsBeforeSmtpRemove", 0*1000);
	defaultsMsg.AddInt32( "NetReceiveBufferSize", 15000);
	defaultsMsg.AddInt32( "NetSendBufferSize", 15000);
	defaultsMsg.AddBool( "QueueNetworkJobs", true);
	defaultsMsg.AddString( "QuoteFormatting", "Push Margin");
	defaultsMsg.AddString( "QuotingLevelRX", 
									"^((?:\\w?\\w?\\w?[>|]|[ \\t]*)*)(.*?)$");
	defaultsMsg.AddString( "QuotingString", "> ");
	defaultsMsg.AddString( "PeopleFolder", "/boot/home/people");
	defaultsMsg.AddBool( "PreferReplyToList", true);
	defaultsMsg.AddBool( "PreferUserAgentOverX-Mailer", true);
	defaultsMsg.AddInt32( "PulsedScrollDelay", 100);
	defaultsMsg.AddInt32( "ReceiveTimeout", 60);
	defaultsMsg.AddString( "ReplyIntroDefaultNick", "you");
	defaultsMsg.AddString( "ReplyIntroStr", "On %d at %t, %f wrote:");
	defaultsMsg.AddString( "ReplySubjectRX", "^\\s*(Re|Aw)(\\[\\d+\\])?:");
	defaultsMsg.AddString( "ReplySubjectStr", "Re: %s");
	defaultsMsg.AddBool( "RestoreFolderStates", true);
	defaultsMsg.AddBool( "SelectNextMailAfterDelete", true);
	defaultsMsg.AddBool( "SendPendingMailsOnCheck", true);
	defaultsMsg.AddBool( "SetMailDateWithEverySave", true);
	defaultsMsg.AddMessage( "Shortcuts", GetShortcutDefaults());
	defaultsMsg.AddBool( "ShowAlertForErrors", false);
	defaultsMsg.AddBool( "ShowDecodedLength", true);
	defaultsMsg.AddBool( "ShowToolbarBorder", false);
	defaultsMsg.AddBool( "ShowToolbarIcons", true);
	defaultsMsg.AddString( "ShowToolbarLabel", "Right");
	defaultsMsg.AddBool( "ShowTooltips", true);
	defaultsMsg.AddString( "SignatureRX", "^---?\\s*\\n");
	defaultsMsg.AddInt32( "SpacesPerTab", 4);
	defaultsMsg.AddBool("SpecialHeaderForEachBcc", false);
	defaultsMsg.AddString( "StandardCharsets", 
									"iso-8859-1,iso-8859-2,iso-8859-3,iso-8859-4,"
									"iso-8859-5,iso-8859-6,iso-8859-7,iso-8859-8,"
									"iso-8859-9,iso-8859-10,iso-8859-13,iso-8859-14,"
									"iso-8859-15,macroman,windows-1251,windows-1252,"
									"cp866,cp850,iso-2022-jp,iso-2022-jp-2,"
									"koi8-r,euc-kr,big-5,us-ascii,utf-8");
	defaultsMsg.AddBool( "StrictCharsetHandling", false);
	defaultsMsg.AddBool( "StripedListView", true);
	defaultsMsg.AddString( "TimeModeInHeaderView", "Local");
	defaultsMsg.AddString( "UndoMode", "Words");
	defaultsMsg.AddBool( "UseDeskbar", true);
	defaultsMsg.AddBool( "UseDocumentResizer", true);
	defaultsMsg.AddBool( "UseSwatchTimeInRefView", false);
	defaultsMsg.AddString( "Workspace", "Current");
}

/*------------------------------------------------------------------------------*\
	GetShortcutFor( shortcutID)
		-	returns the shortcut for a given id (label):
\*------------------------------------------------------------------------------*/
BmString BmPrefs::GetShortcutFor( const char* shortcutID) {
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	const char* sc = mShortcutsMsg.FindString( shortcutID);
	return sc;
}

/*------------------------------------------------------------------------------*\
	SetShortcutIfNew( msg, name, val)
		-	adds a shortcut to the given message if that does not yet have it
		-	this method is used to copy new shortcuts over from the defaults-msg
			to the current prefs-message
\*------------------------------------------------------------------------------*/
void BmPrefs::SetShortcutIfNew( BMessage* msg, const char* name, 
										  const BmString val) {
	type_code tc;
	if (msg->GetInfo( name, &tc) != B_OK)
		msg->AddString( name, val.String());
}

/*------------------------------------------------------------------------------*\
	SetShortcutFor( name, val)
		-	updates a shortcut to the given value
\*------------------------------------------------------------------------------*/
void BmPrefs::SetShortcutFor( const char* name, const BmString val) {
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	SetShortcutIfNew( shortcutsMsg, "Check Mail", "M");
	SetShortcutIfNew( shortcutsMsg, "Check All Accounts", "<SHIFT>M");
	SetShortcutIfNew( shortcutsMsg, "Close", "W");
	SetShortcutIfNew( shortcutsMsg, "Copy", "C");
	SetShortcutIfNew( shortcutsMsg, "Cut", "X");
	SetShortcutIfNew( shortcutsMsg, "Delete Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Edit As New", "");
	SetShortcutIfNew( shortcutsMsg, "Filter", "");
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
	SetShortcutIfNew( shortcutsMsg, "Next Message", "<DOWN_ARROW>");
	SetShortcutIfNew( shortcutsMsg, "Page Setup", "");
	SetShortcutIfNew( shortcutsMsg, "Paste", "V");
	SetShortcutIfNew( shortcutsMsg, "Preferences", "<SHIFT>P");
	SetShortcutIfNew( shortcutsMsg, "Previous Message", "<UP_ARROW>");
	SetShortcutIfNew( shortcutsMsg, "Print Message", "P");
	SetShortcutIfNew( shortcutsMsg, "Quit Beam", "Q");
	SetShortcutIfNew( shortcutsMsg, "Recache Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Redirect", "B");
	SetShortcutIfNew( shortcutsMsg, "Redo", "<SHIFT>Z");
	SetShortcutIfNew( shortcutsMsg, "Rename Folder", "");
	SetShortcutIfNew( shortcutsMsg, "Reply", "R");
	SetShortcutIfNew( shortcutsMsg, "Reply To List", "");
	SetShortcutIfNew( shortcutsMsg, "Reply To Person", "");
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
	GetNumericLogLevelFor( terrain)
		-	returns the current log-level for the given terrain:
\*------------------------------------------------------------------------------*/
uint32 BmPrefs::GetNumericLogLevelFor( uint32 terrain) {
	uint32 level = 0;
	if (terrain == BM_LogRecv)
		level = mPrefsMsg.FindInt32("Loglevel_Recv");
	else if (terrain == BM_LogSmtp)
		level = mPrefsMsg.FindInt32("Loglevel_Smtp");
	else if (terrain == BM_LogApp)
		level = mPrefsMsg.FindInt32("Loglevel_App");
	else if (terrain == BM_LogFilter)
		level = mPrefsMsg.FindInt32("Loglevel_Filter");
	else if (terrain == BM_LogMailParse)
		level = mPrefsMsg.FindInt32("Loglevel_MailParse");
	else if (terrain == BM_LogMailTracking)
		level = mPrefsMsg.FindInt32("Loglevel_MailTracking");
	else if (terrain == BM_LogJobWin)
		level = mPrefsMsg.FindInt32("Loglevel_JobWin");
	else if (terrain == BM_LogGui)
		level = mPrefsMsg.FindInt32("Loglevel_Gui");
	else if (terrain == BM_LogModelController)
		level = mPrefsMsg.FindInt32("Loglevel_ModelController");
	else if (terrain == BM_LogRefCount)
		level = mPrefsMsg.FindInt32("Loglevel_RefCount");
	return level;
}
/*------------------------------------------------------------------------------*\
	GetLogLevelFor( terrain)
		-	returns the current log-level (as string) for the given terrain:
\*------------------------------------------------------------------------------*/
const char* BmPrefs::GetLogLevelFor( uint32 terrain) {
	int32 level = GetNumericLogLevelFor(terrain);

	if (level == 1)
		return LOG_LVL_1;
	else if (level == 2)
		return LOG_LVL_2;
	else if (level == 3)
		return LOG_LVL_3;
	else
		return LOG_LVL_0;
}

/*------------------------------------------------------------------------------*\
	SetLogLevelForTo( terrain, loglevel)
		-	set the current log-level (given as string) for the given terrain
\*------------------------------------------------------------------------------*/
void BmPrefs::SetLogLevelForTo( uint32 terrain, BmString loglevel) {
	int32 level;
	if (loglevel.ICompare(LOG_LVL_1) == 0)
		level = 1;
	else if (loglevel.ICompare(LOG_LVL_2) == 0)
		level = 2;
	else if (loglevel.ICompare(LOG_LVL_3) == 0)
		level = 3;
	else
		level = 0;

	if (terrain == BM_LogRecv)
		mPrefsMsg.ReplaceInt32("Loglevel_Recv", level);
	else if (terrain == BM_LogSmtp)
		mPrefsMsg.ReplaceInt32("Loglevel_Smtp", level);
	else if (terrain == BM_LogApp)
		mPrefsMsg.ReplaceInt32("Loglevel_App", level);
	else if (terrain == BM_LogFilter)
		mPrefsMsg.ReplaceInt32("Loglevel_Filter", level);
	else if (terrain == BM_LogMailParse)
		mPrefsMsg.ReplaceInt32("Loglevel_MailParse", level);
	else if (terrain == BM_LogMailTracking)
		mPrefsMsg.ReplaceInt32("Loglevel_MailTracking", level);
	else if (terrain == BM_LogJobWin)
		mPrefsMsg.ReplaceInt32("Loglevel_JobWin", level);
	else if (terrain == BM_LogGui)
		mPrefsMsg.ReplaceInt32("Loglevel_Gui", level);
	else if (terrain == BM_LogModelController)
		mPrefsMsg.ReplaceInt32("Loglevel_ModelController", level);
	else if (terrain == BM_LogRefCount)
		mPrefsMsg.ReplaceInt32("Loglevel_RefCount", level);
	
	SetLoglevels();
}

/*------------------------------------------------------------------------------*\
	SetLoglevels( )
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetLoglevels() {
	// transfer loglevel-definitions to log-handler:
	int32 loglevels 
		= BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_Recv"), BM_LogRecv)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_JobWin"), BM_LogJobWin) 
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_MailParse"),
															 BM_LogMailParse) 
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_App"), BM_LogApp) 
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_MailTracking"),
															 BM_LogMailTracking)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_Gui"), BM_LogGui)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_ModelController"),
															 BM_LogModelController)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_Smtp"), BM_LogSmtp)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_Filter"), BM_LogFilter)
			+ BM_LOGLVL_VAL(mPrefsMsg.FindInt32( "Loglevel_RefCount"),
															 BM_LogRefCount);
	TheLogHandler->LogLevels( loglevels, 
									  GetInt( "MinLogfileSize", 50*1024),
									  GetInt( "MaxLogfileSize", 200*1024));
	TheLogHandler->ShowErrorsOnScreen( GetBool( "ShowAlertForErrors", false));
	BmString s;
	for( int i=31; i>=0; --i) {
		if (loglevels & (01UL<<i))
			s << "1";
		else
			s << "0";
	}
	BM_LOG3( BM_LogApp, 
				BmString("Initialized loglevels to binary value ") << s);
}

/*------------------------------------------------------------------------------*\
	SetupMailboxVolume( )
		-	
\*------------------------------------------------------------------------------*/
void BmPrefs::SetupMailboxVolume() {
	// determine the volume of our mailbox:
	BmString mailboxPath = GetString( "MailboxPath", "/boot/home/mail");
	BEntry entry;
	if (entry.SetTo( mailboxPath.String(), true) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("Sorry, could not get entry for mailbox <") 
				<< mailboxPath << "> !"
		);
	node_ref nref;
	if (entry.GetNodeRef( &nref) != B_OK)
		BM_THROW_RUNTIME( "Sorry, could not determine mailbox-volume !?!");
	MailboxVolume = nref.device;
	
	// now find out about Trash-path on this volume:
	BPath path;
	if (find_directory(B_TRASH_DIRECTORY, &path, false, &MailboxVolume) != B_OK) {
		BM_LOGERR( "Mailbox-volume has no trash!");
		TrashPath = "/Dummy-Trash";
	} else
		TrashPath = path.Path();
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores preferences into global Settings-file:
\*------------------------------------------------------------------------------*/
bool BmPrefs::Store() {
	BmBackedFile prefsFile;
	status_t err;
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");

	try {
		BmString prefsFilename 
			= BmString( BeamRoster->SettingsPath()) 
					<< "/" << PREFS_FILENAME;
		if ((err = prefsFile.SetTo( prefsFilename.String())) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not create settings file\n\t<") 
										<< prefsFilename << ">\n\n Result: " 
										<< strerror(err));
		// update version:
		mPrefsMsg.RemoveName( MSG_VERSION);
		mPrefsMsg.AddInt16( MSG_VERSION, nPrefsVersion);
		// update shortcuts:
		mPrefsMsg.RemoveName("Shortcuts");
		mPrefsMsg.AddMessage("Shortcuts", &mShortcutsMsg);

		BmString tip("The settings can be found in the attributes of this file!");
		prefsFile.File().Write(tip.String(), tip.Length());
		// store prefs-data as attributes:
#ifdef B_BEOS_VERSION_DANO
		const char *name;
#else
		char *name;
#endif
		uint32 type;
		int32 count;
		for (int32 i = 0; 
			  mPrefsMsg.GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK; 
			  ++i) {
			for (int32 j = 0; j < count; ++j) {
				const void *value;
				ssize_t msize;

				err = mPrefsMsg.FindData( name, type, j, &value, &msize);
				if (err == B_OK)
					err = prefsFile.File().WriteAttr( name, type, 0, value, msize);
				if (err < B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not store setting <") 
							<< name << "> into file\n\t<"
							<< prefsFilename << ">\n\n Result: " 
							<< strerror(err));
			}
		}

		// update saved state to current:
		mSavedPrefsMsg = mPrefsMsg;
	} catch( BM_error &e) {
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	const char* val;
	if (mPrefsMsg.FindString( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindString( name, &val) == B_OK) {
			mPrefsMsg.AddString( name, val);
			return val;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name 
								<< " of type string is unknown");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	int32 val;
	if (mPrefsMsg.FindInt32( name, &val) == B_OK)
		return val;
	else {
		if (mDefaultsMsg.FindInt32( name, &val) == B_OK) {
			mPrefsMsg.AddInt32( name, val);
			return val;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name 
								<< " of type int32 is unknown");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	BMessage* msg = new BMessage();
	if (mPrefsMsg.FindMessage( name, msg) == B_OK) {
		return msg;
	} else {
		if (mDefaultsMsg.FindMessage( name, msg) == B_OK) {
			mPrefsMsg.AddMessage( name, msg);
			return msg;
		} else {
			BM_SHOWERR( BmString("The Preferences-field ") << name 
								<< " of type message is unknown");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
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
	BAutolock lock( mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "Prefs: Unable to get lock!");
	mPrefsMsg.RemoveName( name);
	mPrefsMsg.AddString( name, val.String());
}
