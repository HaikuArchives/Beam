/*
	BmMailFolderView.cpp
		$Id$
*/

#include <String.h>

#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailFolderItem
\********************************************************************************/

enum Columns {
	COL_EXPANDER = 0,
	COL_ICON,
	COL_NAME
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::BmMailFolderItem( BString key, BmListModelItem* _item, 
												bool superitem, BMessage* archive)
	:	inherited( key, _item, true, archive)
{
	UpdateView( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::~BmMailFolderItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmMailFolder* folder = ModelItem();
	if (flags & (UPD_EXPANDER | BmMailFolder::UPD_NAME)) {
		BString displayName = folder->Name();
		if (folder->HasNewMail()) {
			int32 count = folder->NewMailCount();
			if (folder->NewMailCountForSubfolders() && !IsExpanded())
	 			count += folder->NewMailCountForSubfolders();
	 		if (count)
				displayName << " - (" << count << ")";
		}
		SetColumnContent( COL_NAME, displayName.String(), false);
		BBitmap* icon;
		if (folder->NewMailCount())
			icon = TheResources->IconByName("Folder_WithNew");
		else
			icon = TheResources->IconByName("Folder");
		SetColumnContent( COL_ICON, icon, 2.0, false);
	}
}



/********************************************************************************\
	BmMailFolderView
\********************************************************************************/


BmMailFolderView* BmMailFolderView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView* BmMailFolderView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmMailFolderView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::BmMailFolderView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FolderView", B_SINGLE_SELECTION_LIST, 
					  true, true, true, true)
{
	Initialize( BRect( 0,0,width-1,height-1), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);
	AddColumn( new CLVColumn( NULL, 10.0, 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 10.0));
	AddColumn( new CLVColumn( NULL, 18.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT, 18.0));
	AddColumn( new CLVColumn( "Folders", 300.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_NOT_MOVABLE, 300.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_NAME);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::~BmMailFolderView() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailFolderView::CreateListViewItem( BmListModelItem* item,
																		BMessage* archive) {
	if (ThePrefs->RestoreFolderStates())
		return new BmMailFolderItem( item->Key(), item, true, archive);
	else
		return new BmMailFolderItem( item->Key(), item, true, NULL);
}

/*------------------------------------------------------------------------------*\
	AcceptsDropOf( msg)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderView::AcceptsDropOf( const BMessage* msg) {
	return (msg && (msg->what == BM_MAIL_DRAG || msg->what == B_SIMPLE_DATA));
}

/*------------------------------------------------------------------------------*\
	HandleDrop( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::HandleDrop( const BMessage* msg) {
	if (msg && mCurrHighlightItem
	&& (msg->what == BM_MAIL_DRAG || msg->what == B_SIMPLE_DATA)) {
		BmMailFolder* folder = dynamic_cast<BmMailFolder*>( mCurrHighlightItem->ModelItem());
		if (folder) {
			BMessage tmpMsg( BM_JOBWIN_MOVEMAILS);
			entry_ref eref;
			for( int i=0; msg->FindRef( BmMailMoverView::MSG_REFS, i, &eref)==B_OK; ++i) {
				tmpMsg.AddRef( BmMailMoverView::MSG_REFS, &eref);
			}
			tmpMsg.AddString( BmJobStatusWin::MSG_JOB_NAME, folder->Name());
			tmpMsg.AddPointer( BmMailMoverView::MSG_FOLDER, folder);
			TheJobStatusWin->PostMessage( &tmpMsg);
		}
	}
	inherited::HandleDrop( msg);
}

/*------------------------------------------------------------------------------*\
	UpdateModelItem( msg)
		-	Hook function that is called whenever an item needs to be updated 
\*------------------------------------------------------------------------------*/
void BmMailFolderView::UpdateModelItem( BMessage* msg) {
	
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::SelectionChanged( void) {
	int32 selection = CurrentSelection();
	if (selection >= 0) {
		BmMailFolderItem* folderItem;
		folderItem = dynamic_cast<BmMailFolderItem*>(ItemAt( selection));
		if (folderItem) {
			BmMailFolder* folder = dynamic_cast<BmMailFolder*>(folderItem->ModelItem());
			if (folder)
				TheMailRefView->ShowFolder( folder);
		}
	} else
		if (TheMailRefView)
			TheMailRefView->ShowFolder( NULL);
}
