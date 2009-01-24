/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
extern IMPEXPBMBASE BLooper* TheJobMetaController;

class IMPEXPBMBASE BmGuiRosterBase {

public:
	BmGuiRosterBase() 						{}
	virtual ~BmGuiRosterBase() 			{}
	
	// native methods:
	BLooper* JobMetaController();

	virtual bool AskUserForPwd( const BmString& text, BmString& pwd) = 0;
	virtual bool AskUserForPopAcc( const BmString& accName, 
											 BmString& popAccName) = 0;

	virtual bool IsEmailKnown( const BmString& email) = 0;

	virtual void RebuildCharsetMenu( BmMenuControllerBase* menu) = 0;
	virtual void AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType) = 0;
	virtual void RebuildFilterMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildFilterChainMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildFolderMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildIdentityMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildLogMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildPeopleMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildRecvAccountMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildSignatureMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildSmtpAccountMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildStatusMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildMailRefFilterMenu( BmMenuControllerBase* menu) = 0;
	virtual void RebuildMailRefViewFilterMenu( BmMenuControllerBase* menu) = 0;

	virtual int32 ShowAlert( const BmString& text, const char* btn1,
									 const char* btn2, const char* btn3) = 0;
};

extern IMPEXPBMBASE BmGuiRosterBase* BeamGuiRoster;

#endif
