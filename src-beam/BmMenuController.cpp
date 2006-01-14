/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
