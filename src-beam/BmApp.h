/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>
#include <Rect.h>

extern const char* BmAppVersion;
extern const char* BmAppName;

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();

	// native methods:
	BRect ScreenFrame();

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();

	// getters
	bool IsQuitting()							{ return mIsQuitting; }

private:
	bool mIsQuitting;

	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
