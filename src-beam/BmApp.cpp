/*
	BmApp.cpp
		$Id$
*/

#include <FindDirectory.h>

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMainWindow.h"
#include "BmPrefs.h"
#include "BmUtil.h"

int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig)
	:	inherited( sig)
	,	LogHandler( NULL)
	,	Prefs( NULL)
	,	FolderIcon( NULL)
	,	MailIcon( NULL)
	,	WHITESPACE( " \t\n\r\f")
	,	MailFolderList( NULL)
	,	MailFolderView( NULL)
	,	MailRefView( NULL)
	,	MainWindow( NULL)
	,	mIsQuitting( false)
{
	status_t err;
	BMimeType mt;
	BPath path;
	entry_ref eref;

	if (InstanceCount > 0)
		throw BM_runtime_error("Trying to initialize more than one instance of class Beam");

	bmApp = this;

	try {
		// create the log-handler
		LogHandler = BmLogHandler::CreateInstance( 1);
	
		// determine the path to our home-directory:
		find_directory( B_USER_DIRECTORY, &path) == B_OK
														|| BM_THROW_RUNTIME( "Sorry, could not determine user's settings-dir !?!");
		HomePath = path.Path();
	
		// and determine the volume of our mailbox:
		get_ref_for_path( HomePath.String(), &eref) == B_OK
														|| BM_THROW_RUNTIME( "Sorry, could not determine mailbox-volume !?!");
		MailboxVolume = eref.device;
		
		// determine the path to the user-settings-directory:
		find_directory( B_USER_SETTINGS_DIRECTORY, &path) == B_OK
														|| BM_THROW_RUNTIME( "Sorry, could not determine user's settings-dir !?!");
		SettingsPath.SetTo( path.Path(), "Beam");
	
		// load the preferences set by user (if any)
		Prefs = BmPrefs::CreateInstance();
		
		// now we fetch the neccessary icons:
		// the folder-icon...
		FolderIcon = new BBitmap( BRect(0,0,15,15), B_CMAP8);
		(err = mt.SetTo("application/x-vnd.Be-Directory")) == B_OK
														|| BM_THROW_RUNTIME( BString("Beam: Could not initialize mimetype application/folder") << "\n\nError:" << strerror(err));
		(err = mt.GetIcon( FolderIcon, B_MINI_ICON)) == B_OK
														|| BM_THROW_RUNTIME( BString("Beam: Could not find icon for mimetype application/folder") << "\n\nError:" << strerror(err));
		// the mail-icon...
		MailIcon = new BBitmap( BRect(0,0,15,15), B_CMAP8);
		(err = mt.SetTo("text/x-email")) == B_OK
														|| BM_THROW_RUNTIME( BString("Beam: Could not initialize mimetype text/x-email") << "\n\nError:" << strerror(err));
		if ((err = mt.GetIcon( MailIcon, B_MINI_ICON)) != B_OK) {
			(err = mt.SetTo("text")) == B_OK
															|| BM_THROW_RUNTIME( BString("Beam: Could not initialize mimetype text") << "\n\nError:" << strerror(err));
			(err = mt.GetIcon( MailIcon, B_MINI_ICON)) == B_OK
														|| BM_THROW_RUNTIME( BString("Beam: Could not find icon for mimetype text/x-email") << "\n\nError:" << strerror(err));
		}
	
		// we fill necessary info about the standard font-height:
		be_plain_font->GetHeight( &BePlainFontHeight);

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
BmApplication::~BmApplication()
{
	delete MailFolderList;
	delete FolderIcon;
	delete Prefs;
	delete LogHandler;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmApplication::QuitRequested() {
	mIsQuitting = true;
	bool shouldQuit = inherited::QuitRequested();
	if (shouldQuit) {
		if (MailFolderList)
			MailFolderList->Store();
	} else {
		mIsQuitting = false;
	}
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
float BmApplication::FontHeight() { 
	return BePlainFontHeight.ascent + BePlainFontHeight.descent; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
float BmApplication::FontLineHeight() {
	return BePlainFontHeight.ascent 
			+ BePlainFontHeight.descent 
			+ BePlainFontHeight.leading; 
}
