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
	:	inherited( key, _item, bmApp->MailIcon, false)
{
	BmMailRef* ref = dynamic_cast<BmMailRef*>( _item);

	BString st = ref->Status();
	BString status = 
		st == "Read"		? "R" :
		st == "New" 		? "N" :
		st == "Sent" 		? "S" : "?";
	
	BmListColumn cols[] = {
		{ status.String(), 							false },
		{ ref->HasAttachments() ? "*" : " ",	false },
		{ ref->Priority().String(),				false },
		{ ref->From().String(),						false },
		{ ref->Subject().String(),					false },
		{ ref->WhenString().String(),				false },
		{ ref->SizeString().String(),				true  },
		{ ref->Cc().String(),						false },
		{ ref->Account().String(),					false },
		{ ref->To().String(),						false },
		{ ref->ReplyTo().String(),					false },
		{ ref->Name().String(),						false },
		{ ref->CreatedString().String(),			false },
		{ NULL, false }
	};
	SetTextCols( cols);
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
const int32 BmMailRefItem::GetNumValueForColumn( int32 column_index) const {
	BmMailRef* ref = dynamic_cast<BmMailRef*>( mModelItem);
	if (column_index == 7)
		return ref->Size();
	else
		return 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const time_t BmMailRefItem::GetDateValueForColumn( int32 column_index) const {
	BmMailRef* ref = dynamic_cast<BmMailRef*>( mModelItem);
	if (column_index == 6)
		return ref->When();
	else if (column_index == 13)
		return ref->When();
	else
		return 0;
}



/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( minimax minmax, int32 width, int32 height) {
	BmMailRefView* mailRefView = new BmMailRefView( minmax, width, height);
	return mailRefView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width,height), "Beam_MailRefView", B_MULTIPLE_SELECTION_LIST, 
					  false, true)
	,	mCurrFolder( NULL)
{
	SetViewColor( B_TRANSPARENT_COLOR);
	SetStripedBackground( true);
	mContainerView = Initialize( BRect(0,0,width,height),
										  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
										  B_FOLLOW_ALL,
										  true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( NULL, 18.0, CLV_LOCK_AT_BEGINNING | CLV_NOT_MOVABLE 
									  | CLV_NOT_RESIZABLE, 18.0));
	AddColumn( new CLVColumn( "S", 16.0, CLV_SORT_KEYABLE, 16.0));
	AddColumn( new CLVColumn( "A", 16.0, CLV_SORT_KEYABLE, 16.0));
	AddColumn( new CLVColumn( "P", 16.0, CLV_SORT_KEYABLE, 16.0));
	AddColumn( new CLVColumn( "From", 200.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Subject", 200.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Date", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE, 20.0));
	AddColumn( new CLVColumn( "Size", 50.0, CLV_SORT_KEYABLE | CLV_COLDATA_NUMBER | CLV_RIGHT_JUSTIFIED, 20.0));
	AddColumn( new CLVColumn( "Cc", 100.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Account", 100.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "To", 100.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Reply-To", 150.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Tracker-Name", 150.0, CLV_SORT_KEYABLE, 20.0));
	AddColumn( new CLVColumn( "Created", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE, 20.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( 3);
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

