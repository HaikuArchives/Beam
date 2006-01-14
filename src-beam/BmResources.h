/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmResources_h
#define _BmResources_h

#include <Cursor.h>
#include <Font.h>
#include "BmString.h"

#include "Colors.h"
#include "PrefilledBitmap.h"

class BmBitmapHandle;
class BBitmap;
class BMenu;
class BPicture;

enum { 
	BM_FONT_SELECTED 		= 'bmFN',
	BM_FONTSIZE_SELECTED = 'bmFS'
};

/*------------------------------------------------------------------------------*\
	BmResources 
		-	holds all resources needed by Beam
		- 	additionally holds the paths used within Beam
\*------------------------------------------------------------------------------*/
class BmResources {

public:
	// creator-func, c'tors and d'tor:
	static BmResources* CreateInstance();
	BmResources( void);
	~BmResources();

	// native methods:
	void InitializeWithPrefs();

	BmBitmapHandle* IconByName( const BmString name);
	//
	font_height BePlainFontHeight;
	float FontBaselineOffset( const BFont* font=NULL);
	float FontHeight( const BFont* font=NULL);
	float FontLineHeight( const BFont* font=NULL);
	//
	BPicture* CreatePictureFor( BBitmap* image, float width, float height,
										 bool transparentBack = false);
	//
	void AddFontSubmenuTo( BMenu* menu, BHandler* target=NULL, 
								  BFont* selectedFont=NULL);
	//
	PrefilledBitmap mRightArrow;
	PrefilledBitmap mDownArrow;
	//
	BCursor mUrlCursor;

	static BmResources* theInstance;
	
	static const char* const BM_MSG_FONT_FAMILY;
	static const char* const BM_MSG_FONT_STYLE;
	static const char* const BM_MSG_FONT_SIZE;
	
private:
	void FetchIcons();
	void FetchOwnFQDN();
	void FetchFonts();

	BResources* mResources;
};

#define TheResources BmResources::theInstance

#endif
