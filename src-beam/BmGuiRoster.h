/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmGuiRoster_h
#define _BmGuiRoster_h

#include "BmString.h"

#include "BmRosterBase.h"

/*------------------------------------------------------------------------------*\
	BmGuiRoster
		-	a class that can be used by add-ons to retrieve info about
			Beam's state.
\*------------------------------------------------------------------------------*/
class BmGuiRoster : public BmGuiRosterBase {

public:
	BmGuiRoster();
	virtual ~BmGuiRoster()						{}
	
	// overrides of base class:
	bool AskUserForPwd( const BmString& text, BmString& pwd);
	bool AskUserForPopAcc( const BmString& accName, BmString& popAccName);

	bool IsEmailKnown( const BmString& email);

	void RebuildCharsetMenu( BmMenuControllerBase* menu);
	void AddCharsetMenu( BMenu* menu, BHandler* target, int32 msgType);
	void RebuildFilterMenu( BmMenuControllerBase* menu);
	void RebuildFilterChainMenu( BmMenuControllerBase* menu);
	void RebuildFolderMenu( BmMenuControllerBase* menu);
	void RebuildIdentityMenu( BmMenuControllerBase* menu);
	void RebuildLogMenu( BmMenuControllerBase* menu);
	void RebuildPeopleMenu( BmMenuControllerBase* menu);
	void RebuildRecvAccountMenu( BmMenuControllerBase* menu);
	void RebuildSignatureMenu( BmMenuControllerBase* menu);
	void RebuildSmtpAccountMenu( BmMenuControllerBase* menu);
	void RebuildStatusMenu( BmMenuControllerBase* menu);
	void RebuildMailRefFilterMenu( BmMenuControllerBase* menu);
	void RebuildMailRefViewFilterMenu( BmMenuControllerBase* menu);

	int32 ShowAlert( const BmString& text, const char* btn1,
						  const char* btn2, const char* btn3);
};


#endif
