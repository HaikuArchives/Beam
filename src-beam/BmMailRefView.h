/*
	BmMailRefView.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


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
	BmMailRefItem( const BmString& key, BmListModelItem* item);
	~BmMailRefItem();
	
	// overrides of ListViewItem:
	BmMailRef* ModelItem() const	{ return dynamic_cast<BmMailRef*>(mModelItem.Get()); }
	void UpdateView( BmUpdFlags flags);
	
	// overrides of CLVEasyItem base:
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;
	const char* GetUserText( int32 column_index, float column_width) const;

	// Hide copy-constructor and assignment:
	BmMailRefItem( const BmMailRefItem&);
	BmMailRefItem operator=( const BmMailRefItem&);
};

class BmMailView;
class BmMenuController;
/*------------------------------------------------------------------------------*\
	BmMailRefView
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefView : public BmListViewController
{
	typedef BmListViewController inherited;
	
public:
	static const char* const MSG_MAILS_SELECTED;

	static const char* const MENU_MARK_AS;
	static const char* const MENU_FILTER;

	// creator-func, c'tors and d'tor:
	static BmMailRefView* CreateInstance( minimax minmax, int32 width, int32 height);
	BmMailRefView( minimax minmax, int32 width, int32 height);
	~BmMailRefView();

	// native methods:
	void ShowFolder( BmMailFolder* folder);
	inline void TeamUpWith( BmMailView* mv) 	{ mPartnerMailView = mv; }
	void AddSelectedRefsToMsg( BMessage* msg, BmString fieldName);
	void ShowMenu( BPoint point);
	static void AddMailRefMenu( BMenu* menu, BHandler* target,
										 BHandler* menuControllerHandler,
										 bool isContextMenu,
										 BmMenuController** filterMenuPtr=NULL);
	void SendNoticesIfNeeded( bool haveSelectedRef);

	// overrides of listview base:
	void KeyDown(const char *bytes, int32 numBytes);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( const BMessage* msg);
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);
	void ItemInvoked( int32 index);
	void MouseDown(BPoint point);

	// overrides of controller base:
	BmString StateInfoBasename();
	BMessage* DefaultLayout();
	CLVContainerView* CreateContainer( bool horizontal, bool vertical, 
												  bool scroll_view_corner, border_style border, 
												  uint32 ResizingMode, uint32 flags);
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	const char* ItemNameForCaption()		{ return "message"; }

	void AvoidInvoke( bool b)				{ mAvoidInvoke = b; }

private:
	BmRef<BmMailFolder> mCurrFolder;
	BmMailView* mPartnerMailView;
	bool mAvoidInvoke;
	bool mHaveSelectedRef;

	// Hide copy-constructor and assignment:
	BmMailRefView( const BmMailRefView&);
	BmMailRefView operator=( const BmMailRefView&);
};

#endif
