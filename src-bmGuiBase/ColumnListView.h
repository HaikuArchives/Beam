//		$Id$
//Column list view header file

//*** LICENSE ***
//ColumnListView, its associated classes and source code, and the other components of Santa's Gift Bag are
//being made publicly available and free to use in freeware and shareware products with a price under $25
//(I believe that shareware should be cheap). For overpriced shareware (hehehe) or commercial products,
//please contact me to negotiate a fee for use. After all, I did work hard on this class and invested a lot
//of time into it. That being said, DON'T WORRY I don't want much. It totally depends on the sort of project
//you're working on and how much you expect to make off it. If someone makes money off my work, I'd like to
//get at least a little something.  If any of the components of Santa's Gift Bag are is used in a shareware
//or commercial product, I get a free copy.  The source is made available so that you can improve and extend
//it as you need. In general it is best to customize your ColumnListView through inheritance, so that you
//can take advantage of enhancements and bug fixes as they become available. Feel free to distribute the 
//ColumnListView source, including modified versions, but keep this documentation and license with it.


#ifndef _CLV_COLUMN_LIST_VIEW_H_
#define _CLV_COLUMN_LIST_VIEW_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <ListView.h>
#include <layout.h>

#include "SantaPartsForBeam.h"

//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
#include "Colors.h"
#include "CLVColumn.h"
class CLVListItem;
class CLVColumnLabelView;
class CLVFillerView;
class CLVContainerView;
#include "PrefilledBitmap.h"
#include "BetterScrollView.h"


//******************************************************************************************************
//**** CONSTANTS AND TYPE DEFINITIONS
//******************************************************************************************************
typedef int (*CLVCompareFuncPtr)(const CLVListItem* item1, const CLVListItem* item2, int32 sort_key, int32 col_flags);

#define EXPANDER_SHIFT 14.0

extern IMPEXPSANTAPARTSFORBEAM const float darken_tint;

extern IMPEXPSANTAPARTSFORBEAM uint8 CLVRightArrowData[132];
extern IMPEXPSANTAPARTSFORBEAM uint8 CLVDownArrowData[132];

//******************************************************************************************************
//**** ColumnListView CLASS DECLARATION
//******************************************************************************************************
class IMPEXPSANTAPARTSFORBEAM ColumnListView : public BListView
{
		typedef BListView inherited;

		// archival-fieldnames:
		static const char* const MSG_DISPLAYORDER;
		static const char* const MSG_NUMSORTKEYS;
		static const char* const MSG_SORTKEY;
		static const char* const MSG_SORTMODE;
		static const char* const MSG_COLWIDTH;

	public:
		//Constructor and destructor
		ColumnListView( minimax minmax,
						BRect Frame,
						const char* Name = NULL,
						uint32 flags = B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
						list_view_type Type = B_SINGLE_SELECTION_LIST,
						bool hierarchical = false,
						bool showLabelView = true);
		virtual ~ColumnListView();
		virtual CLVContainerView* Initialize( BRect Frame, 
											 uint32 flags, 
											 uint32 ResizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
											 bool horizontal = true,					//Which scroll bars should I add, if any
											 bool vertical = true,
											 bool scroll_view_corner = true,
											 border_style border = B_NO_BORDER,		//What type of border to add, if any
											 const BFont* LabelFont = be_plain_font);

		//Archival stuff
			/*** Not implemented yet
		ColumnListView(BMessage* archive);
		static ColumnListView* Instantiate(BMessage* data);
			***/
		virtual status_t Archive(BMessage* data, bool deep = true) const;
		virtual status_t Unarchive(const BMessage* archive, bool deep = true);

