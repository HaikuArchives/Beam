/*
	BmBodyPartView.h
		$Id$
*/

#ifndef _BmBodyPartView_h
#define _BmBodyPartView_h

#include <map>

#include "BmListController.h"

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmBodyPartView:
\*------------------------------------------------------------------------------*/
#define BM_BODYPARTVIEW_SHOWALL				'bmga'
#define BM_BODYPARTVIEW_SHOWATTACHMENTS	'bmgb'

/*------------------------------------------------------------------------------*\
	BmBodyPartItem
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPartItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	BmBodyPartItem( BString key, BmListModelItem* item);
	~BmBodyPartItem();

	// Hide copy-constructor and assignment:
	BmBodyPartItem( const BmBodyPartItem&);
	BmBodyPartItem operator=( const BmBodyPartItem&);
};

class BmBodyPartList;
/*------------------------------------------------------------------------------*\
	BmBodyPartView
		-	
\*------------------------------------------------------------------------------*/
class BmBodyPartView : public BmListViewController
{
	typedef BmListViewController inherited;
	
	// archival-fieldnames:
	static const char* const MSG_SHOWALL = 	"bm:showall";

public:
	// c'tors and d'tor:
	BmBodyPartView( minimax minmax, int32 width, int32 height, bool editable=false);
	~BmBodyPartView();

	// native methods:
	void AddAttachment( BMessage* msg);
	void AdjustVerticalSize();
	void ShowBody( BmBodyPartList* body);

	// overrides of listview base:
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	void KeyDown(const char *bytes, int32 numBytes);
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);

	// overrides of controller base:
	void AddAllModelItems();
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	void ExpansionChanged( CLVListItem* item, bool expanded);
	void ItemInvoked( int32 index);
	void RemoveModelItem( BmListModelItem* item);
	BString StateInfoBasename();

	// getters:
	float FixedWidth() 						{ return 5000; }
	bool ShowAllParts()						{ return mShowAllParts; }

	static const int16 nFirstTextCol;

private:
	void ShowMenu( BPoint point);

	static float nColWidths[10];
	float mColWidths[10];
	bool mShowAllParts;
	bool mEditable;

	// Hide copy-constructor and assignment:
	BmBodyPartView( const BmBodyPartView&);
	BmBodyPartView operator=( const BmBodyPartView&);
};

#endif
