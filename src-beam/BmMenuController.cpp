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

#include "BmGuiUtil.h"
#include "BmMenuController.h"


/*------------------------------------------------------------------------------*\
	BmMenuController()
		-	
\*------------------------------------------------------------------------------*/
BmMenuController::BmMenuController( const char* label, BHandler* msgTarget,
												BMessage& msgTemplate, BmListModel* listModel,
												bool skipFirstLevel)
	:	inherited( label)
	,	inheritedController( label)
	,	mMsgTemplate( msgTemplate)
	,	mMsgTarget( msgTarget)
	,	mListModel( listModel)
	,	mSkipFirstLevel( skipFirstLevel)
	,	mRebuildMenuFunc( NULL)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::AttachedToWindow( void) {
	if (mRebuildMenuFunc) {
		(*mRebuildMenuFunc)( this);
	} else {
		JobIsDone( true);
	} 
	inherited::AttachedToWindow();
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::DetachedFromWindow( void) {
	if (!mRebuildMenuFunc) {
		// disconnect from list-model:
		DetachModel();
	}
	inherited::DetachedFromWindow();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::MessageReceived( BMessage* msg) {
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
			JobIsDone( true);
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuController::JobIsDone( bool completed) {
	typedef map< BmString, BmListModelItem* > BmSortedItemMap;
	if (completed) {
		BmAutolockCheckGlobal lock( mListModel->ModelLocker());
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( BmString() << ControllerName() << ":JobIsDone(): Unable to lock model");
		BMenuItem* old;
		while( (old = RemoveItem( (int32)0))!=NULL)
			delete old;
		BmSortedItemMap sortedMap;
		BmModelItemMap::const_iterator iter;
		for( iter = mListModel->begin(); iter != mListModel->end(); ++iter) {
			BmString sortKey = iter->second->DisplayKey();
			sortedMap[sortKey.ToLower()] = iter->second.Get();
		}
		int i=0;
		BFont font;
		GetFont( &font);
		BmSortedItemMap::const_iterator siter;
		for( siter = sortedMap.begin(); siter != sortedMap.end(); ++siter, ++i)
			if (i<mShortcuts.Length())
				AddListItemToMenu( siter->second, this, &mMsgTemplate, mMsgTarget, &font,
										 mSkipFirstLevel, mShortcuts[i]);
			else
				AddListItemToMenu( siter->second, this, &mMsgTemplate, mMsgTarget, &font,
										 mSkipFirstLevel);
	}
}
