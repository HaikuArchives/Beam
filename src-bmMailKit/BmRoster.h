/*
	BmRoster.h
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
