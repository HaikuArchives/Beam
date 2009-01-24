/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMenuController_h
#define _BmMenuController_h

#include "BmMenuControllerBase.h"

/*------------------------------------------------------------------------------*\
	BmMenuController
		-	
\*------------------------------------------------------------------------------*/
class BmMenuController : public BmMenuControllerBase
{
	typedef BmMenuControllerBase inherited;
	
public:

	BmMenuController( const char* label, BHandler* msgTarget, 
							BMessage* msgTemplate,
							BmRebuildMenuFunc fn, int32 flags=0);
	
	~BmMenuController();

	// overrides of base-methods:
	void UpdateItemList();
	void Clear();
	
private:
	// Hide copy-constructor and assignment:
	BmMenuController( const BmMenuController&);
	BmMenuController operator=( const BmMenuController&);
};

#endif
