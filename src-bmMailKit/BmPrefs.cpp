/*
	BmPrefs.cpp

		$Id$
*/

#include <Alert.h>
#include <ByteOrder.h>
#include <Directory.h>
#include <File.h>
#include <Message.h>
#include <UTF8.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
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
	SetLoglevels();
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
	mDefaultsMsg.AddBool( "DynamicStatusWin", true);
	mDefaultsMsg.AddInt32( "ReceiveTimeout", 60);
	mDefaultsMsg.AddInt32( "Loglevels", BM_LOGLVL2(BM_LogPop) 
										+ BM_LOGLVL2(BM_LogJobWin) 
										+ BM_LOGLVL3(BM_LogMailParse) 
										+ BM_LOGLVL2(BM_LogUtil) 
										+ BM_LOGLVL2(BM_LogMailTracking)
										+ BM_LOGLVL2(BM_LogFolderView)
										+ BM_LOGLVL2(BM_LogRefView)
										+ BM_LOGLVL2(BM_LogMainWindow)
										+ BM_LOGLVL3(BM_LogModelController)
										+ BM_LOGLVL2(BM_LogMailEditWin)
										+ BM_LOGLVL3(BM_LogSmtp)
										);
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
	mDefaultsMsg.AddString( "DefaultSmtpAccount", "");
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
#endif
	BM_LOG3( BM_LogUtil, BString("Initialized loglevels to binary value ") << s);
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
		(err = mPrefsMsg.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
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
		BM_SHOWERR( BString("The Preferences-field ") << name << " of type string is unknown");
		return "";
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
	else
		return defaultVal;
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
		BM_SHOWERR( BString("The Preferences-field ") << name << " of type bool is unknown");
		return false;
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
	else
		return defaultVal;
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
		BM_SHOWERR( BString("The Preferences-field ") << name << " of type int32 is unknown");
		return 0;
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
	else
		return defaultVal;
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
		BM_SHOWERR( BString("The Preferences-field ") << name << " of type message is unknown");
		return NULL;
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
	} else
		return defaultVal;
}
