/*
	BmMenuController.h
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


#ifndef _BmMenuController_h
#define _BmMenuController_h

#include <Menu.h>

#include "BmController.h"

class BmMenuController : public BMenu, public BmJobController
{
	typedef BMenu inherited;
	typedef BmJobController inheritedController;

public:

	BmMenuController( const char* label, BHandler* msgTarget, 
							BMessage& msgTemplate, BmJobModel* jobModel,
							bool skipFirstLevel=false);

	// overrides of controller-base
	BHandler* GetControllerHandler() 	{ return this; }
	void JobIsDone( bool completed);
	
	// overrides of view-base
	void MessageReceived( BMessage* msg);
	void AttachedToWindow();
	void DetachedFromWindow();

	void Shortcuts( const BmString s) 	{ mShortcuts = s; }

private:
	
	void AddItemToMenu( BmListModelItem* item, BMenu* menu,
							  bool skipThisButAddChildren=false,
							  char shortcut=0);

	BMessage mMsgTemplate;
	BmJobModel* mJobModel;
	BHandler* mMsgTarget;
	BmString mShortcuts;
	bool mSkipFirstLevel;

	// Hide copy-constructor and assignment:
	BmMenuController( const BmMenuController&);
	BmMenuController operator=( const BmMenuController&);
};


#endif
