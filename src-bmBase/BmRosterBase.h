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

class BHandler;
class BMenu;
class BMessage;

extern IMPEXPBMBASE const char* BM_ROSTER_FOLDERLIST;
extern IMPEXPBMBASE const char* BM_ROSTER_STATUSLIST;
extern IMPEXPBMBASE const char* BM_ROSTER_IDENTITYLIST;

/*------------------------------------------------------------------------------*\
	BmRosterBase
		-	abstract class that can be used by add-ons to retrieve info about
			Beam's state.
		-	The implementation lives in a derived class called BmRoster.
\*------------------------------------------------------------------------------*/
class IMPEXPBMBASE BmRosterBase {

public:
	virtual ~BmRosterBase() 				{}
	
	// native methods:
	virtual void FillMenuFromList( const char* listName, BMenu* menu, 
											 BHandler* menuTarget, 
											 BMessage* msgTemplate) = 0;
};

extern IMPEXPBMBASE BmRosterBase* BeamRoster;

#endif
