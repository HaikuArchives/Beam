/*
	BmMailFolderView.cpp
		$Id$
*/

#include <String.h>

#include "BmApp.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView* BmMailFolderView::CreateInstance( minimax minmax, int32 width, int32 height) {
	BmMailFolderView* mailFolderView = new BmMailFolderView( minmax, width, height);
	return mailFolderView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::BmMailFolderView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FolderView", 
					  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					  B_SINGLE_SELECTION_LIST, true, false)
{
	mContainerView = Initialize( BRect(0,0,width-1,height-1), 
										  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
										  false, true, true, B_NO_BORDER);
	AddColumn( new CLVColumn( NULL, 10.0, 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE));
	AddColumn( new CLVColumn( NULL, 20.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT));
	AddColumn( new CLVColumn( NULL, 300.0, CLV_HEADER_TRUNCATE | CLV_TELL_ITEMS_WIDTH, 
									  50.0));
	SetSortFunction( CLVEasyItem::CompareItems);

	AddItem( mTopMailFolderItem = new BmMailFolderItem( 0, true, true, bmApp->FolderIcon, "Mail Folders"));
	AddItem( mTopVirtualFolderItem = new BmMailFolderItem( 0, false, false, bmApp->FolderIcon, "Virtual Folders"));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::AddFolder( BMessage* msg) {
	const char* folderName = NULL;
	ino_t inode = 0;
	ino_t pnode = 0;
	BmMailFolderItem* parentItem = NULL;

	folderName = FindMsgString( msg, BmMailFolderList::MSG_NAME);
	inode = FindMsgInt64( msg, BmMailFolderList::MSG_INODE);
	pnode = FindMsgInt64( msg, BmMailFolderList::MSG_PNODE);
	if (!parentItem) {
		parentItem = mTopMailFolderItem;
	} else {
		(parentItem = FindItemByID( pnode))
													|| BM_THROW_RUNTIME( BString("AddFolder(): Parent-node ") << pnode << " not found for folder " << folderName);
	}
	parentItem->SetSuperItem( true);
	AddUnder( new BmMailFolderItem( parentItem->OutlineLevel()+1, false, false, 
											  bmApp->FolderIcon, folderName), parentItem);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem* BmMailFolderView::FindItemByID( ino_t id) {
	return NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::BmMailFolderItem(uint32 level, bool superitem, bool expanded, BBitmap* icon, const char* text0)
	:	inherited( level, superitem, expanded, 16.0)
{
	SetColumnContent(1, icon, 2.0, true);
	SetColumnContent(2, text0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::~BmMailFolderItem() { 
}
