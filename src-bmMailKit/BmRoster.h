/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmRoster_h
#define _BmRoster_h

#include <Directory.h>

#include "BmString.h"

#include "BmApp.h"
#include "BmRosterBase.h"

/*------------------------------------------------------------------------------*\
	BmRoster
		-	a class that can be used by other classes and add-ons to 
			retrieve info about bmMailKit's state.
\*------------------------------------------------------------------------------*/
class BmRoster : public BmRosterBase {

public:
	BmRoster();
	virtual ~BmRoster()						{}
	
	// overrides of base class:
	bool IsQuitting()							{ return bmApp->IsQuitting(); }

	const char* AppNameWithVersion()		{ return bmApp->BmAppNameWithVersion.String(); }

	const char* AppPath()					{ return bmApp->AppPath().String(); }
	const char* SettingsPath()				{ return mSettingsPath.String(); }

	BDirectory* MailCacheFolder()			{ return &mMailCacheFolder; }
	BDirectory* StateInfoFolder()			{ return &mStateInfoFolder; }

	const char* OwnFQDN()					{ return mOwnFQDN.String(); }

private:
	void FetchOwnFQDN();

	BDirectory mMailCacheFolder;
	BDirectory mStateInfoFolder;

	BmString mSettingsPath;
	BmString mOwnFQDN;
};


#endif
