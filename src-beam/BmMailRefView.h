/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefView_h
#define _BmMailRefView_h

#include <map>

#include "BmListController.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"

class BControl;

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
enum {
	BM_NTFY_MAILREF_SELECTION	= 'bmba',
						// sent from BmMailRefView to observers whenever mail-selection changes
	BM_MAILREF_INVOKED			= 'bmbb'
						// sent from BmMailRefView whenever a mail-ref is invoked (should be shown
						// inside a window of it's own).
};

struct BmDateWidthAdjuster
{
	BmDateWidthAdjuster(CLVEasyItem*);
	const char* operator() (int32 colIdx, time_t utc);
	BmString dateStr;
	float cachedWidth;
	CLVEasyItem* item;
};

/*------------------------------------------------------------------------------*\
	BmMailRefItem
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefItem : public BmListViewItem
{
	typedef BmListViewItem inherited;
	static const int16 nFirstTextCol;

public:
	BmMailRefItem( ColumnListView* lv, BmListModelItem* item);
	~BmMailRefItem();
	
	// overrides of ListViewItem:
	BmMailRef* ModelItem() const		{ 
													return dynamic_cast<	BmMailRef*>(
															mModelItem.Get()
													); 
												}
	void UpdateView( BmUpdFlags flags, bool redraw = true, 
						  uint32 updColBitmap = 0);
	
	void FitDateIntoColumn(int32 colIdx, time_t utc, 
								  BmString& dateStr) const;

	// overrides of CLVEasyItem base:
	const int32 GetNumValueForColumn( int32 column_index) const;
	const time_t GetDateValueForColumn( int32 column_index) const;
	const bigtime_t GetBigtimeValueForColumn( int32 column_index) const;
	const char* GetUserText( int32 column_index, float column_width) const;

private:
	mutable BmDateWidthAdjuster mWhenStringAdjuster;
	mutable BmDateWidthAdjuster mWhenCreatedStringAdjuster;

	// Hide copy-constructor and assignment:
	BmMailRefItem( const BmMailRefItem&);
	BmMailRefItem operator=( const BmMailRefItem&);
};

class BmMailRefViewFilter;
class BmMailView;
class BmMenuController;

/*------------------------------------------------------------------------------*\
	types of messages handled by a mailref-view:
\*------------------------------------------------------------------------------*/
enum {
	BM_CHANGE_LABELS_LOCKED		= 'bmcf',
							// the user has chosen to lock/unlock the column labels
};

/*------------------------------------------------------------------------------*\
	BmMailRefView
		-	
\*------------------------------------------------------------------------------*/
class BmMailRefView : public BmListViewController
{
	typedef BmListViewController inherited;

	class ReselectionInfo {
	public:
		ReselectionInfo() : mTimeOfDeselection(0) {}
		void NoteDeselectionOf(const entry_ref& eref);
		bool QualifiesForReselection(const entry_ref& eref);
	private:
		entry_ref mEntryRef;
		bigtime_t mTimeOfDeselection;
	};

public:
	static const char* const MSG_MAILS_SELECTED;

	static const char* const MENU_MARK_AS;
	static const char* const MENU_FILTER;
	static const char* const MENU_MOVE;

	// creator-func, c'tors and d'tor:
	static BmMailRefView* CreateInstance( int32 width, int32 height);
	BmMailRefView( int32 width, int32 height);
	~BmMailRefView();

	// native methods:
	void ShowFolder( BmMailFolder* folder);
	inline void TeamUpWith( BmMailView* mv) 	{ mPartnerMailView = mv; }
	void AddSelectedRefsToMsg(BMessage* msg, BList* itemList = NULL);
	void ShowMenu( BPoint point);
	void AddMailRefMenu( BMenu* menu, BHandler* target, bool isContextMenu);
	void SendNoticesIfNeeded( bool haveSelectedRef);
	void TrashSelectedMessages();

	// overrides of listview base:
	void KeyDown(const char *bytes, int32 numBytes);
	bool InitiateDrag( BPoint point, int32 index, bool wasSelected);
	bool AcceptsDropOf( const BMessage* msg);
	void HandleDrop( BMessage* msg);
	void MessageReceived( BMessage* msg);
	void SelectionChanged( void);
	void ItemInvoked( int32 index);
	void MouseDown(BPoint point);
	void FrameResized(float width, float height);
	void ColumnWidthChanged(int32 ColumnIndex, float NewWidth);
	void WindowActivated(bool active);
	void AttachedToWindow(void);
	void ReadStateInfo();

	// overrides of controller base:
	BmString StateInfoBasename();
	BmString StateInfoFilename( bool forRead);
	BmListViewItem* CreateListViewItem( BmListModelItem* item, BMessage* archive=NULL);
	const char* ItemNameForCaption()		{ return "message"; }
	BmListViewItem* AddModelItem( BmListModelItem* item);
	void RemoveModelItem( BmListModelItem* item);

protected:
	// overrides of listcontroller base:
	void JobIsDone( bool completed);
	void PopulateLabelViewMenu( BMenu* menu);

private:
	BmRef<BmMailFolder> mCurrFolder;
	BmMailView* mPartnerMailView;
	bool mHaveSelectedRef;
	bool mStateInfoConnectedToParentFolder;
	int32 mHiddenState;
	BControl* mLockLabelsButton;

	ReselectionInfo mReselectionInfo;

	// Hide copy-constructor and assignment:
	BmMailRefView( const BmMailRefView&);
	BmMailRefView operator=( const BmMailRefView&);
};

#endif
