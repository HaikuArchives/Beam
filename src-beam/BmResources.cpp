/*
	BmResources.cpp
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


#include <Application.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Picture.h>
#include <Resources.h>
#include <View.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

// forward declaration
void ShowAlert( const BmString &text);

BmResources* BmResources::theInstance = NULL;

extern uint8 CLVRightArrowData[132];
extern uint8 CLVDownArrowData[132];

// This is the link's mouse cursor (a replica of NetPositive's link cursor).
// [zooey]: Thanks to William Kakes of Tall Hill Software for creating this!
static const uint8 url_cursor[] = { 16, 1, 1, 2,
	// This is the cursor data.
	0x00, 0x00, 0x38, 0x00, 0x24, 0x00, 0x24, 0x00,
	0x13, 0xe0,	0x12, 0x5c,	0x09, 0x2a,	0x08, 0x01,
	0x3c, 0x21,	0x4c, 0x71,	0x42, 0x71,	0x30, 0xf9,
	0x0c, 0xf9,	0x02, 0x00,	0x01, 0x00,	0x00, 0x00,
	// This is the cursor mask.
	0x00, 0x00,	0x38, 0x00,	0x3c, 0x00,	0x3c, 0x00,
	0x1f, 0xe0,	0x1f, 0xfc, 0x0f, 0xfe,	0x0f, 0xff,
	0x3f, 0xff,	0x7f, 0xff,	0x7f, 0xff,	0x3f, 0xff,
	0x0f, 0xff,	0x03, 0xfe,	0x01, 0xf8,	0x00, 0x00,
};

const char* const BmResources::BM_MSG_FONT_FAMILY = "fontfamily";
const char* const BmResources::BM_MSG_FONT_STYLE = "fontstyle";
const char* const BmResources::BM_MSG_FONT_SIZE = "fontsize";

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
	,	mUrlCursor( url_cursor)
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
	if (BeamInTestMode)
		// in order to avoid clobbering precious settings,
		// we use a different settings-folder  in tesmode:
		SettingsPath.SetTo( path.Path(), "Beam_Test");
	else
		// standard settings folder:
		SettingsPath.SetTo( path.Path(), "Beam");
	
	// Load all the needed icons from our resources:
	FetchIcons();

	// Load all font-info:
	FetchFonts();

	// Determine our own FQDN from network settings file, if possible:
	FetchOwnFQDN();
	
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
	for( iter = mIconMap.begin(); iter != mIconMap.end(); ++iter) {
		BBitmap* item = iter->second;
		delete item;
	}
	mIconMap.clear();

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
BBitmap* BmResources::IconByName( const BmString name) {
	BBitmap*& icon = mIconMap[name];
	if (!icon) {
		BMimeType mt( name.String());
		if (mt.InitCheck() == B_OK) {
			icon = new BBitmap( BRect( 0, 0, 15, 15), B_CMAP8);
			if (mt.GetIcon( icon, B_MINI_ICON) != B_OK) {
				// the given mimetype has no icon defined, we check if the preferred app 
				// of this mimetype has an icon for this type:
				BMimeType preferredApp;
				char preferredAppName[B_MIME_TYPE_LENGTH+1];
				if (mt.GetPreferredApp( preferredAppName) != B_OK
				|| preferredApp.SetTo( preferredAppName) != B_OK
				|| preferredApp.GetIconForType( name.String(), icon, B_MINI_ICON) != B_OK) {
					// the preferred-app doesn't exist or has no icon for this mimetype.
					// We check if the mimetype's supertype has an icon:
					BMimeType supertype;
					mt.GetSupertype( &supertype);
					if (supertype.GetIcon( icon, B_MINI_ICON) != B_OK) {
						// no icon for the supertype, tough!
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
			BM_SHOWERR( BmString("FetchIcons(): Could not read icon '") << name << "'");
			continue;
		}
		BArchivable* theObj = NULL;
		BMessage msg;
		if (msg.Unflatten( data) == B_OK) {
			theObj = instantiate_object( &msg);
			mIconMap[name] = dynamic_cast< BBitmap*>( theObj);
		}
	}
}

/*------------------------------------------------------------------------------*\
	FetchOwnFQDN()
		-	fetches hostname and domainname from network settings and build FQDN 
			from that.
\*------------------------------------------------------------------------------*/
void BmResources::FetchOwnFQDN() {
	BmString buffer;
	Regexx rx;
#ifdef BEAM_FOR_BONE
	FetchFile( "/etc/hostname", mOwnFQDN);
	mOwnFQDN.RemoveSet( " \n\r\t");
	if (!mOwnFQDN.Length())
		mOwnFQDN = "bepc";
	FetchFile( "/etc/resolv.conf", buffer);
	if (rx.exec( buffer, "DOMAIN\\s*(\\S*)", Regexx::nocase)
	&& rx.match[0].atom[0].Length())
		mOwnFQDN << "." << rx.match[0].atom[0];
	else
		mOwnFQDN << "." << time( NULL) << ".fake";
#else
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		FetchFile( BmString(path.Path())<<"/network", buffer);
		if (rx.exec( buffer, "HOSTNAME\\s*=[ \\t]*(\\S*)", Regexx::nocase)) {
			mOwnFQDN = rx.match[0].atom[0];
			if (!mOwnFQDN.Length())
				mOwnFQDN = "bepc";
			if (rx.exec( buffer, "DNS_DOMAIN\\s*=[ \\t]*(\\S*)", Regexx::nocase)
			&& rx.match[0].atom[0].Length())
				mOwnFQDN << "." << rx.match[0].atom[0];
			else
				mOwnFQDN << "." << time( NULL) << ".fake";
		}
	}
