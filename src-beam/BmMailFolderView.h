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
	static BmMailFolderView* CreateInstance(  minimax minmax, int32 width, int32 height);
	
	//Constructor and destructor
	BmMailFolderView(  minimax minmax, int32 width, int32 height);
	~BmMailFolderView();

	//
	BmListViewItem* CreateListViewItem( BmListModelItem* item, uint32 level=0);
	
	//
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);

private:
};

#endif
