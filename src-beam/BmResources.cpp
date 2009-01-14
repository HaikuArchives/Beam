/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <map>
#include <vector>

#include <Application.h>
#include <Bitmap.h>
#include <FindDirectory.h>
#include <MenuItem.h>
#include <Picture.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <View.h>

#include "CLVListItem.h"

#include "BmBasics.h"
#include "BmBitmapHandle.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRosterBase.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

BmResources* BmResources::theInstance = NULL;

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

typedef map< BmString, BmBitmapHandle*> BmIconMap;
typedef vector< BmString> BmFontStyleVect;
typedef map< BmString, BmFontStyleVect> BmFontMap;
//
static BmIconMap mIconMap;
static BmFontMap mFontMap;

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
	:	mUrlCursor( url_cursor)
{
	// Load all font-info:
	FetchFonts();

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
		BmBitmapHandle* item = iter->second;
		delete item->bitmap;
		delete item;
	}
	mIconMap.clear();

	// and now... we do something that is AGAINST THE BE_BOOK (gasp)
	// by deleting the apps resources (no one else will do it for us...):
	delete mResources;

	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	InitializeWithPrefs()
		-	do final initialization, this time we are sure ThePrefs is valid
\*------------------------------------------------------------------------------*/
void BmResources::InitializeWithPrefs()
{
	// Load all the needed icons from our resources:
	FetchIcons();
	// set default icons for expanders:
	CLVListItem::SetDefaultExpanderBitmaps(
		IconByName("Expander_Down"),
		IconByName("Expander_Right")
	);
}

/*------------------------------------------------------------------------------*\
	IconByName()
		-	returns the icon corresponding to the given string-id (which can be an
			identifier from the resource-file (Beam) or a mimetype.
\*------------------------------------------------------------------------------*/
BmBitmapHandle* BmResources::IconByName( const BmString name) {
	BmBitmapHandle* icon = mIconMap[name];
	if (!icon || !icon->bitmap) {
		BMimeType mt( name.String());
		BBitmap* bitmap = NULL;
		if (mt.InitCheck() == B_OK) {
			bitmap = new BBitmap( BRect( 0, 0, 15, 15), B_CMAP8);
			if (mt.GetIcon( bitmap, B_MINI_ICON) != B_OK) {
				// the given mimetype has no icon defined, we check if the 
				// preferred app of this mimetype has an icon for this type:
				BMimeType preferredApp;
				char preferredAppName[B_MIME_TYPE_LENGTH+1];
				if (mt.GetPreferredApp( preferredAppName) != B_OK
				|| preferredApp.SetTo( preferredAppName) != B_OK
				|| preferredApp.GetIconForType( name.String(), bitmap, 
														  B_MINI_ICON) != B_OK) {
					// the preferred-app doesn't exist or has no icon for this
					// mimetype. We check if the mimetype's supertype has an icon:
					BMimeType supertype;
					mt.GetSupertype( &supertype);
					if (supertype.GetIcon( bitmap, B_MINI_ICON) != B_OK) {
						// no icon for the supertype, tough!
						// We take the icon of a generic file:
						mt.SetTo("application/octet-stream");
						if (mt.InitCheck() == B_OK) {
							if (mt.GetIcon( bitmap, B_MINI_ICON) != B_OK) {
								// no icon for generic file ?!? we give up...
								delete bitmap;
								bitmap = NULL;
							}
						}
					}
				}
			}
		}
		if (!icon)
			mIconMap[name] = icon = new BmBitmapHandle();
		icon->bitmap = bitmap;
	}
	return icon;
}

