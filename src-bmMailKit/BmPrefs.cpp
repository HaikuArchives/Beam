/*
	BmPopAccount.cpp

		$Id$
*/

#include <string.h>

#include <Alert.h>
#include <ByteOrder.h>
#include <Message.h>
#include <StorageKit.h>

#include "BmPrefs.h"

// global pointer to preferences:
namespace Beam {
	BmPrefs* Prefs = 0;
}

BString BmPrefs::PrefsFilePath = "";

/*------------------------------------------------------------------------------*\
	InitPrefs()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, default prefs are used
\*------------------------------------------------------------------------------*/
bool BmPrefs::InitPrefs() 
{
	status_t err;
	BPath prefsPath;
	BFile prefsFile;
	
	if (find_directory( B_USER_SETTINGS_DIRECTORY, &prefsPath) != B_OK) {
		BAlert *alert = new BAlert( NULL, "Sorry, could not determine user's settings-dir !?!", "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return false;
	}

	PrefsFilePath = BString(prefsPath.Path()) << "/Beam/" << PREFS_FILENAME;
	if ((err = prefsFile.SetTo( PrefsFilePath.String(), B_READ_ONLY)) == B_OK) {
		BMessage archive;
		if ((err = archive.Unflatten( &prefsFile)) != B_OK) {
			BString text = BString("Could not fetch settings from file\n\t<") << PrefsFilePath << ">\n\n Result: " << strerror(err);
			BAlert *alert = new BAlert( NULL, text.String(), "OK", NULL, NULL, 
												 B_WIDTH_AS_USUAL, B_STOP_ALERT);
			alert->Go();
			return false;
		}
		Beam::Prefs = new BmPrefs( &archive);
	} else {
		// no settings file, we start with default settings...
		Beam::Prefs = new BmPrefs;
		// ...and create a new settings file:
		if (!Beam::Prefs->Store())
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
	, mDynamicConnectionWin( CONN_WIN_DYNAMIC)
	, mReceiveTimeout( 60 )
	, mLoglevels( BM_LOGLVL2(BM_LogPop) 
					| BM_LOGLVL3(BM_LogConnWin) 
					| BM_LOGLVL3(BM_LogMailParse) 
					| BM_LOGLVL3(BM_LogUtil) 
					)
	, StopWatch( "Beam", true)
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
	mLoglevels = ntohs(FindMsgInt32( archive, MSG_LOGLEVELS));
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
		||	archive->AddInt32( MSG_LOGLEVELS, htons(mLoglevels)));
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
		if (this->Archive( &archive) != B_OK) {
			throw runtime_error("Unable to archive BmPrefs-object");
		}
		if ((err = prefsFile.SetTo( PrefsFilePath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) != B_OK) {
			BString text = BString("Could not create settings file\n\t<") << PrefsFilePath << ">\n\n Result: " << strerror(err);
			throw runtime_error( text.String());
		}
		if ((err = archive.Flatten( &prefsFile)) != B_OK) {
			BString text = BString("Could not store settings into file\n\t<") << PrefsFilePath << ">\n\n Result: " << strerror(err);
			throw runtime_error( text.String());
		}
	} catch( exception &e) {
		BAlert *alert = new BAlert( NULL, e.what(), "OK", NULL, NULL, 
											 B_WIDTH_AS_USUAL, B_STOP_ALERT);
		alert->Go();
		return false;
	}
	return true;
}
