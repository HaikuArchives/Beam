/*
	BmApp.cpp
		$Id$
*/

#include <FindDirectory.h>
#include <Resources.h>

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
	,	WHITESPACE( " \t\n\r\f")
	,	MailFolderList( NULL)
	,	MailFolderView( NULL)
	,	MailRefView( NULL)
	,	MainWindow( NULL)
	,	mIsQuitting( false)
{
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
		
		// Load all the needed icons from our resources:
		FetchIcons();

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
	FetchIcons()
		-	reads all icons into the public map IconMap (indexed by name), from where 
			they can be easily accessed.
\*------------------------------------------------------------------------------*/
void BmApplication::FetchIcons() {
	BResources* res = AppResources();
	type_code iconType = 'BBMP';
	res->PreloadResourceType( iconType);
	int32 id;
	const char* name;
	size_t length;
	char *data;
	for( int32 i=0; res->GetResourceInfo( iconType, i, &id, &name, &length); i++) {
		if (!(data = (char*)res->LoadResource( iconType, id, &length))) {
			ShowAlert( BString("FetchIcons(): Could not read icon '") << name << "'");
			continue;
		}
		BArchivable* theObj = NULL;
		BMessage msg;
		if (msg.Unflatten( data) == B_OK) {
			theObj = instantiate_object( &msg);
			IconMap[name] = dynamic_cast< BBitmap*>( theObj);
		}
	}
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
