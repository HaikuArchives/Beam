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


const int32 BM_MC_MOVE_RIGHT			= 1<<0;
const int32 BM_MC_SKIP_FIRST_LEVEL	= 1<<1;
const int32 BM_MC_ADD_NONE_ITEM		= 1<<2;
const int32 BM_MC_LABEL_FROM_MARKED	= 1<<3;
const int32 BM_MC_RADIO_MODE			= 1<<4;



/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRebuildCharsetMenu( BmMenuControllerBase* menu) {
	BMenuItem* old;
	while( (old = menu->RemoveItem( (int32)0)) != NULL)
		delete old;
	
	// add all charsets to menu:
	AddCharsetMenu( menu, menu->MsgTarget(), menu->MsgTemplate()->what);
}



/********************************************************************************\
	BmMenuController
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::BmMenuController( const char* label, BHandler* msgTarget,
												BMessage* msgTemplate, 
												BmListModel* listModel,	int32 flags)
	:	inherited( label, msgTarget, msgTemplate, NULL, flags)
	,	mListModel( listModel)
{
	UpdateItemList();
}

/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::BmMenuController( const char* label, BHandler* msgTarget,
												BMessage* msgTemplate, 
												RebuildMenuFunc func, int32 flags)
	:	inherited( label, msgTarget, msgTemplate, func, flags)
	,	mListModel( NULL)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::UpdateItemList( void) {
	if (mListModel) {
		// create menu according to list-model:
		BmAutolockCheckGlobal lock( mListModel->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( "UpdateItemList(): Unable to lock model");
		BMenuItem* old;
		while( (old = RemoveItem( (int32)0))!=NULL)
			delete old;
		BFont font;
		GetFont( &font);
		AddListToMenu( mListModel, this, mMsgTemplate, mMsgTarget,
							&font, mFlags & BM_MC_SKIP_FIRST_LEVEL, 
							mFlags & BM_MC_ADD_NONE_ITEM, mShortcuts);
	}
	inherited::UpdateItemList();
}
