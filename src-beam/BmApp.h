/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();

	// beos-stuff
	bool QuitRequested();

	// getters
	bool IsQuitting()							{ return mIsQuitting; }

private:
	bool mIsQuitting;

	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
