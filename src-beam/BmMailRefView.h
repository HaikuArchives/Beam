/*
	BmMailRefView.h
		$Id$
*/

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include <map>

#include "BmController.h"

class BmMailFolder;

/*------------------------------------------------------------------------------*\
	BmMailRefItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	BmMailRefItem( BString key, BmListModelItem* item);
	~BmMailRefItem();
	
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;

};

/*------------------------------------------------------------------------------*\
	BmMailRefView
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	//
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);
	
	//Constructor and destructor
	BmMailRefView( minimax minmax, int32 width, int32 height);
	~BmMailRefView();

	//
	void MessageReceived( BMessage* msg);
	void ShowFolder( BmMailFolder* folder);

	//
	BmListViewItem* CreateListViewItem( BmListModelItem* item, uint32 level=0);
	
private:
	BmMailFolder* mCurrFolder;

};


#endif
