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


#include <MenuItem.h>

#include "BmMenuController.h"

/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::BmMenuController( const char* label, const char* name,
												BMessage& msgTemplate, BHandler* target)
	:	inherited( label)
	,	inheritedController( name)
	,	mMsgTemplate( msgTemplate)
	,	mTarget( target)
{
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::JobIsDone( bool completed) {
	if (completed) {
		BmAutolockCheckGlobal lock( DataModel()->ModelLocker());
		lock.IsLocked()	 						|| BM_THROW_RUNTIME( BmString() << ControllerName() << ":AddAllModelItems(): Unable to lock model");
		BMenuItem* old;
		while( (old = RemoveItem( (int32)0))!=NULL)
			delete old;
		BmRef<BmDataModel> modelRef( DataModel());
		BmListModel *model = dynamic_cast<BmListModel*>( modelRef.Get());
		BmModelItemMap::const_iterator iter;
		int i=0;
		for( iter = model->begin();  iter != model->end();  ++iter, ++i) {
			BmListModelItem* item = iter->second.Get();
			BMessage* msg = new BMessage( mMsgTemplate);
			msg->AddString( BmListModel::MSG_ITEMKEY, item->Key().String());
			if (item) {
				BMenuItem* menuItem;
				if (i<mShortcuts.Length())
					menuItem = new BMenuItem( item->Key().String(), msg, mShortcuts[i]);
				else
					menuItem = new BMenuItem( item->Key().String(), msg);
				menuItem->SetTarget( GetControllerHandler());
				AddItem( menuItem);
			}
		}
	}
}
