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

#include "BmMenuControllerBase.h"

extern const int32 BM_MC_MOVE_RIGHT;
extern const int32 BM_MC_SKIP_FIRST_LEVEL;
extern const int32 BM_MC_ADD_NONE_ITEM;
extern const int32 BM_MC_LABEL_FROM_MARKED;
extern const int32 BM_MC_RADIO_MODE;

/*------------------------------------------------------------------------------*\
	BmMenuController
		-	
\*------------------------------------------------------------------------------*/
class BmMenuController : public BmMenuControllerBase
{
	typedef BmMenuControllerBase inherited;
	
public:

	BmMenuController( const char* label, BHandler* msgTarget, 
							BMessage* msgTemplate, BmListModel* listModel,
							int32 flags=0);

	BmMenuController( const char* label, BHandler* msgTarget, 
							BMessage* msgTemplate,
							RebuildMenuFunc fn, int32 flags=0);
	
	~BmMenuController();

	// native methods
	void UpdateItemList();
	
protected:
	BmListModel* mListModel;

private:
	// Hide copy-constructor and assignment:
	BmMenuController( const BmMenuController&);
	BmMenuController operator=( const BmMenuController&);
};



void BmRebuildCharsetMenu( BmMenuControllerBase*);


#endif
