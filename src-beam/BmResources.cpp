/*
	BmResources.cpp
		$Id$
*/

#include <Application.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <Picture.h>
#include <Resources.h>
#include <View.h>

#include "BmBasics.h"
#include "BmResources.h"
#include "BmUtil.h"

BmResources* BmResources::theInstance = NULL;

extern uint8 CLVRightArrowData[132];
extern uint8 CLVDownArrowData[132];

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
	,	mRightArrow( BRect(0.0,0.0,10.0,10.0), B_COLOR_8_BIT, CLVRightArrowData)
	,	mDownArrow( BRect(0.0,0.0,10.0,10.0), B_COLOR_8_BIT, CLVDownArrowData)
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
	IconByName()
		-	returns the icon corresponding to the given string-id (which can be an
			identifier from the resource-file (Beam) or a mimetype.
\*------------------------------------------------------------------------------*/
BBitmap* BmResources::IconByName( const BString name) {
	BBitmap*& icon = IconMap[name];
	if (!icon) {
		BMimeType mt( name.String());
		if (mt.InitCheck() == B_OK) {
			icon = new BBitmap( BRect( 0, 0, 15, 15), B_CMAP8);
			if (mt.GetIcon( icon, B_MINI_ICON) != B_OK) {
				// the given mimetype has no icon defined, we check if the preferred app 
				// of this mimetype has an icon for this type:
				status_t err;
				char preferredApp[B_MIME_TYPE_LENGTH+1];
				if (mt.GetPreferredApp( preferredApp) != B_OK
				|| mt.SetTo( preferredApp) != B_OK
				|| (err=mt.GetIconForType( name.String(), icon, B_MINI_ICON)) != B_OK) {
					// the preferred-app doesn't exist or has no icon for this mimetype.
					// We take the icon of a generic file:
					mt.SetTo("application/octet-stream");
					if (mt.InitCheck() == B_OK) {
						if (mt.GetIcon( icon, B_MINI_ICON) != B_OK) {
							// no icon for generic file ?!? we give up...
							delete icon;
							icon = NULL;
						}
					}
				}
			}
		}
	}
	return icon;
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
float BmResources::FontBaselineOffset( const BFont* font) { 
	font_height fh;
	if (!font)
		fh = BePlainFontHeight;
	else
		font->GetHeight( &fh);

	return fh.ascent - 1; 
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

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BPicture* BmResources::CreatePictureFor( BBitmap* image, float width, float height,
													  rgb_color background) {
	BPicture* picture = new BPicture();
	BRect rect(0,0,width-1,height-1);
	BView* view = new BView( rect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( rect, B_RGBA32, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( background);
	view->SetLowColor( background);
	view->FillRect( rect, B_SOLID_LOW);
	if (image) {
		BRect imageRect = image->Bounds();
		float imageWidth = imageRect.Width();
		float imageHeight = imageRect.Height();
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap( image, BPoint( (width-imageWidth)/2.0, (height-imageHeight)/2.0));
		view->SetDrawingMode(B_OP_COPY);
	}
	view->EndPicture();
	drawImage->Unlock();
	delete drawImage;
	return picture;
}



