/*
	BmMailFolderView.h
		$Id$
*/

#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include <map>

#include "BmListController.h"


/*------------------------------------------------------------------------------*\
	BmMailFolderItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

	enum Columns {
		COL_ICON = 1,
		COL_NAME,
		COL_MAX
	};

protected:
	// flags indicating which parts are to be updated
	static const BmUpdFlags UPD_NAME	= 2<<0;

public:
	// c'tors and d'tor:
	BmMailFolderItem( BString key, BmListModelItem* item, bool superitem, 
							BMessage* archive);
	~BmMailFolderItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);
	BmMailFolder* ModelItem() 	{ return dynamic_cast< BmMailFolder*>( mModelItem); }
};


/*------------------------------------------------------------------------------*\
	BmMailFolderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmMailFolderView(  minimax minmax, int32 width, int32 height);
	~BmMailFolderView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	
	// overrides of controller base:
	BString StateInfoBasename()			{ return "MailFolderView"; }
	void Update( BmUpdFlags flags);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);

};

#endif
