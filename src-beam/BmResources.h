/*
	BmResources.h

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


#ifndef _BmResources_h
#define _BmResources_h

#include <map>
#include <vector>

#include <Cursor.h>
#include <Directory.h>
#include <Font.h>
#include <Menu.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>

#include "Colors.h"
#include "PrefilledBitmap.h"

class BBitmap;
class BPicture;

#define BM_FONT_SELECTED 'bmFN'
#define BM_FONTSIZE_SELECTED 'bmFS'

/*------------------------------------------------------------------------------*\
	BmResources 
		-	holds all resources needed by Beam
		- 	additionally holds the paths used within Beam
\*------------------------------------------------------------------------------*/
class BmResources {
	typedef map< BString, BBitmap*> BmIconMap;
	typedef vector< BString> BmFontStyleVect;
	typedef map< BString, BmFontStyleVect> BmFontMap;

public:
	// creator-func, c'tors and d'tor:
	static BmResources* CreateInstance();
	BmResources( void);
	~BmResources();

	// native methods:
	BDirectory* MailCacheFolder();
	BDirectory* StateInfoFolder();
	BDirectory* GetFolder( const BString& name, BDirectory& dir);

	BBitmap* IconByName( const BString name);
	//
	BString HomePath;
	BVolume MailboxVolume;
	BPath	SettingsPath;
	//
	font_height BePlainFontHeight;
	float FontBaselineOffset( const BFont* font=NULL);
	float FontHeight( const BFont* font=NULL);
	float FontLineHeight( const BFont* font=NULL);
	//
	BPicture* CreatePictureFor( BBitmap* image, float width, float height,
										 rgb_color background = BeBackgroundGrey,
										 bool transparentBack = false);
	//
	void CheckMimeTypeFile( BString sig, time_t appModTime);
	//
	void AddFontSubmenuTo( BMenu* menu, BHandler* target=NULL, 
								  BFont* selectedFont=NULL);
	//
	const char* WHITESPACE;
	PrefilledBitmap mRightArrow;
	PrefilledBitmap mDownArrow;
	BString mOwnFQDN;
	//
	BCursor mUrlCursor;

	static BmResources* theInstance;
	
	static const char* const BM_MSG_FONT_FAMILY = "fontfamily";
	static const char* const BM_MSG_FONT_STYLE = "fontstyle";
	static const char* const BM_MSG_FONT_SIZE = "fontsize";
	
private:
	void FetchIcons();
	void FetchOwnFQDN();
	void FetchFonts();

	//
	BmIconMap mIconMap;
	BmFontMap mFontMap;

	// folders living under settings/Beam/:
	BDirectory mMailCacheFolder;
	BDirectory mStateInfoFolder;
	
	BResources* mResources;
};

#define TheResources BmResources::theInstance

#endif
