/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>
#include <Rect.h>
#include <String.h>

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();

	// native methods:
	BRect ScreenFrame();
	bool HandlesMimetype( const BString mimetype);

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();

	// getters
	bool IsQuitting()							{ return mIsQuitting; }

	BString BmAppVersion;
	BString BmAppName;
	BString BmAppNameWithVersion;

private:
	bool mIsQuitting;

	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
