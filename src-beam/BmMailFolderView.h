/*
	BmMailFolderView.h
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


#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include <map>

#include "BmListController.h"

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
#define BM_NTFY_MAILFOLDER_SELECTION	'bmbb'
						// sent from BmMailFolderView to observers whenever selection changes

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

	static int CompareItems( const CLVListItem *a_Item1, const CLVListItem *a_Item2,
									 int32 KeyColumn, int32 col_flags);

private:
	// Hide copy-constructor and assignment:
	BmMailFolderItem( const BmMailFolderItem&);
	BmMailFolderItem operator=( const BmMailFolderItem&);
};


class BmMailRefView;
/*------------------------------------------------------------------------------*\
	BmMailFolderView
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderView : public BmListViewController
{
	typedef BmListViewController inherited;
	
	//	message component definitions for archive:
	static const char* const MSG_CURR_FOLDER = "bm:currfolder";

public:
	static const char* const MSG_FOLDERS_SELECTED = 		"bm:fsel";

	// creator-func, c'tors and d'tor:
	static BmMailFolderView* CreateInstance(  minimax minmax, int32 width, int32 height);
	BmMailFolderView(  minimax minmax, int32 width, int32 height);
	~BmMailFolderView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	void ShowMenu( BPoint point);
	inline void TeamUpWith( BmMailRefView* mrv) 	{ mPartnerMailRefView = mrv; }
	
	// overrides of controller base:
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( const BMessage* msg);
	BString StateInfoBasename()			{ return "MailFolderView"; }
	void UpdateModelItem( BMessage* msg);
	const char* ItemNameForCaption()		{ return "folder"; }
	void JobIsDone( bool completed);
	//
	status_t Archive(BMessage* archive, bool deep=true) const;
	status_t Unarchive(const BMessage* archive, bool deep=true);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	void SelectionChanged( void);

	static BmMailFolderView* theInstance;

private:

	BmRef<BmMailFolder> CurrentFolder( void) const;
	
	BmMailRefView* mPartnerMailRefView;
	
	BString mLastActiveKey;

	// Hide copy-constructor and assignment:
	BmMailFolderView( const BmMailFolderView&);
	BmMailFolderView operator=( const BmMailFolderView&);
};

#define TheMailFolderView BmMailFolderView::theInstance

#endif
