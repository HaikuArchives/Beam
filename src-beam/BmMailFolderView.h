/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailFolderView_h
#define _BmMailFolderView_h

#include <map>

#include "BmListController.h"
#include "BmMailFolder.h"

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
enum {
	BM_NTFY_MAILFOLDER_SELECTION = 'bmbb'
						// sent from BmMailFolderView to observers whenever 
						// selection changes
};

/*------------------------------------------------------------------------------*\
	BmMailFolderItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailFolderItem : public BmListViewItem
{
	typedef BmListViewItem inherited;

public:
	// c'tors and d'tor:
	BmMailFolderItem( ColumnListView* lv, BmListModelItem* item, bool superitem,
							BMessage* archive);
	~BmMailFolderItem();

	// overrides of listitem base:
	BmMailFolder* ModelItem() const	{ return dynamic_cast<BmMailFolder*>(
																				mModelItem.Get()); }
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
	BmBitmapHandle* GetExpanderBitmap( bool expanded);

	static int CompareItems( const CLVListItem *a_Item1, 
									 const CLVListItem *a_Item2,
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
	static const char* const MSG_CURR_FOLDER;
	static const char* const MSG_VERSION;

public:
	static const char* const MSG_HAVE_SELECTED_FOLDER;

	// creator-func, c'tors and d'tor:
	static BmMailFolderView* CreateInstance( int32 width, int32 height);
	BmMailFolderView( int32 width, int32 height);
	~BmMailFolderView();

	// native methods:
	BmListViewItem* CreateListViewItem( BmListModelItem* item, 
													BMessage* archive=NULL);
	void ShowMenu( BPoint point);
	inline void TeamUpWith( BmMailRefView* mrv) 	{ mPartnerMailRefView = mrv; }
	void SendNoticesIfNeeded( bool haveSelectedFolder);
	
	// overrides of controller base:
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( BMessage* msg);
	void ItemInvoked( int32 index);
	void KeyDown(const char *bytes, int32 numBytes);
	BmString StateInfoBasename()			{ return "MailFolderView"; }
	const char* ItemNameForCaption()		{ return "folder"; }
	//
	status_t Archive(BMessage* archive, bool deep=true) const;
	status_t Unarchive(const BMessage* archive, bool deep=true);

	// overrides of listview base:
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	void SelectionChanged( void);

	static BmMailFolderView* theInstance;

protected:
	void JobIsDone( bool completed);

private:
	static int16 nArchiveVersion;

	BmRef<BmMailFolder> CurrentFolder( void) const;
	
	BmMailRefView* mPartnerMailRefView;
	
	BmString mLastActiveKey;
	
	bool mHaveSelectedFolder;

	// Hide copy-constructor and assignment:
	BmMailFolderView( const BmMailFolderView&);
	BmMailFolderView operator=( const BmMailFolderView&);
};

#define TheMailFolderView BmMailFolderView::theInstance

#endif
