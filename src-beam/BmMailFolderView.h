/*
	BmMailFolderView.h
		$Id$
*/

#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include "ColumnListView.h"
#include "CLVEasyItem.h"

class BmMailFolderItem : public CLVEasyItem
{
	typedef CLVEasyItem inherited;

public:
	BmMailFolderItem( uint32 level, bool superitem, bool expanded, BBitmap* icon, const char* text0);
	~BmMailFolderItem();
};



class BmMailFolderView : public ColumnListView
{
	typedef ColumnListView inherited;

public:
	//
	static BmMailFolderView* CreateInstance( minimax minmax, int32 width, int32 height);
	
	//Constructor and destructor
	BmMailFolderView(	minimax minmax, int32 width, int32 height);
	~BmMailFolderView() 						{}

	// getters:
	CLVContainerView* ContainerView()	{ return mContainerView; };

	//
	void AddFolder( BMessage* msg);

private:
	CLVContainerView* mContainerView;
	BmMailFolderItem* mTopMailFolderItem;
	BmMailFolderItem* mTopVirtualFolderItem;

	BmMailFolderItem* FindItemByID( ino_t id);
};

#endif