		//Column setup functions
		virtual bool AddColumn(CLVColumn* Column);			//Note that a column may only be added to
															//one ColumnListView at a time, and may not
															//be added more than once to the same
															//ColumnListView without removing it
															//inbetween
		virtual bool AddColumnList(BList* NewColumns);
		virtual bool RemoveColumn(CLVColumn* Column);
		virtual bool RemoveColumns(CLVColumn* Column, int32 Count);	//Finds Column in ColumnList
																	//and removes Count columns and
																	//their data from the view
																	//and its items
		int32 CountColumns() const;
		int32 IndexOfColumn(CLVColumn* column) const;
		CLVColumn* ColumnAt(int32 column_index) const;
		virtual bool SetDisplayOrder(const int32* Order);
			//Sets the display order: each int32 in the Order list specifies the column index of the
			//next column to display.  Note that this DOES NOT get called if the user drags a
			//column, so overriding it will not inform you of user changes.  If you need that info,
			//override DisplayOrderChanged instead.  Also note that SetDisplayOrder does call
			//DisplayOrderChanged(false).
		void GetDisplayOrder(int32* order) const;
			//Fills a caller-provided array with the display order of the columns in the same format
			//as used in calling SetDisplayOrder.  The order pointer should point to an array 
			//int32 order[n] where n is the number of columns in the ColumnListView.
		void ShowColumn( int32 col_index);
		void HideColumn( int32 col_index);
		virtual void ColumnWidthChanged(int32 ColumnIndex, float NewWidth);
		virtual void DisplayOrderChanged(const int32* order);
			//Override this if you want to find out when the display order changed.
		virtual void SetSortKey(int32 ColumnIndex);
			//Set it to -1 to remove the sort key.
		virtual void AddSortKey(int32 ColumnIndex);
		void ReverseSortMode(int32 ColumnIndex);
		virtual void SetSortMode(int32 ColumnIndex,CLVSortMode Mode, bool doSort=true);
			//Override this to filter the modes
		int32 GetSorting(int32* SortKeys, CLVSortMode* SortModes) const;
			//Returns the number of used sort keys, and fills the provided arrays with the sort keys
			//by column index and sort modes, in priority order.  The pointers should point to an array 
			//int32 SortKeys[n], and an array CLVSortMode SortModes[n] where n is the number of sortable
			//columns in the ColumnListView.  Note: sorting will only occur if the key column is shown.
		void SetSorting(int32 NumberOfKeys, int32* SortKeys, CLVSortMode* SortModes);
			//Sets the sorting parameters using the same format returned by Sorting
		virtual void SortingChanged();	//Override this to detect changes to the sorting keys or modes.

		inline void SetItemSelectColor(bool window_active, rgb_color color)
		{
			if(window_active) fSelectedItemColorWindowActive = color;
			else fSelectedItemColorWindowInactive = color;
		}
		inline rgb_color ItemSelectColor(bool window_active) const
		{
			if(window_active) return fSelectedItemColorWindowActive;
			return fSelectedItemColorWindowInactive;
		}
		inline rgb_color ItemSelectColor() const
		{
			if(fWindowActive) return fSelectedItemColorWindowActive;
			return fSelectedItemColorWindowInactive;
		}
		inline rgb_color ItemSelectColorTinted() const
		{
			if(fWindowActive) return fSelectedItemColorTintedWindowActive;
			return fSelectedItemColorTintedWindowInactive;
		}

		//BView overrides
		void FrameResized(float Width, float Height);
 		void ScrollTo(BPoint point);
 		void MouseDown(BPoint point);
		void MouseUp(BPoint where);
		void MouseMoved(BPoint where, uint32 code, const BMessage* message);
		void WindowActivated(bool active);
		//
		void Draw( BRect updateRect);
		void DrawColumn( BRect updateRect, int32 column_index);


		//List functions
		virtual bool AddUnder(BListItem* item, BListItem* superitem);
		int32 DetermineSortedPosHierarchical( CLVListItem* item, uint32 startIndex);
		virtual bool AddItem(BListItem* item, int32 fullListIndex);
		virtual bool AddItem(BListItem* item);
		virtual bool AddList(BList* newItems);						//This must be a BList of
																	//CLVListItem*'s, NOT BListItem*'s
		virtual bool AddList(BList* newItems, int32 fullListIndex);	//This must be a BList of
																	//CLVListItem*'s, NOT BListItem*'s
		int32 DetermineSortedPos(CLVListItem* item);
		virtual bool RemoveItem(BListItem* item);
		virtual BListItem* RemoveItem(int32 fullListIndex);			//Actually returns CLVListItem
		virtual bool RemoveItems(int32 fullListIndex, int32 count);
		virtual void MakeEmpty();
		bool Hierarchical() const			{ return fHierarchical; }
		CLVListItem* FullListItemAt(int32 fullListIndex)  const;
		int32 FullListIndexOf(const CLVListItem* item) const;
		int32 FullListIndexOf(BPoint point) const;
		CLVListItem* FullListFirstItem() const;
		CLVListItem* FullListLastItem() const;
		bool FullListHasItem(const CLVListItem* item) const;
		int32 FullListCountItems() const;
		bool FullListIsEmpty() const;
		int32 FullListCurrentSelection(int32 index = 0) const;
		void FullListDoForEach(bool (*func)(CLVListItem*));
		void FullListDoForEach(bool (*func)(CLVListItem*, void*), void* arg2);
		CLVListItem* Superitem(const CLVListItem* item) const;
		int32 FullListNumberOfSubitems(const CLVListItem* item) const;
		virtual void Expand(CLVListItem* item);
		virtual void Collapse(CLVListItem* item);
		bool IsExpanded(int32 fullListIndex) const;
		virtual void ExpansionChanged(CLVListItem*, bool) {}
		void SetSortFunction(CLVCompareFuncPtr compare);
		void SortItems();
		virtual CLVContainerView* CreateContainer(bool horizontal, bool vertical, bool scroll_view_corner,
			border_style border, uint32 ResizingMode, uint32 flags);
		virtual void KeyDown(const char *bytes, int32 numBytes);

