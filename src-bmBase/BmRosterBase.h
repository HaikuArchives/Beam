/*
	BmRosterBase.h

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


#ifndef _BmRosterBase_h
#define _BmRosterBase_h

#include <time.h>

#include "BmBase.h"
#include "BmString.h"

class BDirectory;
class BHandler;
class BLooper;
class BMenu;
class BMessage;

class BmMenuControllerBase;

/*------------------------------------------------------------------------------*\
	BmRosterBase
		-	abstract class that can be used by other classes and add-ons to 
			retrieve info about bmMailKit's state.
		-	The implementation lives in a derived class called BmRoster.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmRosterBase {

public:
	BmRosterBase()								{}
	virtual ~BmRosterBase()					{}
	
	// native methods:
	virtual bool IsQuitting() = 0;

	virtual const char* AppNameWithVersion() = 0;

	virtual const char* AppPath() = 0;
	virtual const char* SettingsPath() = 0;

	virtual BDirectory* MailCacheFolder() = 0;
	virtual BDirectory* StateInfoFolder() = 0;

	virtual const char* OwnFQDN() = 0;
};

extern IMPEXPBMBASE BmRosterBase* BeamRoster;


/*------------------------------------------------------------------------------*\
	BmGuiRosterBase
		-	abstract class that can be used by other classes and add-ons to 
			retrieve info from the user (through a GUI) or fill menus.
		-	The implementation lives in a derived class called BmGuiRoster.
\*------------------------------------------------------------------------------*/
extern BLooper* TheJobMetaController;

class IMPEXPBMBASE BmGuiRosterBase {

public:
	BmGuiRosterBase() 						{}
	virtual ~BmGuiRosterBase() 			{}
	
	// native methods:
	BLooper* JobMetaController()			{ return TheJobMetaController; }

	virtual bool AskUserForPwd( const BmString& text, BmString& pwd) = 0;
	virtual bool AskUserForPopAcc( const BmString& accName, 
											 BmString& popAccName) = 0;

	virtual void RebuildCharsetMenu( BmMenuControllerBase* menu) = 0;
	virtual void AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType) = 0;
	virtual void RebuildFilterMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildFilterChainMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildFolderMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildIdentityMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildLogMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildPeopleMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildPopAccountMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildSignatureMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildSmtpAccountMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildStatusMenu( BmMenuControllerBase* menu) = 0;
};

extern IMPEXPBMBASE BmGuiRosterBase* BeamGuiRoster;

#endif
