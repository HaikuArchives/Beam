/*
	BmMailFolderView.h
		$Id$
*/

#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include <map>

#include "BmListController.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailFolderView:
\*------------------------------------------------------------------------------*/
#define BM_FOLDERVIEW_NEW				'bmga'
#define BM_FOLDERVIEW_RENAME			'bmgb'
#define BM_FOLDERVIEW_RECACHE			'bmgc'
#define BM_FOLDERVIEW_DELETE			'bmgd'

/*------------------------------------------------------------------------------*\
	BmMailFolderItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmMailFolderItem( BString key, BmListModelItem* item, bool superitem, 
							BMessage* archive);
	~BmMailFolderItem();

	// overrides of listitem base:
	void UpdateView( BmUpdFlags flags);
	BmMailFolder* ModelItem() const 		{ return dynamic_cast< BmMailFolder*>( mModelItem.Get()); }

private:
	// Hide copy-constructor and assignment:
	BmMailFolderItem( const BmMailFolderItem&);
	BmMailFolderItem operator=( const BmMailFolderItem&);
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
	void ShowMenu( BPoint point);
	
	// overrides of controller base:
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( const BMessage* msg);
	BString StateInfoBasename()			{ return "MailFolderView"; }
	void UpdateModelItem( BMessage* msg);
	const char* ItemNameForCaption()		{ return "folder"; }

	// overrides of listview base:
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	void SelectionChanged( void);

	static BmMailFolderView* theInstance;

private:

	BmRef<BmMailFolder> CurrentFolder( void) const;

	// Hide copy-constructor and assignment:
	BmMailFolderView( const BmMailFolderView&);
	BmMailFolderView operator=( const BmMailFolderView&);
};

#define TheMailFolderView BmMailFolderView::theInstance;

#endif
