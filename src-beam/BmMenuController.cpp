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
BmMenuController::BmMenuController( const char* label, BHandler* msgTarget,
												BMessage& msgTemplate, BmJobModel* jobModel,
												bool skipFirstLevel)
	:	inherited( label)
	,	inheritedController( label)
	,	mMsgTemplate( msgTemplate)
	,	mMsgTarget( msgTarget)
	,	mJobModel( jobModel)
	,	mSkipFirstLevel( skipFirstLevel)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::AttachedToWindow( void) {
	// connect to the list-model:
	AttachModel( mJobModel);
	JobIsDone( true);
//	StartJob( mJobModel, false);
	inherited::AttachedToWindow();
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::DetachedFromWindow( void) {
	// disconnect from list-model:
	DetachModel();
	inherited::DetachedFromWindow();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOB_DONE:
			case BM_LISTMODEL_ADD:
			case BM_LISTMODEL_UPDATE:
			case BM_LISTMODEL_REMOVE: {
				// handle job-related messages (will re-create our menu):
				BmListModelItem* item=NULL;
				msg->FindPointer( BmListModel::MSG_MODELITEM, (void**)&item);
				if (item)
					item->RemoveRef();		// the msg is no longer referencing the item
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( ModelNameNC() << ": " << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::JobIsDone( bool completed) {
	if (completed) {
		BmAutolockCheckGlobal lock( DataModel()->ModelLocker());
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( BmString() << ControllerName() << ":JobIsDone(): Unable to lock model");
		BMenuItem* old;
		while( (old = RemoveItem( (int32)0))!=NULL)
			delete old;
		BmRef<BmDataModel> modelRef( DataModel());
		BmListModel *model = dynamic_cast<BmListModel*>( modelRef.Get());
		typedef map< BmString, BmListModelItem* > BmSortedItemMap;
		BmSortedItemMap sortedMap;
		BmModelItemMap::const_iterator iter;
		for( iter = model->begin(); iter != model->end(); ++iter) {
			BmString sortKey = iter->second->DisplayKey();
			sortedMap[sortKey.ToLower()] = iter->second.Get();
		}
		int i=0;
		BmSortedItemMap::const_iterator siter;
		for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter, ++i)
			if (i<mShortcuts.Length())
				AddItemToMenu( siter->second, this, mSkipFirstLevel, mShortcuts[i]);
			else
				AddItemToMenu( siter->second, this, mSkipFirstLevel);
	}
}

/*------------------------------------------------------------------------------*\
	AddItemToMenu()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::AddItemToMenu( BmListModelItem* item, BMenu* menu, 
												  bool skipThisButAddChildren,
												  char shortcut) {
	if (item && menu) {
		typedef map< BmString, BmListModelItem* > BmSortedItemMap;
		BmSortedItemMap sortedMap;
		if (skipThisButAddChildren) {
			if (!item->empty()) {
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddItemToMenu( siter->second, menu);
			}
		} else {
			BFont font;
			GetFont( &font);
			BMessage* msg = new BMessage( mMsgTemplate);
			msg->AddString( BmListModel::MSG_ITEMKEY, item->Key().String());
			BMenuItem* menuItem;
			if (!item->empty()) {
				BMenu* subMenu = new BMenu( item->DisplayKey().String());
				subMenu->SetFont( &font);
				BmModelItemMap::const_iterator iter;
				for( iter = item->begin();  iter != item->end();  ++iter) {
					BmString sortKey = iter->second->DisplayKey();
					sortedMap[sortKey.ToLower()] = iter->second.Get();
				}
				BmSortedItemMap::const_iterator siter;
				for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter)
					AddItemToMenu( siter->second, subMenu);
				menuItem = new BMenuItem( subMenu, msg);
			} else {
				menuItem = new BMenuItem( item->DisplayKey().String(), msg);
			}
			if (shortcut)
				menuItem->SetShortcut( shortcut, 0);
			menuItem->SetTarget( mMsgTarget);
			menu->AddItem( menuItem);
		}
	}
}
