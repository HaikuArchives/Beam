/*
	BmMenuController.cpp
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


#include <MenuBar.h>
#include <MenuItem.h>

#include "split.hh"
using namespace regexx;

#include "BmDataModel.h"
#include "BmGuiUtil.h"
#include "BmMenuController.h"


/********************************************************************************\
	BmMenuController
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::BmMenuController( const char* label, BHandler* msgTarget,
												BMessage* msgTemplate, 
												BmRebuildMenuFunc func, int32 flags)
	:	inherited( label, msgTarget, msgTemplate, func, flags)
{
}

/*------------------------------------------------------------------------------*\
	~BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::~BmMenuController() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::UpdateItemList( void) {
	// lock window first...
	BLooper* win = Looper();
	if (win)
		win->Lock();
	try {
		inherited::UpdateItemList();
	} catch(...) {
		if (win)
			win->Unlock();
		throw;
	}
	if (win)
		win->Unlock();
}