#endif
	if (!mOwnFQDN.Length())
		mOwnFQDN << "bepc." << time( NULL) << ".fake";
	mOwnFQDN.RemoveSet( "\r\n");
}

/*------------------------------------------------------------------------------*\
	FetchFonts()
		-	reads info about all existing font families & styles into the font-map.
\*------------------------------------------------------------------------------*/
void BmResources::FetchFonts() {
	int32 numFamilies = count_font_families();
	for ( int32 i = 0; i < numFamilies; i++ ) {
		font_family family;
		uint32 flags;
		if ( get_font_family(i, &family, &flags) == B_OK ) {
			int32 numStyles = count_font_styles(family);
			for ( int32 j = 0; j < numStyles; j++ ) {
				font_style style;
				if ( get_font_style(family, j, &style, &flags) == B_OK ) {
					mFontMap[family].push_back( style);
				}
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	CheckMimeTypeFile( sig, appModTime)
		-	checks age of our own mimetype-file 
			(.../settings/beos_mime/application/x-vnd.zooey-beam)
			and removes the file if it's older than the application file.
\*------------------------------------------------------------------------------*/
void BmResources::CheckMimeTypeFile( BmString sig, time_t appModTime) {
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		sig.ToLower();
		BEntry mtEntry( (BmString(path.Path())<<"/beos_mime/"<<sig).String());
		if (mtEntry.InitCheck() == B_OK) {
			time_t modTime;
			if (mtEntry.GetModificationTime( &modTime) == B_OK) {
				if (appModTime > modTime) {
					// application is newer than mimetype-file, we simply remove
					// that and let BeOS recreate it when needed. The new version
					// will then contain all current icons, etc.
					mtEntry.Remove();
				}
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	AddFontSubmenuTo( menu)
		-	reads info about all existing font families & styles into the font-map.
\*------------------------------------------------------------------------------*/
void BmResources::AddFontSubmenuTo( BMenu* menu, BHandler* target, 
												BFont* selectedFont) {
	if (!menu)
		return;
	BMenu* fontMenu = new BMenu( "Select Font...");
	BFont font( *be_plain_font);
	font.SetSize( 10);
	fontMenu->SetFont( &font);
	menu->AddItem( fontMenu);
	font_family family;
	font_style style;
	if (selectedFont)
		selectedFont->GetFamilyAndStyle( &family, &style);
	BmFontMap::const_iterator iter;
	for( iter = mFontMap.begin(); iter != mFontMap.end(); ++iter) {
		const BmFontStyleVect& styles = iter->second;
		BMenu* subMenu = new BMenu( iter->first.String());
		subMenu->SetFont( &font);
		for( uint32 i=0; i<styles.size(); ++i) {
			BMessage* msg = new BMessage( BM_FONT_SELECTED);
			msg->AddString( BM_MSG_FONT_FAMILY, iter->first.String());
			msg->AddString( BM_MSG_FONT_STYLE, styles[i].String());
			BMenuItem* item = new BMenuItem( styles[i].String(), msg);
			if (target)
				item->SetTarget( target);
			if (selectedFont && iter->first==family && styles[i]==style)
				item->SetMarked( true);
			subMenu->AddItem( item);
		}
		fontMenu->AddItem( subMenu);
		BMenuItem* familyItem = fontMenu->FindItem( family);
		if (familyItem) {
			if (selectedFont && iter->first==family)
				familyItem->SetMarked( true);
		}
	}
	BMenu* sizeMenu = new BMenu( "Select Fontsize...");
	sizeMenu->SetFont( &font);
	const char* sizes[] = { "8","9","10","11","12","14","16","18","20","24",NULL};
	for( int i=0; sizes[i]!=NULL; ++i) {
		BMessage* msg = new BMessage( BM_FONTSIZE_SELECTED);
		int16 size = atoi(sizes[i]);
		msg->AddInt16( BM_MSG_FONT_SIZE, size);
		BMenuItem* item = new BMenuItem( sizes[i], msg);
		if (target)
			item->SetTarget( target);
		if (selectedFont && selectedFont->Size()==size)
			item->SetMarked( true);
		sizeMenu->AddItem( item);
	}
	menu->AddItem( sizeMenu);
}

/*------------------------------------------------------------------------------*\
	FontBaselineOffset( font)
		-	returns the baseline offset for the given font
		-	the baselne offset is the distance of the baseline (measured in pixels)
			from the top of the (virtual) font-rectangle
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
	FontHeight( font)
		-	returns the height (ascent+descent) for the given font in pixels
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
	FontLineHeight( font)
		-	returns the line-height (ascent+descent+leading) for the given font 
			in pixels
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
	MailCacheFolder()
		-	returns a Directory* that is set to the folder where Beam's mail-cache
			lives (normally .../settings/Beam/MailCache/)
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::MailCacheFolder() {
	return GetFolder( BmString( SettingsPath.Path()) << "/MailCache/", mMailCacheFolder);
}

/*------------------------------------------------------------------------------*\
	StateInfoFolder()
		-	returns a Directory* that is set to the folder where Beam's state-info
			lives (normally .../settings/Beam/StateInfo/)
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::StateInfoFolder() {
	return GetFolder( BmString( SettingsPath.Path()) << "/StateInfo/", mStateInfoFolder);
}

/*------------------------------------------------------------------------------*\
	GetFolder( name, dir)
		-	initializes the given BDirectory dir to the given path name
		-	if the directory does not yet exist, it is created
		-	a pointer to the initialized directory is returned, so you probably
			don't want to delete that
\*------------------------------------------------------------------------------*/
BDirectory* BmResources::GetFolder( const BmString& name, BDirectory& dir) {
	if (dir.InitCheck() != B_OK) {
		status_t res = dir.SetTo( name.String());
		if (res != B_OK) {
			if ((res = create_directory( name.String(), 0755) || dir.SetTo( name.String())) != B_OK) {
				BM_SHOWERR( BmString("Sorry, could not create folder ")<<name<<".\n\t Going down!");
				exit( 10);
			}
		}
	}
	return &dir;
}

/*------------------------------------------------------------------------------*\
	CreatePictureFor( image, width, height, background, transparentBack)
		-	creates and returns a BPicture for the given image
		-	param transparentBack decides whether or not the background shall be
			transparent
		-	the caller takes ownership of the returned BPicture
\*------------------------------------------------------------------------------*/
BPicture* BmResources::CreatePictureFor( BBitmap* image, float width, float height,
													  rgb_color background,
													  bool transparentBack) {
	BPicture* picture = new BPicture();
	BRect rect(0,0,width-1,height-1);
	BView* view = new BView( rect, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( rect, B_RGBA32, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( background);
	if (!transparentBack) {
		view->SetLowColor( background);
		view->FillRect( rect, B_SOLID_LOW);
	}
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
