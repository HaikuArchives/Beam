/*
	BmResources.h

		$Id$
*/

#ifndef _BmResources_h
#define _BmResources_h

#include <map>

#include <Directory.h>
#include <Font.h>
#include <Path.h>
#include <String.h>
#include <Volume.h>

#include "Colors.h"
#include "PrefilledBitmap.h"

class BBitmap;

/*------------------------------------------------------------------------------*\
	BmResources 
		-	holds all resources needed by Beam
		- 	additionally holds the paths used within Beam
\*------------------------------------------------------------------------------*/
class BmResources {
	typedef map< BString, BBitmap*> BmIconMap;

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
										 rgb_color background = BeBackgroundGrey);
	//
	const char* WHITESPACE;
	PrefilledBitmap mRightArrow;
	PrefilledBitmap mDownArrow;

	static BmResources* theInstance;

private:
	void FetchIcons();

	//
	BmIconMap IconMap;

	// folders living under settings/Beam/:
	BDirectory mMailCacheFolder;
	BDirectory mStateInfoFolder;
	
	BResources* mResources;
};

#define TheResources BmResources::theInstance

#endif
