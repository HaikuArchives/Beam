/*
	BmMailRefView.cpp
		$Id$
*/

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRefList.h"
#include "BmMailRefView.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item, bmApp->FolderIcon, false)
{
	BmMailRef* ref = dynamic_cast<BmMailRef*>( _item);
	const char* textCols[] = {
		ref->HasAttachments() ? "*" : " ",
		ref->Priority().String(),
		ref->From().String(),
		ref->Subject().String(),
		ref->WhenString().String(),
		ref->Cc().String(),
		ref->Account().String(),
		ref->To().String(),
		ref->ReplyTo().String(),
		ref->Name().String(),
		ref->CreatedString().String(),
		NULL
	};
	SetTextCols( textCols);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::~BmMailRefItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const BString& BmMailRefItem::GetSortKey( const BString& col) {
	BmMailRef* ref = dynamic_cast<BmMailRef*>( mModelItem);
	return ref->From();
}



/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( BRect rect) {
	BmMailRefView* mailRefView = new BmMailRefView( rect);
	return mailRefView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( BRect rect)
	:	inherited( rect, "Beam_MailRefView", B_MULTIPLE_SELECTION_LIST, 
					  false, true)
	,	mCurrFolder( NULL)
{
	mContainerView = Initialize( rect, 
										  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
										  B_FOLLOW_NONE,
										  true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( NULL, 18.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE, 18.0));
	AddColumn( new CLVColumn( "A", 20.0, CLV_NOT_RESIZABLE | CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "P", 20.0, CLV_NOT_RESIZABLE | CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "From", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Subject", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "When", 50.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Cc", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Account", 50.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "To", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Reply-To", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Tracker-Name", 100.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	AddColumn( new CLVColumn( "Created", 50.0, CLV_SORT_KEYABLE | CLV_TELL_ITEMS_WIDTH, 20.0));
	SetSortFunction( CLVEasyItem::CompareItems);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::~BmMailRefView() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailRefView::CreateListViewItem( BmListModelItem* item, uint32 level) {
	return new BmMailRefItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ShowFolder()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::ShowFolder( BmMailFolder* folder) {
	StopJob();
	BmMailRefList* refList = folder->MailRefList();
	StartJob( refList, true, false);
	mCurrFolder = folder;
}

/*------------------------------------------------------------------------------*\
	MakeEmpty()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefView::MakeEmpty( void) {
	inherited::MakeEmpty();
	BmItemMap::iterator iter;
	for( iter=mItemMap.begin(); iter != mItemMap.end(); ++iter) {
		delete iter->second;
	}
	mItemMap.clear();
}
