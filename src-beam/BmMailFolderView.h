/*
	BmMailFolderView.h
		$Id$
*/

#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include <map>

#include "BmController.h"


/*------------------------------------------------------------------------------*\
	BmMailFolderItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	BmMailFolderItem( BString key, BmListModelItem* item, uint32 level, 
							bool superitem, bool expanded);
	~BmMailFolderItem();

	//
	const BString& GetSortKey( const BString& col);
	
};


/*------------------------------------------------------------------------------*\
	BmMailFolderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	//
	static BmMailFolderView* CreateInstance( BRect rect);
	
	//Constructor and destructor
	BmMailFolderView(	BRect rect);
	~BmMailFolderView();

	//
	BmListViewItem* CreateListViewItem( BmListModelItem* item, uint32 level=0);
	
	//
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);

private:
};

#endif
