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
#include "BmPrefs.h"
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
BBitmap* BmResources::IconByName( const BString name) {
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
			ShowAlert( BString("FetchIcons(): Could not read icon '") << name << "'");
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
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		BFile netFile( (BString(path.Path())<<"/network").String(), B_READ_ONLY);
		if (netFile.InitCheck() == B_OK) {
			BString buffer;
			char* buf = buffer.LockBuffer(4096);
			ssize_t readSize = netFile.Read( buf, 4095);
			buffer.UnlockBuffer( readSize > 0 ? readSize : 0);
			Regexx rx;
			if (rx.exec( buffer, "HOSTNAME\\s*=\\s*(\\S+)", Regexx::nocase)) {
				mOwnFQDN = rx.match[0].atom[0];
				if (rx.exec( buffer, "DNS_DOMAIN\\s*=\\s*(\\S+)", Regexx::nocase))
					mOwnFQDN << "." << rx.match[0].atom[0];
			}
		}
	}
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
void BmResources::CheckMimeTypeFile( BString sig, time_t appModTime) {
	BPath path;
	if (find_directory( B_COMMON_SETTINGS_DIRECTORY, &path) == B_OK) {
		if (sig.Length())
			// check introduced for Dano compatibility, otherwise "mysterious things"(TM) happen:
			sig.ToLower();
		BEntry mtEntry( (BString(path.Path())<<"/beos_mime/"<<sig).String());
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
	menu->AddItem( fontMenu);
	font_family family;
	font_style style;
	if (selectedFont)
		selectedFont->GetFamilyAndStyle( &family, &style);
	BmFontMap::const_iterator iter;
	for( iter = mFontMap.begin(); iter != mFontMap.end(); ++iter) {
		const BmFontStyleVect& styles = iter->second;
		BMenu* subMenu = new BMenu( iter->first.String());
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



