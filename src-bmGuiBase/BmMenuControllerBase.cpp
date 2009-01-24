/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <vector>

#include <MenuItem.h>
#include <Window.h>

#include "BmBasics.h"
#include "BmMenuControllerBase.h"

/*------------------------------------------------------------------------------*\
	BmMenuControllerBase()
		-	
\*------------------------------------------------------------------------------*/
BmMenuControllerBase::BmMenuControllerBase( const char* label, 
														  BHandler* msgTarget,
														  BMessage* msgTemplate, 
														  BmRebuildMenuFunc func, 
														  int32 flags)
	:	inherited( label, flags & (BM_MC_RADIO_MODE|BM_MC_LABEL_FROM_MARKED),
					  flags & BM_MC_LABEL_FROM_MARKED)
	,	mMsgTarget( msgTarget)
	,	mMsgTemplate( msgTemplate)
	,	mRebuildMenuFunc( func)
	,	mFlags( flags)
{
	UpdateItemList();
}

/*------------------------------------------------------------------------------*\
	~BmMenuControllerBase()
		-	
\*------------------------------------------------------------------------------*/
BmMenuControllerBase::~BmMenuControllerBase() {
	delete mMsgTemplate;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::UpdateItemList( void) {
	if (mRebuildMenuFunc) {
		// menu is created by a dedicated function:
		(BeamGuiRoster->*mRebuildMenuFunc)( this);
		if (mFlags & BM_MC_ADD_NONE_ITEM) {
			BMenuItem* item = new BMenuItem( BM_NoItemLabel.String(), 
														new BMessage(*mMsgTemplate));
			item->SetTarget( mMsgTarget);
			AddItem( item);
		}
	}
	if (mFlags & BM_MC_LABEL_FROM_MARKED) {
		// mark the item corresponding to the label of our controlling menu-item:
		BMenuItem* labelItem = Superitem();
		if (labelItem)
			MarkItemInMenu( this, labelItem->Label());
		else
			ClearMarkInMenu( this);
	} else {
		if (mMarkedLabel.Length())
			MarkItemInMenu( this, mMarkedLabel.String());
		else
			ClearMarkInMenu( this);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::MarkItem( const char* label) {
	mMarkedLabel = label;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::ClearMark() {
	mMarkedLabel.Truncate(0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BMenuItem* BmMenuControllerBase::MarkItemInMenu( BMenu* menu, 	
																 const char* label) {
	// we walk down the tree to find the corresponding menu-item:
	if (!menu || !label)
		return NULL;
	BMenuItem* item = NULL;
	// split path into it's parts:
	vector<BmString> itemVect;
	int32 startPos = 0;
	int32 endPos;
	BmString labelStr( label);
	BmString str;
	while( (endPos = labelStr.FindFirst( "/", startPos)) >= B_OK) {
		labelStr.CopyInto( str, startPos, endPos);
		itemVect.push_back( str);
		startPos = endPos + 1;
	}
	itemVect.push_back( labelStr.String()+startPos);
	// mark menu according to path-parts:
	BMenu* currMenu = menu;
	for( uint32 i=0; currMenu && i<itemVect.size(); ++i) {
		item = currMenu->FindItem( itemVect[i].String());
		currMenu = item 
						? item->Submenu()
						: NULL;
	}
	if (item)
		item->SetMarked( true);
	else
		ClearMarkInMenu( menu);
	return item;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::ClearMarkInMenu( BMenu* menu) {
	if (!menu)
		return;
	BMenuItem* item;
	while( (item = menu->FindMarked()) != NULL)
		item->SetMarked( false);
	int32 count=menu->CountItems();
	for( int i=0; i<count; ++i) {
		BMenu* subMenu = menu->SubmenuAt( i);
		if (subMenu)
			ClearMarkInMenu( subMenu);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::AttachedToWindow( void) {
	UpdateItemList();
	inherited::AttachedToWindow();
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BPoint BmMenuControllerBase::ScreenLocation() {
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BHandler* BmMenuControllerBase::MsgTarget() const
{ 
	if (!mMsgTarget) {
		// no target specified, return Window of controlling item as target:
		BMenuItem* item = Superitem();
		if (item && item->Menu())
			return item->Menu()->Window();
	}
	return mMsgTarget; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::Shortcuts( const BmString s)
{ 
	mShortcuts = s; 
	UpdateItemList(); 
}

/*------------------------------------------------------------------------------*\
	ClearMenu()
		-	
\*------------------------------------------------------------------------------*/
void BmMenuControllerBase::Clear()
{
	BMenuItem* old;
	while( (old = RemoveItem( (int32)0)) != NULL)
		delete old;
}

