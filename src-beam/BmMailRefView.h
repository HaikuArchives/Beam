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
	
	//
	const BString& GetSortKey( const BString& col);

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
	static BmMailRefView* CreateInstance( BRect rect);
	
	//Constructor and destructor
	BmMailRefView(	BRect rect);
	~BmMailRefView();

	//
	void MessageReceived( BMessage* msg);
	void ShowFolder( BmMailFolder* folder);

	//
	BmListViewItem* CreateListViewItem( BmListModelItem* item, uint32 level=0);
	
	//
	void MakeEmpty( void);

private:
	BmMailFolder* mCurrFolder;

};


#endif
