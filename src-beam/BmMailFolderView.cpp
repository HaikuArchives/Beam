/*
	BmMailFolderView.cpp
		$Id$
*/

#include <String.h>

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderItem::BmMailFolderItem( BString key, BmListModelItem* _item, uint32 level, 
												bool superitem, bool expanded)
	:	inherited( key, _item, bmApp->FolderIcon, true, level, superitem, expanded)
{
	BmMailFolder* folder = dynamic_cast<BmMailFolder*>( _item);
	const char* textCols[] = {
		folder->Name().String(),
		NULL
	};
	SetTextCols( textCols);
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
const BString& BmMailFolderItem::GetSortKey( const BString& col) {
	BmMailFolder* folder = dynamic_cast<BmMailFolder*>( mModelItem); 
	return folder->Name();
}

	

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView* BmMailFolderView::CreateInstance( BRect rect) {
	BmMailFolderView* mailFolderView = new BmMailFolderView( rect);
	return mailFolderView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::BmMailFolderView( BRect rect)
	:	inherited( rect, "Beam_FolderView", B_SINGLE_SELECTION_LIST, 
					  true, false)
{
	mContainerView = Initialize( rect,
										  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
										  B_FOLLOW_NONE,
										  false, true, false, B_FANCY_BORDER);
	AddColumn( new CLVColumn( NULL, 10.0, 
									  CLV_EXPANDER | CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE, 10.0));
	AddColumn( new CLVColumn( NULL, 18.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE | CLV_PUSH_PASS | CLV_MERGE_WITH_RIGHT, 18.0));
	AddColumn( new CLVColumn( NULL, 300.0, CLV_NOT_RESIZABLE | CLV_NOT_MOVABLE, 50.0));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolderView::~BmMailFolderView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailFolderView::CreateListViewItem( BmListModelItem* item,
																		uint32 level) {
	return new BmMailFolderItem( item->Key(), item, level, false, false);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailFolderView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	SelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderView::SelectionChanged( void) {
	uint32 selection = CurrentSelection();
	if (selection >= 0) {
		BmMailFolderItem* folderItem;
		folderItem = dynamic_cast<BmMailFolderItem*>(ItemAt( selection));
		if (folderItem) {
			BmMailFolder* folder = dynamic_cast<BmMailFolder*>(folderItem->ModelItem());
			if (folder)
				bmApp->MailRefView->ShowFolder( folder);
		}
	}
}
