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
		-	if no preference-file is found, default prefs are used
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
		// ...no settings file yet, we start with default settings...
		prefs = new BmPrefs;
		// ...and create a new and shiny settings file:
		prefs->Store();
	}

	theInstance = prefs;
	return prefs;
}

/*------------------------------------------------------------------------------*\
	BmPrefs()
		-	default constructor
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( void)
	:	BArchivable() 
	,	mDynamicConnectionWin( CONN_WIN_DYNAMIC)
	,	mReceiveTimeout( 60 )
	,	mLoglevels( BM_LOGLVL2(BM_LogPop) 
						+ BM_LOGLVL2(BM_LogConnWin) 
						+ BM_LOGLVL2(BM_LogMailParse) 
						+ BM_LOGLVL2(BM_LogUtil) 
						+ BM_LOGLVL2(BM_LogMailTracking)
						+ BM_LOGLVL2(BM_LogFolderView)
						+ BM_LOGLVL2(BM_LogRefView)
						+ BM_LOGLVL2(BM_LogMainWindow)
						+ BM_LOGLVL3(BM_LogModelController)
						)
	,	mMailboxPath("/boot/home/mail")			// TODO: change default to .../mail
	,	mRefCacheInMem( false)
	,	mRefCacheOnDisk( true)
	,	mDefaultEncoding( B_ISO1_CONVERSION)
	,	mStripedListView( true)
	,	mMailRefLayout( new BMessage)
	,	mRestoreFolderStates( true)
	,	mShowDecodedLength( true)
{
#ifdef BM_LOGGING
	BString s;
	for( int i=31; i>=0; --i) {
		if (mLoglevels & (01UL<<i))
			s << "1";
		else
			s << "0";
	}
#endif
	// transfer loglevel-definitions to log-handler:
	TheLogHandler->LogLevels( mLoglevels);
	BM_LOG3( BM_LogUtil, BString("Initialized loglevels to binary value ") << s);
}

/*------------------------------------------------------------------------------*\
	BmPrefs( archive)
		-	constructs a BmPrefs from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( BMessage* archive) 
	: BArchivable( archive)
{
	mDynamicConnectionWin = static_cast<TConnWinMode>(FindMsgInt16( archive, MSG_DYNAMIC_CONN_WIN));
	mReceiveTimeout = ntohs(FindMsgInt16( archive, MSG_RECEIVE_TIMEOUT));
	mLoglevels = ntohl(FindMsgInt32( archive, MSG_LOGLEVELS));
	mMailboxPath = FindMsgString( archive, MSG_MAILBOXPATH);
	mRefCacheInMem = FindMsgBool( archive, MSG_REF_CACHE_MEM);
	mRefCacheOnDisk = FindMsgBool( archive, MSG_REF_CACHE_DISK);
	mDefaultEncoding = FindMsgInt32( archive, MSG_DEFAULT_ENCODING);
	mStripedListView = FindMsgBool( archive, MSG_STRIPED_LISTVIEW);
	mMailRefLayout = FindMsgMsg( archive, MSG_MAILREF_LAYOUT);
	mRestoreFolderStates = FindMsgBool( archive, MSG_RESTORE_FOLDERS);
	mShowDecodedLength = FindMsgBool( archive, MSG_SHOW_DECODED_LENGTH);
	TheLogHandler->LogLevels( mLoglevels);
}

/*------------------------------------------------------------------------------*\
	~BmPrefs()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmPrefs::~BmPrefs() {
	delete mMailRefLayout;
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPrefs into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPrefs::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		||	archive->AddString("class", "BmPrefs")
		||	archive->AddInt16( MSG_DYNAMIC_CONN_WIN, mDynamicConnectionWin)
		||	archive->AddInt16( MSG_RECEIVE_TIMEOUT, htons(mReceiveTimeout))
		||	archive->AddInt32( MSG_LOGLEVELS, htonl(mLoglevels))
		||	archive->AddString( MSG_MAILBOXPATH, mMailboxPath.String())
		||	archive->AddBool( MSG_REF_CACHE_MEM, mRefCacheInMem)
		||	archive->AddBool( MSG_REF_CACHE_DISK, mRefCacheOnDisk)
		||	archive->AddInt32( MSG_DEFAULT_ENCODING, htonl(mDefaultEncoding))
		||	archive->AddBool( MSG_STRIPED_LISTVIEW, mStripedListView)
		||	archive->AddMessage( MSG_MAILREF_LAYOUT, mMailRefLayout)
		||	archive->AddBool( MSG_RESTORE_FOLDERS, mRestoreFolderStates)
		||	archive->AddBool( MSG_SHOW_DECODED_LENGTH, mShowDecodedLength);
	return ret;
}

/*------------------------------------------------------------------------------*\
	Instantiate( archive)
		-	(re-)creates a PopAccount from a given BMessage
\*------------------------------------------------------------------------------*/
BArchivable* BmPrefs::Instantiate( BMessage* archive) {
	if (!validate_instantiation( archive, "BmPrefs"))
		return NULL;
	return new BmPrefs( archive);
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores preferences into global Settings-file:
\*------------------------------------------------------------------------------*/
bool BmPrefs::Store() {
	BMessage archive;
	BFile prefsFile;
	status_t err;

	try {
		BString prefsFilename = BString( TheResources->SettingsPath.Path()) << "/" << PREFS_FILENAME;
		this->Archive( &archive) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive BmPrefs-object");
		(err = prefsFile.SetTo( prefsFilename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create settings file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}
