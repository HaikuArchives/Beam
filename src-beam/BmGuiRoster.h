/*
	BmGuiRoster.h
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
	void RebuildPopAccountMenu( BmMenuControllerBase* menu);
	void RebuildSignatureMenu( BmMenuControllerBase* menu);
	void RebuildSmtpAccountMenu( BmMenuControllerBase* menu);
	void RebuildStatusMenu( BmMenuControllerBase* menu);
};


#endif
