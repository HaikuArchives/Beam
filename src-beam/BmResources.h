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

class BBitmap;

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
	BDirectory* MailCacheFolder();
	BDirectory* StateInfoFolder();
	BDirectory* GetFolder( const BString& name, BDirectory& dir);

	//
	typedef map< BString, BBitmap*> BmIconMap;
	BmIconMap IconMap;

	//
	BString HomePath;
	BVolume MailboxVolume;
	BPath	SettingsPath;
	//
	font_height BePlainFontHeight;
	float FontHeight();
	float FontLineHeight();
	//
	const char* WHITESPACE;

	static BmResources* theInstance;

private:
	void FetchIcons();

	// folders living under settings/Beam/:
	BDirectory mMailCacheFolder;
	BDirectory mStateInfoFolder;
};

#define TheResources BmResources::theInstance

#endif
