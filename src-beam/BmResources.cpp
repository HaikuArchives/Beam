/*
	BmResources.cpp
		$Id$
*/

#include <Application.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <Resources.h>

#include "BmResources.h"
#include "BmUtil.h"

BmResources* BmResources::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes preferences by reading them from a file
		-	if no preference-file is found, default prefs are used
\*------------------------------------------------------------------------------*/
BmResources* BmResources::CreateInstance() {
	if (theInstance)
		return theInstance;
	else
		return theInstance = new BmResources();
}

/*------------------------------------------------------------------------------*\
	BmResources()
		-	constructor
\*------------------------------------------------------------------------------*/
BmResources::BmResources()
	:	WHITESPACE( " \t\n\r\f")
{
	BMimeType mt;
	BPath path;
	entry_ref eref;

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
	
	// Load all the needed icons from our resources:
	FetchIcons();

	// we fill necessary info about the standard font-height:
	be_plain_font->GetHeight( &BePlainFontHeight);
}

/*------------------------------------------------------------------------------*\
	~BmResources()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmResources::~BmResources()
{
	BmIconMap::iterator iter;
	for( iter = IconMap.begin(); iter != IconMap.end(); ++iter) {
		BBitmap* item = iter->second;
		delete item;
	}
	IconMap.clear();

	// and now... we do something that is AGAINST THE BE_BOOK (gasp)
	// by deleting the apps resources (no one else will do it for us...):
	delete mResources;

	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	FetchIcons()
		-	reads all icons into the public map IconMap (indexed by name), from where 
			they can be easily accessed.
\*------------------------------------------------------------------------------*/
void BmResources::FetchIcons() {
	BResources* res = mResources = be_app->AppResources();
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
float BmResources::FontHeight( const BFont* font) { 
	font_height fh;
	if (!font)
		fh = BePlainFontHeight;
	else
		font->GetHeight( &fh);

	return fh.ascent + fh.descent; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
float BmResources::FontLineHeight( const BFont* font) {
	font_height fh;
	if (!font)
		fh = BePlainFontHeight;
	else
		font->GetHeight( &fh);

	return fh.ascent 
			+ fh.descent 
			+ fh.leading; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::MailCacheFolder() {
	return GetFolder( BString( SettingsPath.Path()) << "/MailCache/", mMailCacheFolder);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::StateInfoFolder() {
	return GetFolder( BString( SettingsPath.Path()) << "/StateInfo/", mStateInfoFolder);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::GetFolder( const BString& name, BDirectory& dir) {
	if (dir.InitCheck() != B_OK) {
		status_t res = dir.SetTo( name.String());
		if (res != B_OK) {
			(res = create_directory( name.String(), 0755) || dir.SetTo( name.String())) == B_OK
													|| BM_DIE( BString("Sorry, could not create folder ")<<name<<".\n\t Going down!");
		}
	}
	return &dir;
}

