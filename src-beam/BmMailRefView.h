/*
	BmMailRefView.h
		$Id$
*/

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include <map>

#include "BmListController.h"
#include "BmMailRef.h"

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
#define BM_NTFY_MAILREF_SELECTION	'bmba'
						// sent from BmMailRefView to observers whenever mail-selection changes
#define BM_MAILREF_INVOKED				'bmbb'
						// sent from BmMailRefView whenever a mail-ref is invoked (should be shown
						// inside a window of it's own).

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
	
	// overrides of ListViewItem:
	BmMailRef* ModelItem() const			{ return dynamic_cast< BmMailRef*>( mModelItem.Get()); }
	void UpdateView( BmUpdFlags flags);
	
	// overrides of CLVEasyItem base:
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;

	// Hide copy-constructor and assignment:
	BmMailRefItem( const BmMailRefItem&);
	BmMailRefItem operator=( const BmMailRefItem&);
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
	static const char* const MSG_MAILS_SELECTED = 		"bm:msel";

	// creator-func, c'tors and d'tor:
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);
	BmMailRefView( minimax minmax, int32 width, int32 height);
	~BmMailRefView();

	// native methods:
	void ShowFolder( BmMailFolder* folder);
	void TeamUpWith( BmMailView* mv) 	{ mPartnerMailView = mv; }

	// overrides of listview base:
	void KeyDown(const char *bytes, int32 numBytes);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( const BMessage* msg);
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);
	void ItemInvoked( int32 index);
	void MouseDown(BPoint point);
	void MouseUp(BPoint point);

	// overrides of controller base:
	BString StateInfoBasename();
	const BMessage* DefaultLayout();
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	const char* ItemNameForCaption()		{ return "message"; }

	// getters:
	BmRef<BmMailRef> CurrMailRef()		{ return mCurrMailRef; }

	static BmMailRefView* theInstance;
	
private:
	BmRef<BmMailFolder> mCurrFolder;
	BmMailView* mPartnerMailView;
	bool mMouseIsDown;
	BmRef<BmMailRef> mCurrMailRef;

	// Hide copy-constructor and assignment:
	BmMailRefView( const BmMailRefView&);
	BmMailRefView operator=( const BmMailRefView&);
};

#define TheMailRefView BmMailRefView::theInstance

#endif