/*------------------------------------------------------------------------------*\
	FetchIcons()
		-	reads all icons into the public map IconMap (indexed by name), from
			where they can be easily accessed.
\*------------------------------------------------------------------------------*/
void BmResources::FetchIcons() {
	BResources* res = mResources = be_app->AppResources();
	type_code iconType = 'BBMP';
	res->PreloadResourceType( iconType);
	int32 id;
	const char* name;
	size_t length;
	char *data;
	BmString iconPath = ThePrefs->GetString("IconPath");
	BTranslatorRoster pngTR;
	// first try "local" version of PNGTranslator...
	pngTR.AddTranslators( "/boot/home/config/add-ons/Translators/PNGTranslator");
	// AddTranslators() *always* returns B_OK, even if translator wasn't found,
	// so we fetch info about loaded translators manually:
	translator_id* trIDs = NULL;
	int32 trCount=0;
	pngTR.GetAllTranslators(&trIDs, &trCount);
	delete [] trIDs;
	if (!trCount)
		// no translator found, try "system" version of PNGTranslator...
		pngTR.AddTranslators("/system/add-ons/Translators/PNGTranslator");
	for( 	int32 i=0; 
			res->GetResourceInfo( iconType, i, &id, &name, &length); i++) {
		BmString picFile = iconPath + "/" + name;
		BBitmap* bitmap = BTranslationUtils::GetBitmap( picFile.String(), &pngTR);
		if (!bitmap)
			// PNG not available (or file is no PNG), try any translator:
			bitmap = BTranslationUtils::GetBitmap( picFile.String());
		if (!bitmap) {
			// load default pic from resources:
			if (!(data = (char*)res->LoadResource( iconType, id, &length))) {
				BM_SHOWERR( BmString("FetchIcons(): Could not read icon '") << name 
									<< "'");
				continue;
			}
			BArchivable* theObj = NULL;
			BMessage msg;
			if (msg.Unflatten( data) == B_OK) {
				theObj = instantiate_object( &msg);
				bitmap = dynamic_cast< BBitmap*>( theObj);
			}
		}
		BmIconMap::iterator iter = mIconMap.find(name);
		if (iter != mIconMap.end())
			iter->second->bitmap = bitmap;
		else
			mIconMap[name] = new BmBitmapHandle(bitmap);
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
	AddFontSubmenuTo( menu)
		-	reads info about all existing font families & styles into the font-map.
\*------------------------------------------------------------------------------*/
void BmResources::AddFontSubmenuTo( BMenu* menu, BHandler* target, 
												BFont* selectedFont) {
	if (!menu)
		return;
	BMenu* fontMenu = new BMenu( "Select Font");
	BFont font( *be_plain_font);
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
	BMenu* sizeMenu = new BMenu( "Select Fontsize");
	sizeMenu->SetFont( &font);
	const char* sizes[] 
		= { "8","9","10","11","12","13","14","15","16","18","20","24",NULL };
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
	CreatePictureFor( image, width, height, background, transparentBack)
		-	creates and returns a BPicture for the given image
		-	param transparentBack decides whether or not the background shall be
			transparent
		-	the caller takes ownership of the returned BPicture
\*------------------------------------------------------------------------------*/
BPicture* BmResources::CreatePictureFor( BBitmap* image, 
													  float width, float height,
													  bool transparentBack, 
													  BmPicFrameType frameType) {
	BPicture* picture = new BPicture();
	BRect r(0,0,width-1,height-1);
	BView* view = new BView( r, NULL, B_FOLLOW_NONE, 0);
	BBitmap* drawImage = new BBitmap( r, B_RGBA32, true);
	drawImage->AddChild( view);
	drawImage->Lock();
	view->BeginPicture( picture);
	view->SetViewColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
	if (!transparentBack) {
		view->SetLowColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
		view->FillRect( r, B_SOLID_LOW);
	}
	if (frameType == BmPicFrame_ActionButton) {
		view->SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
		view->StrokeLine( r.RightTop(), r.RightBottom());
		view->StrokeLine( r.LeftTop(), r.LeftBottom());
		view->StrokeLine( r.LeftBottom(), r.RightBottom());
		view->SetHighColor( ui_color( B_UI_SHINE_COLOR));
		view->StrokeLine( BPoint(1.0,0.0), BPoint(r.right-1,0.0));
		view->StrokeLine( BPoint(1.0,0.0), BPoint(1.0,r.bottom));
	}
	if (image) {
		BRect imageRect = image->Bounds();
		float imageWidth = imageRect.Width();
		float imageHeight = imageRect.Height();
		view->SetDrawingMode(B_OP_OVER);
		view->DrawBitmap( image, BPoint( (width-imageWidth)/2.0, 
													(height-imageHeight)/2.0));
		view->SetDrawingMode(B_OP_COPY);
	}
	view->EndPicture();
	drawImage->Unlock();
	delete drawImage;
	return picture;
}
