/*
	BmMailRefView.h
		$Id$
*/

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include <map>

#include "BmListController.h"

class BmMailFolder;
/*------------------------------------------------------------------------------*\
	BmMailRefItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefItem : public BmListViewItem
{
	typedef BmListViewItem inherited;
	static const int16 nFirstTextCol;

public:
	BmMailRefItem( BString key, BmListModelItem* item);
	~BmMailRefItem();
	
	// overrides of CLVEasyItem base:
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;

};

class BmMailView;
/*------------------------------------------------------------------------------*\
	BmMailRefView
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	// creator-func, c'tors and d'tor:
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);
	BmMailRefView( minimax minmax, int32 width, int32 height);
	~BmMailRefView();

	// native methods:
	void ShowFolder( BmMailFolder* folder);
	void TeamUpWith( BmMailView* mv) 	{ mPartnerMailView = mv; }

	// overrides of listview base:
	void KeyDown(const char *bytes, int32 numBytes);
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);

	// overrides of controller base:
	BString StateInfoBasename();
	BMessage* DefaultLayout();
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);

	static BmMailRefView* theInstance;
	
private:
	BmMailFolder* mCurrFolder;
	BmMailView* mPartnerMailView;

};

#define TheMailRefView BmMailRefView::theInstance

#endif
