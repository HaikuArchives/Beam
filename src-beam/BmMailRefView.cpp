/*
	BmMailRefView.cpp
		$Id$
*/

#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRefList.h"
#include "BmMailRefView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

const int16 BmMailRefItem::nFirstTextCol = 3;

/********************************************************************************\
	BmMailRefItem
\********************************************************************************/


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem( BString key, BmListModelItem* _item)
	:	inherited( key, _item)
{
	BmMailRef* ref = dynamic_cast<BmMailRef*>( _item);

	BString st = BString("Mail_") << ref->Status();
	BBitmap* icon = TheResources->IconMap[st];
	SetColumnContent( 0, icon, 2.0, false);

	if (ref->HasAttachments()) {
		icon = TheResources->IconMap["Attachment"];
		SetColumnContent( 1, icon, 2.0, false);
	}
	
	BString priority = BString("Priority_") << ref->Priority();
	if ((icon = TheResources->IconMap[priority])) {
		SetColumnContent( 2, icon, 2.0, false);
	}

	BmListColumn cols[] = {
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
		{ ref->TrackerName(),						false },
		{ NULL, false }
	};
	SetTextCols( nFirstTextCol, cols, !ThePrefs->StripedListView());
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
	if (column_index == 0) {
		// status
		BString st = ref->Status();
		return st == "New" 			? 0 :
				 st == "Read" 			? 1 :
				 st == "Forwarded" 	? 2 :
				 st == "Replied" 		? 3 :
				 st == "Pending" 		? 4 :
				 st == "Sent" 			? 5 : 99;
	} else if (column_index == 1) {
		// attachment
		return ref->HasAttachments() ? 0 : 1;	
							// show mails with attachment at top (with sortmode 'ascending')
	} else if (column_index == 2) {
		// priority
		int16 prio = atol( ref->Priority().String());
		return (prio>=1 && prio<=5) ? prio : 3;
							// illdefined priority means medium priority (=3)
	} else if (column_index == 6) {
		// size
		return ref->Size();
	} else {
		return 0;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const time_t BmMailRefItem::GetDateValueForColumn( int32 column_index) const {
	BmMailRef* ref = dynamic_cast<BmMailRef*>( mModelItem);
	if (column_index == 5)
		return ref->When();
	else if (column_index == 12)
		return ref->When();
	else
		return 0;
}



/********************************************************************************\
	BmMailRefView
\********************************************************************************/


BmMailRefView* BmMailRefView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView* BmMailRefView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmMailRefView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::BmMailRefView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width,height), "Beam_MailRefView", B_MULTIPLE_SELECTION_LIST, 
					  false, true, true, true)
	,	mCurrFolder( NULL)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->StripedListView())
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;
	Initialize( BRect(0,0,width,height), B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_ALL, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "Status [Icon]"));
	AddColumn( new CLVColumn( "A", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "(A)ttachments [Icon]"));
	AddColumn( new CLVColumn( "P", 18.0, CLV_SORT_KEYABLE | CLV_NOT_RESIZABLE | CLV_COLDATA_NUMBER, 
									  18.0, "(P)riority [Icon]"));
	AddColumn( new CLVColumn( "From", 200.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Subject", 200.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Date", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE | flags, 20.0));
	AddColumn( new CLVColumn( "Size", 50.0, CLV_SORT_KEYABLE | CLV_COLDATA_NUMBER | CLV_RIGHT_JUSTIFIED | flags, 20.0));
	AddColumn( new CLVColumn( "Cc", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Account", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "To", 100.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Reply-To", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Name", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	AddColumn( new CLVColumn( "Created", 100.0, CLV_SORT_KEYABLE | CLV_COLDATA_DATE | flags, 20.0));
	AddColumn( new CLVColumn( "Tracker-Name", 150.0, CLV_SORT_KEYABLE | flags, 20.0));
	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( 3);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefView::~BmMailRefView() { 
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmMailRefView::CreateListViewItem( BmListModelItem* item, 
																	BMessage* archive) {
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
	try {
		StopJob();
		BmMailRefList* refList = folder->MailRefList();
		StartJob( refList, true);
		mCurrFolder = folder;
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailRefView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BString BmMailRefView::StateInfoBasename()	{ 
	return "MailRefView";
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BMessage* BmMailRefView::DefaultLayout()		{ 
	return ThePrefs->MailRefLayout(); 
}
