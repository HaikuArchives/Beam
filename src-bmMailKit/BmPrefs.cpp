/*
	BmPrefs.cpp

		$Id$
*/

#include <Alert.h>
#include <ByteOrder.h>
#include <Message.h>
#include <StorageKit.h>

#include "BmPrefs.h"

BPath BmPrefs::mgPrefsPath;

/*------------------------------------------------------------------------------*\
	Beam
		-	definition of our global vars:
\*------------------------------------------------------------------------------*/
BmLogHandler* Beam::LogHandler = NULL;
BmPrefs* Beam::Prefs = NULL;
BString Beam::HomePath;
BVolume Beam::MailboxVolume;
char *Beam::WHITESPACE = "\n\r\t ";
int Beam::InstanceCount = 0;

Beam::Beam() {
	if (InstanceCount > 0)
		throw BM_runtime_error("Trying to initialize more than one instance of class Beam");
	if (!BmPrefs::InitPrefs())
		exit(10);
	Beam::LogHandler = new BmLogHandler();
	InstanceCount++;
}

Beam::~Beam() {
	delete Beam::LogHandler;
	delete Beam::Prefs;
	InstanceCount--;
};

/*------------------------------------------------------------------------------*\
	InitPrefs()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, default prefs are used
\*------------------------------------------------------------------------------*/
bool BmPrefs::InitPrefs() 
{
	status_t err;
	BPath homePath, settingsPath;
	BString prefsFilename;
	BFile prefsFile;
	entry_ref eref;

	try {
		// determine the path to our home-directory:
		find_directory( B_USER_DIRECTORY, &homePath) == B_OK	
													|| BM_THROW_RUNTIME( "Sorry, could not determine user's settings-dir !?!");
		Beam::HomePath = homePath.Path();
		// and determine the volume of our mailbox:
		get_ref_for_path( homePath.Path(), &eref) == B_OK
													|| BM_THROW_RUNTIME( "Sorry, could not determine mailbox-volume !?!");
		Beam::MailboxVolume = eref.device;
	
		// determine the path to the user-settings-directory:
		find_directory( B_USER_SETTINGS_DIRECTORY, &settingsPath) == B_OK
													|| BM_THROW_RUNTIME( "Sorry, could not determine user's settings-dir !?!");

		mgPrefsPath.SetTo( settingsPath.Path(), "Beam");

		// try to open settings-file...
		prefsFilename = BString(mgPrefsPath.Path()) << "/" << PREFS_FILENAME;
		if ((err = prefsFile.SetTo( prefsFilename.String(), B_READ_ONLY)) == B_OK) {
			// ...ok, settings file found, we fetch our prefs from it:
			BMessage archive;
			(err = archive.Unflatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch settings from file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
			Beam::Prefs = new BmPrefs( &archive);
		} else {
			// ...no settings file yet, we start with default settings...
			Beam::Prefs = new BmPrefs;
			// ...and create a new and shiny settings file:
			if (!Beam::Prefs->Store())
				return false;
		}
	} catch (exception &e) {
		BAlert *alert = new BAlert( NULL, e.what(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------*\
	BmPrefs()
		-	default constructor
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( void)
	: BArchivable() 
	, StopWatch( "Beam", true)
	, mDynamicConnectionWin( CONN_WIN_DYNAMIC)
	, mReceiveTimeout( 60 )
	, mLoglevels( BM_LOGLVL2(BM_LogPop) 
					| BM_LOGLVL2(BM_LogConnWin) 
					| BM_LOGLVL2(BM_LogMailParse) 
					| BM_LOGLVL2(BM_LogUtil) 
					| BM_LOGLVL2(BM_LogMailFolders)
					)
{
}

/*------------------------------------------------------------------------------*\
	BmPrefs( archive)
		-	constructs a BmPrefs from a BMessage
		-	N.B.: BMessage must be in NETWORK-BYTE-ORDER
\*------------------------------------------------------------------------------*/
BmPrefs::BmPrefs( BMessage *archive) 
	: BArchivable( archive)
	, StopWatch( "Beam", true)
{
	mDynamicConnectionWin = static_cast<TConnWinMode>(FindMsgInt16( archive, MSG_DYNAMIC_CONN_WIN));
	mReceiveTimeout = ntohs(FindMsgInt16( archive, MSG_RECEIVE_TIMEOUT));
	mLoglevels = ntohl(FindMsgInt32( archive, MSG_LOGLEVELS));
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmPrefs into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmPrefs::Archive( BMessage *archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString("class", "BmPrefs")
		||	archive->AddInt16( MSG_DYNAMIC_CONN_WIN, mDynamicConnectionWin)
		||	archive->AddInt16( MSG_RECEIVE_TIMEOUT, htons(mReceiveTimeout))
		||	archive->AddInt32( MSG_LOGLEVELS, htonl(mLoglevels)));
	return ret;
}

/*------------------------------------------------------------------------------*\
	Instantiate( archive)
		-	(re-)creates a PopAccount from a given BMessage
\*------------------------------------------------------------------------------*/
BArchivable* BmPrefs::Instantiate( BMessage *archive) {
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
		BString prefsFilename = BString(mgPrefsPath.Path()) << "/" << PREFS_FILENAME;
		this->Archive( &archive) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive BmPrefs-object");
		(err = prefsFile.SetTo( prefsFilename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create settings file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << prefsFilename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BAlert *alert = new BAlert( NULL, e.what(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return false;
	}
	return true;
}
