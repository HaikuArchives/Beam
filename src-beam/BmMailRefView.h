/*
	BmMailRefView.h
		$Id$
*/

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include "ColumnListView.h"
#include "CLVEasyItem.h"

class BmMailRefView : public ColumnListView
{
	typedef ColumnListView inherited;

public:
	//
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);

	//Constructor and destructor
	BmMailRefView(	minimax minmax, int32 width, int32 height);

	// getters:
	CLVContainerView* ContainerView()	{ return mContainerView; };

private:
	CLVContainerView* mContainerView;
};

class BmMailRefItem : public CLVEasyItem
{
	typedef CLVEasyItem inherited;

public:
	BmMailRefItem( uint32 level, bool superitem, bool expanded, BBitmap* icon, const char* text0);
	~BmMailRefItem();
};

#endif
