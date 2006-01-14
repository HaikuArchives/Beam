/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>

#include "BmMailKit.h"

#include "BmString.h"

class BLocker;
class BView;

class IMPEXPBMMAILKIT BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig, bool testModeRequested);
	~BmApplication();

	// native methods:

	// beos-stuff
	thread_id Run();

	// getters
	inline bool IsQuitting()				{ return mIsQuitting; }
	inline BLocker* StartupLocker()		{ return mStartupLocker; }
	inline const BmString& AppPath()		{ return mAppPath; }

	BmString BmAppVersion;
	BmString BmAppName;
	BmString BmAppNameWithVersion;

protected:
	status_t mInitCheck;
	bool mIsQuitting;

	BLocker* mStartupLocker;
	
	BmString mAppPath;
	
	static int InstanceCount;
};

extern BmApplication* bmApp;

#endif