		inline int32 GetDisplayIndexForColumn( int32 column_index) {
			return fColumnDisplayList.IndexOf( fColumnList.ItemAt(column_index));
		}
		inline rgb_color LightColumnCol()	{ return fLightColumnCol; }
		inline rgb_color DarkColumnCol()	{ return fDarkColumnCol; }
		inline void SetStripedBackground( bool b)		{ fStripedBackground = b; }
		inline bool StripedBackground( )	{ return fStripedBackground; }
		inline void SetInsertAtSortedPos( bool b)		{ fInsertAtSortedPos = b; }
		inline bool InsertAtSortedPos( )	{ return fInsertAtSortedPos; }
		inline void ClickSetsFocus( bool b)		{ fClickSetsFocus = b; }
		inline bool ClickSetsFocus( )	{ return fClickSetsFocus; }
		inline void SetMinItemHeight( float mh)		{ fMinItemHeight = mh; }

		virtual void MessageReceived( BMessage* msg);

		static void SetExtendedSelectionPolicy( bool likeTracker);

	protected:
		void SetDisconnectScrollView( bool disconnect);
		void UpdateColumnSizesDataRectSizeScrollBars(bool scrolling_allowed = true);
		void ColumnsChanged();
		virtual void ShowLabelViewMenu( BPoint)	{ }

	protected:
		friend class CLVMainView;
		friend class CLVColumn;
		friend class CLVColumnLabelView;
		friend class CLVListItem;
		friend class CLVEasyItem;

		void EmbedInContainer(bool horizontal, bool vertical, bool scroll_view_corner, border_style border,
			uint32 ResizingMode, uint32 flags);
		void SortListArray(CLVListItem** SortArray, int32 NumberOfItems);
		void MakeEmptyPrivate();
		bool AddListPrivate(BList* newItems, int32 fullListIndex);
		bool AddItemPrivate(CLVListItem* item, int32 fullListIndex);
		void SortFullListSegment(int32 OriginalListStartIndex, int32 InsertionPoint, BList* NewList);
		BList* SortItemsInThisLevel(int32 OriginalListStartIndex);
		static int PlainBListSortFunc(BListItem** item1, BListItem** item2);
		static int HierarchicalBListSortFunc(BListItem** item1, BListItem** item2);
		void AssertWindowLocked() const;
		virtual CLVContainerView* ScrollView() 	{ return fScrollView; }
		virtual CLVColumnLabelView* ColumnLabelView() 	{ return fColumnLabelView; }

		CLVColumnLabelView* fColumnLabelView;
		CLVContainerView* fScrollView;
		bool fHierarchical;
		bool fShowLabelView;
		BList fColumnList;
		BList fColumnDisplayList;
		BList fSortKeyList;		//List contains CLVColumn pointers
		BList fFullItemList;
		BList m_column_types;	//List of int32's converted from CLVColumnTypes
		PrefilledBitmap fRightArrow;
		PrefilledBitmap fDownArrow;
		int32 fExpanderColumn;
		CLVCompareFuncPtr fCompare;
		bool fWatchingForDrag;
		BPoint fLastMouseDown;
		int32 fNoKeyMouseDownItemIndex;
		rgb_color fSelectedItemColorWindowActive;
		rgb_color fSelectedItemColorWindowInactive;
		rgb_color fSelectedItemColorTintedWindowActive;
		rgb_color fSelectedItemColorTintedWindowInactive;
		rgb_color fDarkColumnCol;
		rgb_color fLightColumnCol;
		bool fWindowActive;
		BScrollBar* fDeactivatedVerticalBar;
		bool fStripedBackground;
		bool fInsertAtSortedPos;
		bool fClickSetsFocus;
		static int32 fExtendSelMask;
		static int32 fToggleSelMask;
		bool fExtendingDownwards;
		float fMinItemHeight;

		minimax fMinMax;		
};

class IMPEXPSANTAPARTSFORBEAM CLVContainerView : public MView, public BetterScrollView
{
		typedef BetterScrollView inherited;
	public:
		CLVContainerView( minimax minmax, ColumnListView* target, uint32 resizingMode, 
								uint32 flags, bool horizontal, bool vertical,
								bool scroll_view_corner, border_style border);
		~CLVContainerView();
	
		// adapted for liblayout
		virtual minimax layoutprefs();
		virtual BRect layout(BRect);
		// (end of adaptation)	

	private:
		friend class ColumnListView;
		bool IsBeingDestroyed;
};



#endif
