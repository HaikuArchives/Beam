/*
	BmMailRefView.cpp
		$Id$
*/

#include "BmMailRefView.h"

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
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_MailView", 
					  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, 
					  B_MULTIPLE_SELECTION_LIST, false, true)
{
	mContainerView = Initialize( BRect(0,0,width-1,height-1), 
										  B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
										  true, true, false, B_NO_BORDER);

	AddColumn(new CLVColumn(NULL,20.0,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE));
	AddColumn(new CLVColumn(NULL,20.0,CLV_LOCK_AT_BEGINNING|CLV_NOT_MOVABLE|
		CLV_NOT_RESIZABLE|CLV_PUSH_PASS|CLV_MERGE_WITH_RIGHT));
	AddColumn(new CLVColumn("Name",108.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH,50.0));
	AddColumn(new CLVColumn("Size",70.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Date",131.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Label",180.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Locked to right",161.0,CLV_LOCK_WITH_RIGHT|
		CLV_MERGE_WITH_RIGHT|CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Boolean",55.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Another bool",80.0,CLV_SORT_KEYABLE|CLV_HEADER_TRUNCATE|
		CLV_TELL_ITEMS_WIDTH));
	AddColumn(new CLVColumn("Locked at end",237.0,CLV_LOCK_AT_END|CLV_NOT_MOVABLE|
		CLV_HEADER_TRUNCATE|CLV_TELL_ITEMS_WIDTH|CLV_RIGHT_JUSTIFIED));
	SetSortFunction( CLVEasyItem::CompareItems);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::BmMailRefItem(uint32 level, bool superitem, bool expanded, BBitmap* icon, const char* text0)
	:	inherited( level, superitem, expanded, 16.0)
{
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefItem::~BmMailRefItem() { 
}
