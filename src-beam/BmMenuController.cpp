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
void BmRebuildCharsetMenu( BmMenuController* menu) {
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
	:	inherited( label, flags & (BM_MC_RADIO_MODE|BM_MC_LABEL_FROM_MARKED),
					  flags & BM_MC_LABEL_FROM_MARKED)
	,	mMsgTarget( msgTarget)
	,	mMsgTemplate( msgTemplate)
	,	mListModel( listModel)
	,	mRebuildMenuFunc( NULL)
	,	mFlags( flags)
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
	:	inherited( label, flags & (BM_MC_RADIO_MODE|BM_MC_LABEL_FROM_MARKED),
					  flags & BM_MC_LABEL_FROM_MARKED)
	,	mMsgTarget( msgTarget)
	,	mMsgTemplate( msgTemplate)
	,	mListModel( NULL)
	,	mRebuildMenuFunc( func)
	,	mFlags( flags)
{
	UpdateItemList();
}

/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::~BmMenuController() {
	delete mMsgTemplate;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::UpdateItemList( void) {
	if (mRebuildMenuFunc) {
		// menu is created by a dedicated function:
		(*mRebuildMenuFunc)( this);
	} else if (mListModel) {
		// create menu according to list-model:
		BmAutolockCheckGlobal lock( mListModel->ModelLocker());
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "UpdateItemList(): Unable to lock model");
		BMenuItem* old;
		while( (old = RemoveItem( (int32)0))!=NULL)
			delete old;
		BFont font;
		GetFont( &font);
		AddListToMenu( mListModel, this, mMsgTemplate, mMsgTarget,
							&font, mFlags & BM_MC_SKIP_FIRST_LEVEL, 
							mFlags & BM_MC_ADD_NONE_ITEM, mShortcuts);
	}
	BMenuItem* labelItem = Superitem();
	if (labelItem) {
		// we walk down the tree to find the corresponding menu-item:
		BMenuItem* item = NULL;
		vector<BmString> itemVect;
		split( "/", labelItem->Label(), itemVect);
		BMenu* currMenu = this;
		for( uint32 i=0; currMenu && i<itemVect.size(); ++i) {
			BmString str = itemVect[i];
			item = currMenu->FindItem( str.String());
			currMenu = item 
							? item->Submenu()
							: NULL;
		}
		if (item)
			item->SetMarked( true);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::AttachedToWindow( void) {
	UpdateItemList();
	inherited::AttachedToWindow();
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BPoint BmMenuController::ScreenLocation() {
	BPoint pt = inherited::ScreenLocation();
	if (mFlags & BM_MC_MOVE_RIGHT) {
		// we are part of a real menu (no simple popup), so we have to move
		// the menu to the right edge of its controlling item:
		BMenuItem* item = Superitem();
		if (item)
			pt.x += item->Frame().Width()+2;
	}
	return pt;
}
