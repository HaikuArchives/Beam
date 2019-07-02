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


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <ClassInfo.h>
#include <Region.h>
#include <Window.h>


// adapted for liblayout
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BMessageRunner;
	class BRect;
	class BStatusBar;
#endif
#include "layout.h"

//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include <Debug.h>
#include "BmBasics.h"
#include "ColumnListView.h"
#include "CLVColumnLabelView.h"
#include "CLVColumn.h"
#include "CLVListItem.h"

const char* const ColumnListView::MSG_DISPLAYORDER = 	"bm:dsplord";
const char* const ColumnListView::MSG_NUMSORTKEYS = 	"bm:nsort";
const char* const ColumnListView::MSG_SORTKEY = 		"bm:sortk";
const char* const ColumnListView::MSG_SORTMODE = 		"bm:sortm";
const char* const ColumnListView::MSG_COLWIDTH = 		"bm:colw";
const char* const ColumnListView::MSG_LAYOUTLOCKED =	"bm:laylock";

int32 ColumnListView::fExtendSelMask = B_SHIFT_KEY;
int32 ColumnListView::fToggleSelMask = B_OPTION_KEY|B_COMMAND_KEY;

//******************************************************************************************************
//**** ColumnListView CLASS DEFINITION
//******************************************************************************************************
ColumnListView::ColumnListView(BRect Frame, const char *Name,
	uint32 flags, list_view_type Type, bool hierarchical, bool showLabelView)
: BListView(Frame,Name,Type,B_FOLLOW_NONE,flags),
fHierarchical( hierarchical),
fShowLabelView( showLabelView),
fColumnList(6),
fColumnDisplayList(6),
fSortKeyList(6),
fFullItemList(32),
fExpanderColumn( -1),
fCompare( NULL),
fWatchingForDrag( false),
fSelectedItemColorWindowActive( ui_color( B_MENU_SELECTED_BACKGROUND_COLOR)),
fSelectedItemColorWindowInactive( ui_color( B_MENU_SELECTED_BACKGROUND_COLOR)),
fSelectedItemColorTintedWindowActive( BmWeakenColor( B_MENU_SELECTED_BACKGROUND_COLOR, 1)),
fSelectedItemColorTintedWindowInactive( BmWeakenColor( B_MENU_SELECTED_BACKGROUND_COLOR, 1)),
fDarkColumnCol( BmWeakenColor( B_DOCUMENT_BACKGROUND_COLOR, 1)),
fLightColumnCol( ui_color( B_DOCUMENT_BACKGROUND_COLOR)),
fWindowActive( false),
fDeactivatedVerticalBar( NULL),
fStripedBackground( false),
fInsertAtSortedPos( true),
fClickSetsFocus( true),
fExtendingDownwards( true),
fAvoidColPushing( false),
fMinItemHeight( 5),
fScrollView(NULL)
{
	SetViewColor( B_TRANSPARENT_COLOR);
	SetFont(&bm_plain_font);
	BFont labelFont(bm_plain_font);
	//Create the column titles bar view
	font_height FontAttributes;
	labelFont.GetHeight(&FontAttributes);
	float LabelFontHeight = ceilf(FontAttributes.ascent) + ceilf(FontAttributes.descent);
	BRect labelFrame = Frame;
	labelFrame.bottom = labelFrame.top + LabelFontHeight + 2.0f;
	fColumnLabelView = new CLVColumnLabelView( labelFrame, this, &labelFont);
}

ColumnListView::~ColumnListView()
{
	//Delete all list columns
	int32 ColumnCount = fColumnList.CountItems();
	for(int32 Counter = ColumnCount-1; Counter >= 0; Counter--)
	{
		CLVColumn* Item = (CLVColumn*)fColumnList.RemoveItem(Counter);
		if(Item)
			delete Item;
	}
	// remove label view if that is not being displayed 
	// (and thus not being destroyed as part of view hierarchy):
	if (!fShowLabelView)
		delete fColumnLabelView;
}

// for Zeta only, react on color changes:
status_t ColumnListView::UISettingsChanged(const BMessage* changes, uint32 flags) {
	fSelectedItemColorWindowActive = ui_color( B_MENU_SELECTED_BACKGROUND_COLOR);
	fSelectedItemColorWindowInactive = ui_color( B_MENU_SELECTED_BACKGROUND_COLOR);
	fSelectedItemColorTintedWindowActive = BmWeakenColor( B_MENU_SELECTED_BACKGROUND_COLOR, 1);
	fSelectedItemColorTintedWindowInactive = BmWeakenColor( B_MENU_SELECTED_BACKGROUND_COLOR, 1);
	fDarkColumnCol = BmWeakenColor( B_DOCUMENT_BACKGROUND_COLOR, 1);
	fLightColumnCol = ui_color( B_DOCUMENT_BACKGROUND_COLOR);
	Invalidate();
	return B_OK;
}

void ColumnListView::AttachedToWindow() {
	inherited::AttachedToWindow();
	fScrollView = dynamic_cast<BetterScrollView*>(Parent());
	if (fScrollView && fShowLabelView) {
		BRect viewFrame = Frame();
		BRect labelFrame = fColumnLabelView->Frame();
		// Resize the main view to make room for the CLVColumnLabelView (and add 1 more line
		// that represents the lower border of the labelview):
		ResizeTo(viewFrame.Width(), viewFrame.Height()-(labelFrame.Height()+1));
		MoveTo(viewFrame.left, viewFrame.top+labelFrame.Height()+1);
		fColumnLabelView->MoveTo(viewFrame.left, viewFrame.top);
		fScrollView->AddChild(fColumnLabelView);
	}

	//Complete the setup
	UpdateDataRect();
	fColumnLabelView->UpdateDragGroups();
}

void ColumnListView::UpdateDataRect(bool scrolling_allowed)
{
	//Figure out the width
	float ColumnBegin;
	float ColumnEnd = -1.0;
	float DataWidth = 0.0;
	bool NextPushedByExpander = false;
	int32 NumberOfColumns = fColumnDisplayList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
		if(NextPushedByExpander)
			Column->fPushedByExpander = true;
		else
			Column->fPushedByExpander = false;
		if(Column->IsShown())
		{
			float ColumnWidth = Column->Width();
			ColumnBegin = ColumnEnd + 1.0f;
			ColumnEnd = ColumnBegin + ColumnWidth;
			Column->fColumnBegin = ColumnBegin;
			Column->fColumnEnd = ColumnEnd;
			DataWidth = Column->fColumnEnd;
			if(NextPushedByExpander)
				if(!(Column->fFlags & CLV_PUSH_PASS))
					NextPushedByExpander = false;
			if(Column->fFlags & CLV_EXPANDER)
				//Set the next column to be pushed
				NextPushedByExpander = true;
		}
	}

	//Figure out the height
	float DataHeight = 0.0;
	int32 NumberOfItems = CountItems();
	for(int32 Counter2 = 0; Counter2 < NumberOfItems; Counter2++) {
		DataHeight += ItemAt(Counter2)->Height();
#ifndef __HAIKU__
		// BeOS seems to compute the item height incorrectly, returning one pixel less than
		// will be used to draw the item, we compensate:
		++DataHeight;
#endif
	}

	//Update the scroll bars
	if (fScrollView)
		fScrollView->SetDataRect(BRect(0,0,DataWidth,DataHeight),scrolling_allowed);
}


void ColumnListView::ColumnsChanged()
{
	//Any previous column dragging/resizing will get corrupted, so deselect
	if(fColumnLabelView->fColumnClicked)
		fColumnLabelView->fColumnClicked = NULL;

	//Update the internal sizes and grouping of the columns and sizes of drag groups
	UpdateDataRect();
	fColumnLabelView->UpdateDragGroups();
	fColumnLabelView->Invalidate();
	Invalidate();
}


void ColumnListView::AssertWindowLocked() const
{
#ifdef DEBUG
	BWindow* ParentWindow = Window();
	if(ParentWindow && !ParentWindow->IsLocked())
		_debuggerAssert(__FILE__,__LINE__,(char*)"The looper must be locked.");
#endif
}


bool ColumnListView::AddColumn(CLVColumn* Column)
//Adds a column to the ColumnListView at the end of the list.  Returns true if successful.
{
	AssertWindowLocked();
	int32 NumberOfColumns = fColumnList.CountItems();
	int32 DisplayIndex = NumberOfColumns;

	//Make sure a second Expander is not being added
	if(Column->fFlags & CLV_EXPANDER)
	{
		if(!fHierarchical)
			return false;
		for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
			if(((CLVColumn*)fColumnList.ItemAt(Counter))->fFlags & CLV_EXPANDER)
				return false;
		if(Column->IsShown())
			fExpanderColumn = NumberOfColumns;
	}

	//Make sure this column hasn't already been added to another ColumnListView
	if(Column->fParent != NULL)
		return false;

	//Check if this should be locked at the beginning or end, and adjust its position if necessary
	if(!Column->Flags() & CLV_LOCK_AT_END)
	{
		bool Repeat;
		if(Column->Flags() & CLV_LOCK_AT_BEGINNING)
		{
			//Move it to the beginning, after the last CLV_LOCK_AT_BEGINNING item
			DisplayIndex = 0;
			Repeat = true;
			while(Repeat && DisplayIndex < NumberOfColumns)
			{
				Repeat = false;
				CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex);
				if(LastColumn->Flags() & CLV_LOCK_AT_BEGINNING)
				{
					DisplayIndex++;
					Repeat = true;
				}
			}
		}
		else
		{
			//Make sure it isn't after a CLV_LOCK_AT_END item
			Repeat = true;
			while(Repeat && DisplayIndex > 0)
			{
				Repeat = false;
				CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex-1);
				if(LastColumn->Flags() & CLV_LOCK_AT_END)
				{
					DisplayIndex--;
					Repeat = true;
				}
			}
		}
	}

	//Add the column to the display list in the appropriate position
	fColumnDisplayList.AddItem(Column, DisplayIndex);

	//Add the column to the end of the column list
	fColumnList.AddItem(Column);

	//Tell the column it belongs to me now
	Column->fParent = this;

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::AddColumnList(BList* NewColumns)
//Adds a BList of CLVColumn's to the ColumnListView at the position specified, or at the end of the list
//if AtIndex == -1.  Returns true if successful.
{
	AssertWindowLocked();
	int32 NumberOfColumns = int32(fColumnList.CountItems());
	int32 NumberOfColumnsToAdd = int32(NewColumns->CountItems());

	//Make sure a second CLVExpander is not being added
	int32 Counter;
	int32 NumberOfExpanders = 0;
	for(Counter = 0; Counter < NumberOfColumns; Counter++)
		if(((CLVColumn*)fColumnList.ItemAt(Counter))->fFlags & CLV_EXPANDER)
			NumberOfExpanders++;
	int32 SetfExpanderColumnTo = -1;
	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
	{
		CLVColumn* ThisColumn = (CLVColumn*)NewColumns->ItemAt(Counter);
		if(ThisColumn->fFlags & CLV_EXPANDER)
		{
			NumberOfExpanders++;
			if(ThisColumn->IsShown())
				SetfExpanderColumnTo = NumberOfColumns + Counter;
		}
	}
	if(NumberOfExpanders != 0 && !fHierarchical)
		return false;
	if(NumberOfExpanders > 1)
		return false;
	if(SetfExpanderColumnTo != -1)
		fExpanderColumn = SetfExpanderColumnTo;

	//Make sure none of these columns have already been added to a ColumnListView
	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
		if(((CLVColumn*)NewColumns->ItemAt(Counter))->fParent != NULL)
			return false;
	//Make sure none of these columns are being added twice
	for(Counter = 0; Counter < NumberOfColumnsToAdd-1; Counter++)
		for(int32 Counter2 = Counter+1; Counter2 < NumberOfColumnsToAdd; Counter2++)
			if(NewColumns->ItemAt(Counter) == NewColumns->ItemAt(Counter2))
				return false;

	for(Counter = 0; Counter < NumberOfColumnsToAdd; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)NewColumns->ItemAt(Counter);
		//Check if this should be locked at the beginning or end, and adjust its position if necessary
		int32 DisplayIndex = NumberOfColumns;
		if(!Column->Flags() & CLV_LOCK_AT_END)
		{
			bool Repeat;
			if(Column->Flags() & CLV_LOCK_AT_BEGINNING)
			{
				//Move it to the beginning, after the last CLV_LOCK_AT_BEGINNING item
				DisplayIndex = 0;
				Repeat = true;
				while(Repeat && DisplayIndex < NumberOfColumns)
				{
					Repeat = false;
					CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex);
					if(LastColumn->Flags() & CLV_LOCK_AT_BEGINNING)
					{
						DisplayIndex++;
						Repeat = true;
					}
				}
			}
			else
			{
				//Make sure it isn't after a CLV_LOCK_AT_END item
				Repeat = true;
				while(Repeat && DisplayIndex > 0)
				{
					Repeat = false;
					CLVColumn* LastColumn = (CLVColumn*)fColumnDisplayList.ItemAt(DisplayIndex-1);
					if(LastColumn->Flags() & CLV_LOCK_AT_END)
					{
						DisplayIndex--;
						Repeat = true;
					}
				}
			}
		}

		//Add the column to the display list in the appropriate position
		fColumnDisplayList.AddItem(Column, DisplayIndex);

		//Tell the column it belongs to me now
		Column->fParent = this;

		NumberOfColumns++;
	}

	//Add the columns to the end of the column list
	fColumnList.AddList(NewColumns);

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::RemoveColumn(CLVColumn* Column)
//Removes a CLVColumn from the ColumnListView.  Returns true if successful.
{
	AssertWindowLocked();
	if(!fColumnList.HasItem(Column))
		return false;
	int32 ColumnIndex = fSortKeyList.IndexOf(Column);
	if(ColumnIndex >= 0)
		fSortKeyList.RemoveItem(ColumnIndex);
		
	if(Column->fFlags & CLV_EXPANDER)
		fExpanderColumn = -1;

	//Remove Column from the column and display lists
	fColumnDisplayList.RemoveItem(Column);
	fColumnList.RemoveItem(Column);

	//Tell the column it has been removed
	Column->fParent = NULL;

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


bool ColumnListView::RemoveColumns(CLVColumn* Column, int32 Count)
//Finds Column in ColumnList and removes Count columns and their data from the view and its items
{
	AssertWindowLocked();
	int32 ColumnIndex = fColumnList.IndexOf(Column);
	if(ColumnIndex < 0)
		return false;
	if(ColumnIndex + Count > fColumnList.CountItems())
		return false;

	//Remove columns from the column and display lists
	for(int32 Counter = ColumnIndex; Counter < ColumnIndex+Count; Counter++)
	{
		CLVColumn* ThisColumn = (CLVColumn*)fColumnList.ItemAt(Counter);
		fColumnDisplayList.RemoveItem(ThisColumn);

		int32 SortIndex = fSortKeyList.IndexOf(Column);
		if(SortIndex >= 0)
			fSortKeyList.RemoveItem(SortIndex);

		if(ThisColumn->fFlags & CLV_EXPANDER)
			fExpanderColumn = -1;

		//Tell the column it has been removed
		ThisColumn->fParent = NULL;
	}
	fColumnList.RemoveItems(ColumnIndex,Count);

	//Set the scroll bars and tell views to update
	ColumnsChanged();
	return true;
}


int32 ColumnListView::CountColumns() const
{
	AssertWindowLocked();
	return fColumnList.CountItems();
}


int32 ColumnListView::IndexOfColumn(CLVColumn* column) const
{
	AssertWindowLocked();
	return fColumnList.IndexOf(column);
}


CLVColumn* ColumnListView::ColumnAt(int32 column_index) const
{
	AssertWindowLocked();
	return (CLVColumn*)fColumnList.ItemAt(column_index);
}


bool ColumnListView::SetDisplayOrder(const int32* ColumnOrder)
//Sets the display order using a BList of CLVColumn's
{
	AssertWindowLocked();
	//Add the items to the display list in order
	fColumnDisplayList.MakeEmpty();
	int32 ColumnsToSet = fColumnList.CountItems();
	for(int32 Counter = 0; Counter < ColumnsToSet; Counter++)
	{
		int32 pos = ColumnOrder[Counter];
		if (pos == -1)
			continue;
		if(pos >= ColumnsToSet)
			return false;
		for(int32 Counter2 = 0; Counter2 < Counter; Counter2++)
			if(pos == ColumnOrder[Counter2])
				return false;
		CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( pos);
		if (column)
			fColumnDisplayList.AddItem( column);
	}

	//Update everything about the columns
	ColumnsChanged();

	//Let the program know that the display order changed.
	DisplayOrderChanged(ColumnOrder);
	return true;
}

void ColumnListView::ColumnWidthChanged(int32 ColumnIndex, float NewWidth)
{
	CLVColumn* ThisColumn = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
	if(ThisColumn->fFlags & CLV_TELL_ITEMS_WIDTH)
	{
		//Figure out what the limit is for expanders pushing other columns
		float PushMax = 100000;
		if(ThisColumn->fPushedByExpander || (ThisColumn->fFlags & CLV_EXPANDER))
		{
			int32 NumberOfColumns = fColumnDisplayList.CountItems();
			for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
			{
				CLVColumn* SomeColumn = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
				if((SomeColumn->fFlags & CLV_EXPANDER) || SomeColumn->fPushedByExpander)
					PushMax = SomeColumn->fColumnEnd;
			}
		}

		int32 number_of_items;
		if(fHierarchical)
			number_of_items = fFullItemList.CountItems();
		else
			number_of_items = CountItems();
		float ColumnEnd = ThisColumn->fColumnEnd;
		for(int32 Counter = 0; Counter < number_of_items; Counter++)
		{
			CLVListItem* ThisItem;
			if(fHierarchical)
				ThisItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
			else
				ThisItem = (CLVListItem*)ItemAt(Counter);
			if(ThisColumn->fPushedByExpander || (ThisColumn->fFlags & CLV_EXPANDER))
			{
				float ColumnWidth = NewWidth;
				float ExpanderShift = float(ThisItem->OutlineLevel()) * EXPANDER_SHIFT;
				if(ThisColumn->fFlags & CLV_EXPANDER)
				{
					if(ColumnEnd + ExpanderShift > PushMax)
						ColumnWidth += PushMax - ColumnEnd;
					else
						ColumnWidth += ExpanderShift;
				}
				else
				{
					if(ColumnEnd + ExpanderShift > PushMax)
					{
						ColumnWidth -= (ColumnEnd + ExpanderShift - PushMax);
						if(ColumnWidth < 0)
							ColumnWidth = 0;
					}
				}
				if(ExpanderShift > 0)
				{
					int32 DisplayIndex = IndexOf(ThisItem);
					if(DisplayIndex >= 0)
					{
						//The column scrolling scrolled some content that is encroaching into the next
						//column and needs to be invalidated now
						BRect InvalidateRect = ItemFrame(DisplayIndex);
						InvalidateRect.left = ColumnEnd;
						InvalidateRect.right = ColumnEnd + ExpanderShift;
						if(InvalidateRect.right > PushMax)
							InvalidateRect.right = PushMax;
						if(InvalidateRect.right >= InvalidateRect.left)
						Invalidate(InvalidateRect);
					}
				}
				ThisItem->ColumnWidthChanged(ColumnIndex,ColumnWidth);
			}
			else
				ThisItem->ColumnWidthChanged(ColumnIndex,NewWidth);
		}
	}
}


void ColumnListView::DisplayOrderChanged(const int32* order)
{
	int num_columns = fColumnList.CountItems();

	//Figure out what the limit is for expanders pushing other columns
	float PushMax = 100000;
	CLVColumn* ThisColumn;
	if(fHierarchical)
		for(int32 Counter = 0; Counter < num_columns; Counter++)
		{
			ThisColumn = (CLVColumn*)fColumnDisplayList.ItemAt(Counter);
			if (!ThisColumn)
				continue;
			if((ThisColumn->fFlags & CLV_EXPANDER) || ThisColumn->fPushedByExpander)
				PushMax = ThisColumn->fColumnEnd;
		}

	int32 number_of_items;
	if(fHierarchical)
		number_of_items = fFullItemList.CountItems();
	else
		number_of_items = CountItems();
	for(int column = 0; column < num_columns; column++)
	{
		ThisColumn = (CLVColumn*)fColumnList.ItemAt(column);
		if(ThisColumn && ThisColumn->fFlags & CLV_TELL_ITEMS_WIDTH)
		{
			float ColumnLeft = ThisColumn->fColumnBegin;
			float ColumnRight = ThisColumn->fColumnEnd;
			for(int32 item_index = 0; item_index < number_of_items; item_index++)
			{
				CLVListItem* item;
				if(fHierarchical)
					item = (CLVListItem*)fFullItemList.ItemAt(item_index);
				else
					item = (CLVListItem*)ItemAt(item_index);
				int32 DisplayIndex = IndexOf(item);	
				if(DisplayIndex >= 0 && ThisColumn->IsShown())
				{
					BRect ThisColumnRect = ItemFrame(DisplayIndex);
					ThisColumnRect.left = ColumnLeft;
					ThisColumnRect.right = ColumnRight;
					if(ThisColumn->fFlags & CLV_EXPANDER)
					{
						ThisColumnRect.right += float(item->OutlineLevel()) * EXPANDER_SHIFT;
						if(ThisColumnRect.right > PushMax)
							ThisColumnRect.right = PushMax;
					}
					else
					{
						if(ThisColumn->fPushedByExpander)
						{
							float Shift = float(item->OutlineLevel()) * EXPANDER_SHIFT;
							ThisColumnRect.left += Shift;
							ThisColumnRect.right += Shift;
							if(Shift > 0.0f && ThisColumnRect.right > PushMax)
								ThisColumnRect.right = PushMax;
						}
					}
				
					if(ThisColumnRect.right >= ThisColumnRect.left)
						item->FrameChanged(column,ThisColumnRect);
					else
						item->FrameChanged(column,BRect(-1,-1,-1,-1));
				}
				else
					item->FrameChanged(column,BRect(-1,-1,-1,-1));
			}
		}
	}
	//Get rid of a warning:
	order = NULL;
}


void ColumnListView::GetDisplayOrder(int32* order) const
{
	AssertWindowLocked();
	int32 ColumnsInList = fColumnList.CountItems();
	for(int32 Counter = 0; Counter < ColumnsInList; Counter++)
		order[Counter] = int32(fColumnList.IndexOf(fColumnDisplayList.ItemAt(Counter)));
}

void ColumnListView::SetSortKey(int32 ColumnIndex)
{
	AssertWindowLocked();
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		Column = NULL;
	if(fSortKeyList.ItemAt(0) != Column || Column == NULL)
	{
		BRect LabelBounds = fColumnLabelView->Bounds();
		//Need to remove old sort keys and erase all the old underlines
		int32 SortKeyCount = fSortKeyList.CountItems();
		for(int32 Counter = 0; Counter < SortKeyCount; Counter++)
		{
			CLVColumn* UnderlineColumn = (CLVColumn*)fSortKeyList.ItemAt(Counter);
			if(UnderlineColumn->fSortMode != NoSort)
				fColumnLabelView->Invalidate(BRect(UnderlineColumn->fColumnBegin,LabelBounds.top,
					UnderlineColumn->fColumnEnd,LabelBounds.bottom));
		}
		fSortKeyList.MakeEmpty();

		if(Column)
		{
			fSortKeyList.AddItem(Column);
			if(Column->fSortMode == NoSort)
				SetSortMode(ColumnIndex,Ascending,false);
			SortItems();
			//Need to draw new underline
			fColumnLabelView->Invalidate(BRect(Column->fColumnBegin,LabelBounds.top,Column->fColumnEnd,
				LabelBounds.bottom));
		}
	}

	SortingChanged();
}


void ColumnListView::AddSortKey(int32 ColumnIndex)
{
	AssertWindowLocked();
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		Column = NULL;
	if(Column && !fSortKeyList.HasItem(Column))
	{
		BRect LabelBounds = fColumnLabelView->Bounds();
		fSortKeyList.AddItem(Column);
		if(Column->fSortMode == NoSort)
			SetSortMode(ColumnIndex,Ascending, false);
		SortItems();
		//Need to update sorting-order
		fColumnLabelView->Invalidate(BRect(Column->fColumnBegin,LabelBounds.top,Column->fColumnEnd,
			LabelBounds.bottom));
	}
	SortingChanged();
}

bool ColumnListView::IsSortKey(int32 ColumnIndex)
{
	AssertWindowLocked();
	CLVColumn* Column = NULL;
	if (ColumnIndex >= 0)
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
	return (Column && fSortKeyList.HasItem(Column));
}

void ColumnListView::SetSortMode(int32 ColumnIndex,CLVSortMode Mode, bool doSort)
{
	AssertWindowLocked();
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		return;
	if(Column->fSortMode != Mode)
	{
		BRect invalidRect = fColumnLabelView->Bounds();
		Column->fSortMode = Mode;
		if(Mode == NoSort && fSortKeyList.HasItem(Column))
			fSortKeyList.RemoveItem(Column);
		else {
			invalidRect.left = Column->fColumnBegin;
			invalidRect.right = Column->fColumnEnd;
		}
		if (doSort) 
			SortItems();
		//Need to update sorting-order
		fColumnLabelView->Invalidate( invalidRect);
	}

	SortingChanged();
}


void ColumnListView::ReverseSortMode(int32 ColumnIndex)
{
	AssertWindowLocked();
	CLVColumn* Column;
	if(ColumnIndex >= 0)
	{
		Column = (CLVColumn*)fColumnList.ItemAt(ColumnIndex);
		if(!(Column->Flags()&CLV_SORT_KEYABLE))
			return;
	}
	else
		return;
	if(Column->fSortMode == Ascending)
		SetSortMode(ColumnIndex,Descending);
	else if(Column->fSortMode == Descending)
		SetSortMode(ColumnIndex,NoSort);
	else if(Column->fSortMode == NoSort)
		SetSortMode(ColumnIndex,Ascending);
}


int32 ColumnListView::GetSorting(int32* SortKeys, CLVSortMode* SortModes) const
{
	AssertWindowLocked();
	int32 NumberOfKeys = fSortKeyList.CountItems();
	for(int32 Counter = 0; Counter < NumberOfKeys; Counter++)
	{
		CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(Counter);
		SortKeys[Counter] = IndexOfColumn(Column);
		SortModes[Counter] = Column->SortMode();
	}
	return NumberOfKeys;
}


void ColumnListView::SetSorting(int32 NumberOfKeys, int32* SortKeys, CLVSortMode* SortModes)
{
	AssertWindowLocked();

	//Need to remove old sort keys and erase all the old underlines
	BRect LabelBounds = fColumnLabelView->Bounds();
	int32 SortKeyCount = fSortKeyList.CountItems();
	for(int32 Counter = 0; Counter < SortKeyCount; Counter++)
	{
		CLVColumn* UnderlineColumn = (CLVColumn*)fSortKeyList.ItemAt(Counter);
		if(UnderlineColumn->fSortMode != NoSort)
			fColumnLabelView->Invalidate(BRect(UnderlineColumn->fColumnBegin,LabelBounds.top,
				UnderlineColumn->fColumnEnd,LabelBounds.bottom));
	}
	fSortKeyList.MakeEmpty();

	for(int32 Counter = 0; Counter < NumberOfKeys; Counter++)
	{
		if(Counter == 0)
			SetSortKey(SortKeys[0]);
		else
			AddSortKey(SortKeys[Counter]);
		SetSortMode(SortKeys[Counter],SortModes[Counter]);
	}

	SortingChanged();
}


void ColumnListView::SortingChanged()
{ }


void ColumnListView::FrameResized(float width, float height)
{
	UpdateDataRect();
	if (fColumnLabelView)
		fColumnLabelView->ResizeTo(width, fColumnLabelView->Frame().Height());
	BListView::FrameResized(width,height);
}


void ColumnListView::ScrollTo(BPoint point)
{
	BListView::ScrollTo(point);
	fColumnLabelView->ScrollTo(BPoint(point.x,0.0));
}


void ColumnListView::MouseDown(BPoint where)
{
	//BListView::MouseDown gives undesirable behavior during click-and drag (it reselects instead of
	//doing nothing to allow for initiating drag & drop), so I need to replace the selection behavior,
	//and while I'm at it, I'll just do an InitiateDrag hook function.
	BMessage* message = Window()->CurrentMessage();
	int32 item_index = IndexOf(where);
	int32 modifier_keys = message->FindInt32("modifiers");
	int32 clicks = message->FindInt32("clicks");
	if(message->FindInt32("buttons") != B_PRIMARY_MOUSE_BUTTON)
		return;
	if (fClickSetsFocus)
		MakeFocus( true);
	if(item_index >= 0)
	{
		CLVListItem* clicked_item = (CLVListItem*)BListView::ItemAt(item_index);
		if(clicked_item->IsSuperItem() && clicked_item->ExpanderRectContains(where))
		{
			if(clicked_item->IsExpanded())
				Collapse(clicked_item);
			else
				Expand(clicked_item);
		}
		else
		{
			if(!clicked_item->IsSelected())
			{
				//Clicked a new item...select it.
				list_view_type type = ListType();
				if((modifier_keys & fExtendSelMask) && type != B_SINGLE_SELECTION_LIST)
				{
					//If shift held down, expand the selection to include all intervening items.
					int32 min_selection = 0x7FFFFFFF;
					int32 max_selection = -1;
					int32 selection_index = 0;
					int32 selection_item_index;
					while((selection_item_index = CurrentSelection(selection_index)) != -1)
					{
						if(min_selection > selection_item_index)
							min_selection = selection_item_index;
						if(max_selection < selection_item_index)
							max_selection = selection_item_index;
						selection_index++;
					}
					if(min_selection == 0x7FFFFFFF)
						Select(item_index,false);
					else
					{
						if(min_selection > item_index)
							min_selection = item_index;
						if(max_selection < item_index)
							max_selection = item_index;
						Select(min_selection,max_selection,false);
					}
				}
				else if((modifier_keys & fToggleSelMask) && type != B_SINGLE_SELECTION_LIST)
					//If option held down, expand the selection to include just it.
					Select(item_index,true);
				else
					//If neither key held down, select this item alone.
					Select(item_index,false);

				//Also watch for drag of the new selection
				fLastMouseDown = where;
				fNoKeyMouseDownItemIndex = -1;
				fWatchingForDrag = true;
			}
			else
			{
				//Clicked an already selected item...
				if(modifier_keys & fToggleSelMask)
					//if option held down, remove it.
					Deselect(item_index);
				else if(modifier_keys & fExtendSelMask)
				{
					//If shift held down, ignore it and just watch for drag.
					fLastMouseDown = where;
					fNoKeyMouseDownItemIndex = -1;
					fWatchingForDrag = true;
				}
				else
				{
					if(clicks > 1)
						Invoke(InvocationMessage());
					else
					{
						//If neither key held down, watch for drag, but if no drag and just a click on
						//this item, select it alone at mouse up.
						fLastMouseDown = where;
						fNoKeyMouseDownItemIndex = item_index;
						fWatchingForDrag = true;
					}
				}
			}
//	[zooey]: it seems we don't need this and it sometimes causes problems
//			   with popups (like Alerts).
//			if(fWatchingForDrag)
//				SetMouseEventMask(B_POINTER_EVENTS,B_NO_POINTER_HISTORY);
		}
	}
	else
	{
		//Clicked outside of any items.  If no shift or option key, deselect all.
		if(!(modifier_keys & (B_SHIFT_KEY | B_OPTION_KEY | B_CONTROL_KEY )))
			DeselectAll();
	}
}


void ColumnListView::MouseUp(BPoint where)
{
	if(fWatchingForDrag && fNoKeyMouseDownItemIndex != -1)
		Select(fNoKeyMouseDownItemIndex,false);
	fNoKeyMouseDownItemIndex = -1;
	fWatchingForDrag = false;
	inherited::MouseUp( where);
}


void ColumnListView::MouseMoved(BPoint where, uint32, const BMessage *)
{
	if(fWatchingForDrag && (where.x<fLastMouseDown.x-4 || where.x>fLastMouseDown.x+4 ||
		where.y<fLastMouseDown.y-4 || where.y>fLastMouseDown.y+4))
	{
		BPoint p;
		uint32 buttons;
		GetMouse( &p, &buttons);
		if (!(buttons & B_PRIMARY_MOUSE_BUTTON)) {
			// sometimes, MouseUp() has not been invoked, although the mouse-button
			// had been released some time ago. We check if the mouse-button is really
			// still down, if not, we break out:
			fWatchingForDrag = false;
			return;
		}
		InitiateDrag(fLastMouseDown,IndexOf(fLastMouseDown),true);
		fNoKeyMouseDownItemIndex = -1;
		fWatchingForDrag = false;
	}
}


void ColumnListView::WindowActivated(bool active)
{
	fWindowActive = active;
	if(fSelectedItemColorWindowActive.red != fSelectedItemColorWindowInactive.red ||
		fSelectedItemColorWindowActive.green != fSelectedItemColorWindowInactive.green ||
		fSelectedItemColorWindowActive.blue != fSelectedItemColorWindowInactive.blue ||
		fSelectedItemColorWindowActive.alpha != fSelectedItemColorWindowInactive.alpha)
	{
		Invalidate();
	}
	BListView::WindowActivated(active);
}


bool ColumnListView::AddUnder(BListItem* a_item, BListItem* a_superitem)
{
	AssertWindowLocked();

	if(!fHierarchical)
		return false;

	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	CLVListItem* superitem cast_as(a_superitem,CLVListItem);
	if(item == NULL || superitem == NULL)
		return false;

	//Find the superitem in the full list and display list (if shown)
	int32 SuperItemPos = fFullItemList.IndexOf(superitem);
	if(SuperItemPos < 0)
		return false;

	//Add the item under the superitem in the full list
	int32 ItemPos = SuperItemPos + 1;
	if (fInsertAtSortedPos) {
		ItemPos = DetermineSortedPosHierarchical( item, SuperItemPos+1);
		// now skip over any child-items belonging to the item before us:
		for(	CLVListItem* Temp = (CLVListItem*)fFullItemList.ItemAt(ItemPos);
				Temp && Temp->OutlineLevel() > item->OutlineLevel();
		) {
			Temp = (CLVListItem*)fFullItemList.ItemAt(++ItemPos);
		}
	} else {
		ItemPos = SuperItemPos + 1;
		// now skip to end of items on same level as ourselves (add new
		// item to the end):
		for(	CLVListItem* Temp = (CLVListItem*)fFullItemList.ItemAt(ItemPos);
				Temp && Temp->OutlineLevel() >= item->OutlineLevel();
		) {
			Temp = (CLVListItem*)fFullItemList.ItemAt(++ItemPos);
		}
	}
	bool result = AddItemPrivate(item,ItemPos);
	UpdateDataRect();
	return result;
}


int32 ColumnListView::DetermineSortedPosHierarchical( CLVListItem* item, uint32 startIndex)
{
	int32 SortDepth = fSortKeyList.CountItems();

	//Create a new BList of the items in this level
	int32 Counter = startIndex;
	uint32 ThisLevel = item->OutlineLevel();
	BList ThisLevelItems(16);
	while(true)
	{
		CLVListItem* ThisItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
		if(ThisItem == NULL)
			break;
		uint32 ThisItemLevel = ThisItem->OutlineLevel();
		if(ThisItemLevel == ThisLevel)
		{
			ThisLevelItems.AddItem((void*)Counter);
		}
		else if(ThisItemLevel < ThisLevel)
			break;
		Counter++;
	}

	// now find sorted position for new item within this list:
	int32 numItems = ThisLevelItems.CountItems();
	if (!numItems)
		return startIndex;
	if (!fCompare || !SortDepth)
		return ((int32)(addr_t)ThisLevelItems.LastItem())+1;
	int32 currPos = numItems/2;
	int32 lb = 0;
	int32 ub = numItems-1;
	while( lb<=ub ) {
		int CompareResult = 0;
		for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
		{
			CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(SortIteration);
			CompareResult = fCompare(item,(CLVListItem*)FullListItemAt((int32)(addr_t)ThisLevelItems.ItemAt(currPos)),fColumnList.IndexOf(Column),Column->Flags());
			if(Column->fSortMode == Descending)
				CompareResult = 0-CompareResult;
		}
		if (CompareResult >= 0) {
			lb = currPos+1;
			currPos = MIN((ub+lb+1) / 2, numItems);
		} else {
			ub = currPos-1;
			currPos = MAX((ub+lb+1) / 2, 0);
		}
	}
	if (currPos == numItems) {
		int32 idx = 1+(int32)(addr_t)ThisLevelItems.ItemAt( numItems-1);
		CLVListItem* item = (CLVListItem*)fFullItemList.ItemAt(idx);
		while( item && item->OutlineLevel() > ThisLevel) {
			item = (CLVListItem*)fFullItemList.ItemAt(++idx);
		}
		return idx;
	}
	else
		return (int32)(addr_t)ThisLevelItems.ItemAt( currPos);
}


bool ColumnListView::AddItem(BListItem* a_item, int32 fullListIndex)
{
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	bool result = AddItemPrivate(item,fullListIndex);
	UpdateDataRect();
	return result;
}


bool ColumnListView::AddItem(BListItem* a_item)
{
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	bool result;
	if(fHierarchical) {
		result = AddItemPrivate(item,fFullItemList.CountItems());
	} else {
		if (fInsertAtSortedPos)
			result = AddItemPrivate(item, DetermineSortedPos( item));
		else 
			result = AddItemPrivate(item, CountItems());
	}
	UpdateDataRect();
	return result;
}


void ColumnListView::ReSortItem( CLVListItem* item)
{
	if (!item)
		return;


	if (fHierarchical) {
		int32 oldPos = FullListIndexOf( item);
		ASSERT(oldPos >= 0);
		int32 newPos;
		int32 itemCount = 1+FullListNumberOfSubitems( item);
		CLVListItem* superitem = Superitem( item);

		// temporarily remove item(s) from full list:
		BList movingItems( itemCount);
		for( int32 i=0; i<itemCount; ++i)
			movingItems.AddItem( fFullItemList.ItemAt( oldPos+i));
		fFullItemList.RemoveItems( oldPos, itemCount);

		// now determine new position of (main-)item:
		int32 startPos = superitem ? FullListIndexOf( superitem)+1 : 0;
		newPos = DetermineSortedPosHierarchical( item, startPos);

		// reinsert item(s) in full list at new position...
		fFullItemList.AddList( &movingItems, newPos);
		if (newPos == oldPos)
			return;			// positions haven't changed, nothing more to do
	
		// Find the new position in listview:
		int32 prevPos = fFullItemList.IndexOf( item)-1;
		CLVListItem* prevItem = NULL;
		while( prevPos >= 0)	{
			prevItem = (CLVListItem*)fFullItemList.ItemAt(prevPos);
			if (prevItem && BListView::HasItem(prevItem))
				break;
			else
				prevPos--;
		}
		if (prevItem)
			newPos = BListView::IndexOf( prevItem) + 1;
		else
			newPos = 0;
	
		// ...and finally move the item(s) in the listview, too:
		for( int32 i=0; i<itemCount; ++i) {
			oldPos = BListView::IndexOf( (BListItem*)movingItems.ItemAt( i));
			if (oldPos < 0)
				continue;
			if (newPos > oldPos)
				MoveItem( oldPos, newPos-1);
			else {
				MoveItem( oldPos, newPos);
				newPos++;
			}
		}
	} else {
		int32 oldPos = IndexOf( item);
		// first check if resorting is required at all...
		CLVListItem *prevItem = FullListItemAt(oldPos-1);
		int ComparePrevResult = prevItem ? CompareItems(prevItem, item) : 0;
		CLVListItem *nextItem = FullListItemAt(oldPos+1);
		int CompareNextResult = nextItem ? CompareItems(item, nextItem) : 0;
		if (ComparePrevResult <= 0 && CompareNextResult <= 0)
			// item still belongs at current position, we we avoid resorting, as
			// that would move this item to the end of a list of items that are 
			// equal (with respect to sorting).
			return;
		// ok, need to resort, so we move item to new position:
		int32 newPos = DetermineSortedPos( item);
		if (newPos > oldPos)
			newPos--;
		MoveItem( oldPos, newPos);
	}
}

int32 ColumnListView::DetermineSortedPos(CLVListItem* item)
{
	int32 SortDepth = fSortKeyList.CountItems();
	int32 numItems = FullListCountItems();
	if (!numItems || !fCompare || !SortDepth)
		return numItems;
	int32 currPos = numItems/2;
	int32 lb = 0;
	int32 ub = numItems-1;
	CLVListItem* compItem;
	int CompareResult;
	while( lb<=ub ) {
		compItem = (CLVListItem*)ItemAt(currPos);
		if (item == compItem) {
			// avoid comparing with ourselves, since that would give unwanted results:
			int32 cp = currPos;
			compItem = NULL;
			if (currPos>lb)
				compItem = (CLVListItem*)ItemAt(--cp);
			if (!compItem && currPos<ub)
				compItem = (CLVListItem*)ItemAt(++cp);
			if (!compItem)
				return currPos;
			currPos = cp;
		}
		CompareResult = CompareItems(item,compItem);
		if (CompareResult >= 0) {
			lb = currPos+1;
			currPos = MIN((ub+lb+1) / 2, numItems);
		} else {
			ub = currPos-1;
			currPos = MAX((ub+lb+1) / 2, 0);
		}
	}
	return currPos;
}

bool ColumnListView::AddItemPrivate(CLVListItem* item, int32 fullListIndex)
{
	AssertWindowLocked();
	item->fOwner = this;
	if(fHierarchical)
	{
		uint32 ItemLevel = item->OutlineLevel();

		//Figure out whether it is visible (should it be added to visible list)
		bool Visible = true;

		//Find the item that contains it in the full list
		int32 SuperItemPos;
		if(ItemLevel == 0)
			SuperItemPos = -1;
		else
			SuperItemPos = fullListIndex - 1;
		CLVListItem* SuperItem;
		while(SuperItemPos >= 0)
		{
			SuperItem = (CLVListItem*)fFullItemList.ItemAt(SuperItemPos);
			if(SuperItem)
			{
				if(SuperItem->OutlineLevel() >= ItemLevel)
					SuperItemPos--;
				else
					break;
			}
			else
				return false;
		}
		if(SuperItemPos >= 0 && SuperItem)
		{
			if(!SuperItem->IsExpanded())
				//SuperItem's contents aren't visible
				Visible = false;
			if(!HasItem(SuperItem))
				//SuperItem itself isn't showing
				Visible = false;
		}

		//Add the item to the full list
		if(!fFullItemList.AddItem(item,fullListIndex))
			return false;
		else
		{
			//Add the item to the display list
			if(Visible)
			{
				//Find the previous item, or -1 if the item I'm adding will be the first one
				int32 PreviousItemPos = fullListIndex - 1;
				CLVListItem* PreviousItem;
				while(PreviousItemPos >= 0)
				{
					PreviousItem = (CLVListItem*)fFullItemList.ItemAt(PreviousItemPos);
					if(PreviousItem && HasItem(PreviousItem))
						break;
					else
						PreviousItemPos--;
				}

				//Add the item after the previous item, or first on the list
				bool Result;
				if(PreviousItemPos >= 0)
					Result = BListView::AddItem((BListItem*)item,IndexOf(PreviousItem)+1);
				else
					Result = BListView::AddItem((BListItem*)item,0);
				if(Result == false) {
					fFullItemList.RemoveItem(item);
					return false;
				}
			}
			return true;
		}
	}
	else {
		return BListView::AddItem(item,fullListIndex);
	}
}


bool ColumnListView::ReplaceItemsWith(BList* newItems, bool sort)
{
	AssertWindowLocked();
	MakeEmptyPrivate();
	bool result = AddListPrivate(newItems, 0);
	if (sort)
		SortItems();
	UpdateDataRect();
	return result;
}


bool ColumnListView::AddList(BList* newItems)
{
	bool result;
	if(fHierarchical)
		result = AddListPrivate(newItems,fFullItemList.CountItems());
	else
		result = AddListPrivate(newItems,CountItems());
	UpdateDataRect();
	return result;
}


bool ColumnListView::AddList(BList* newItems, int32 fullListIndex)
{
	bool result = AddListPrivate(newItems,fullListIndex);
	UpdateDataRect();
	return result;
}


bool ColumnListView::AddListPrivate(BList* newItems, int32 fullListIndex)
{
	AssertWindowLocked();
	if (fHierarchical) {
		int32 NumberOfItems = newItems->CountItems();
		for(int32 count = 0; count < NumberOfItems; count++)
			if(!AddItemPrivate((CLVListItem*)newItems->ItemAt(count),fullListIndex+count))
				return false;
	} else {
		int32 NumberOfItems = newItems->CountItems();
		for(int32 count = 0; count < NumberOfItems; count++)
			((CLVListItem*)newItems->ItemAt( count))->fOwner = this;
		fFullItemList.AddList(newItems);
		if (!BListView::AddList( newItems, fullListIndex))
			return false;
	}
	return true;
}


bool ColumnListView::RemoveItem(BListItem* a_item)
{
	AssertWindowLocked();

	bool result;
	//Get the CLVListItems
	CLVListItem* item = cast_as(a_item,CLVListItem);
	if(item == NULL)
		return false;
	if(fHierarchical)
	{
		if(!fFullItemList.HasItem(item))
			return false;
		int32 ItemsToRemove = 1 + FullListNumberOfSubitems(item);
		result = RemoveItems(fFullItemList.IndexOf(item),ItemsToRemove);
	}
	else {
		result = BListView::RemoveItem((BListItem*)item);
		UpdateDataRect();
	}
	return result;
}


BListItem* ColumnListView::RemoveItem(int32 fullListIndex)
{
	AssertWindowLocked();
	BListItem* item = NULL;
	if(fHierarchical)
	{
		CLVListItem* TheItem = (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
		if(TheItem)
		{
			int32 ItemsToRemove = 1 + FullListNumberOfSubitems(TheItem);
			if (RemoveItems(fullListIndex,ItemsToRemove) && !item)
				item = TheItem;
		}
	}
	else {
		item = BListView::RemoveItem(fullListIndex);
		UpdateDataRect();
	}
	return item;
}


bool ColumnListView::RemoveItems(int32 fullListIndex, int32 count)
{
	AssertWindowLocked();
	CLVListItem* TheItem;
	bool result;
	if(fHierarchical)
	{
		uint32 LastSuperItemLevel = UINT32_MAX;
		int32 Counter;
		int32 DisplayItemsToRemove = 0;
		int32 FirstDisplayItemToRemove = -1;
		for(Counter = fullListIndex; Counter < fullListIndex+count; Counter++)
		{
			TheItem = FullListItemAt(Counter);
			if(TheItem->OutlineLevel() < LastSuperItemLevel)
				LastSuperItemLevel = TheItem->OutlineLevel();
			if(BListView::HasItem((BListItem*)TheItem))
			{
				DisplayItemsToRemove++;
				if(FirstDisplayItemToRemove == -1)
					FirstDisplayItemToRemove = BListView::IndexOf(TheItem);
			}
		}
		while(true)
		{
			TheItem = FullListItemAt(Counter);
			if(TheItem && TheItem->OutlineLevel() > LastSuperItemLevel)
			{
				count++;
				Counter++;
				if(BListView::HasItem((BListItem*)TheItem))
				{
					DisplayItemsToRemove++;
					if(FirstDisplayItemToRemove == -1)
						FirstDisplayItemToRemove = BListView::IndexOf((BListItem*)TheItem);
				}
			}
			else
				break;
		}
		while(DisplayItemsToRemove > 0)
		{
			BListView::RemoveItem(FirstDisplayItemToRemove);
			DisplayItemsToRemove--;
		}
		result = fFullItemList.RemoveItems(fullListIndex,count);
	}
	else
		result = BListView::RemoveItems(fullListIndex,count);
	UpdateDataRect();
	return result;
}


CLVListItem* ColumnListView::FullListItemAt(int32 fullListIndex) const
{
	AssertWindowLocked();
	if (Hierarchical())
		return (CLVListItem*)fFullItemList.ItemAt(fullListIndex);
	else
		return (CLVListItem*)ItemAt(fullListIndex);
}


int32 ColumnListView::FullListIndexOf(const CLVListItem* item) const
{
	AssertWindowLocked();
	if (Hierarchical())
		return fFullItemList.IndexOf((CLVListItem*)item);
	else
		return IndexOf((CLVListItem*)item);
}


int32 ColumnListView::FullListIndexOf(BPoint point) const
{
	AssertWindowLocked();
	int32 DisplayListIndex = IndexOf(point);
	CLVListItem* TheItem = (CLVListItem*)ItemAt(DisplayListIndex);
	if(TheItem) {
		if (Hierarchical())
			return FullListIndexOf(TheItem);
		else
			return IndexOf(TheItem);
	} else
		return -1;
}


CLVListItem* ColumnListView::FullListFirstItem() const
{
	AssertWindowLocked();
	if (Hierarchical())
		return (CLVListItem*)fFullItemList.FirstItem();
	else
		return (CLVListItem*)FirstItem();
}


CLVListItem* ColumnListView::FullListLastItem() const
{
	AssertWindowLocked();
	if (Hierarchical())
		return (CLVListItem*)fFullItemList.LastItem();
	else
		return (CLVListItem*)LastItem();
}


bool ColumnListView::FullListHasItem(const CLVListItem* item) const
{
	AssertWindowLocked();
	if (Hierarchical())
		return fFullItemList.HasItem((CLVListItem*)item);
	else
		return HasItem((CLVListItem*)item);
}


int32 ColumnListView::FullListCountItems() const
{
	AssertWindowLocked();
	if (Hierarchical())
		return fFullItemList.CountItems();
	else
		return CountItems();
}


void ColumnListView::MakeEmpty()
{
	AssertWindowLocked();
	MakeEmptyPrivate();
	UpdateDataRect();
}


void ColumnListView::MakeEmptyPrivate()
{
	fFullItemList.MakeEmpty();
	BListView::MakeEmpty();
}


bool ColumnListView::FullListIsEmpty() const
{
	AssertWindowLocked();
	if (Hierarchical())
		return fFullItemList.IsEmpty();
	else
		return IsEmpty();
}


int32 ColumnListView::FullListCurrentSelection(int32 index) const
{
	AssertWindowLocked();
	int32 Selection = CurrentSelection(index);
	CLVListItem* SelectedItem = (CLVListItem*)ItemAt(Selection);
	if (Hierarchical())
		return FullListIndexOf(SelectedItem);
	else
		return IndexOf(SelectedItem);
}


void ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*))
{
	AssertWindowLocked();
	int32 NumberOfItems = fFullItemList.CountItems();
	for (int32 Counter = 0; Counter < NumberOfItems; Counter++)
		if (Hierarchical()) {
			if (func((CLVListItem*)fFullItemList.ItemAt(Counter)) == true)
				return;
		} else {
			if (func((CLVListItem*)ItemAt(Counter)) == true)
				return;
		}
}


void ColumnListView::FullListDoForEach(bool (*func)(CLVListItem*, void*), void* arg2)
{
	AssertWindowLocked();
	int32 NumberOfItems = fFullItemList.CountItems();
	for (int32 Counter = 0; Counter < NumberOfItems; Counter++)
		if (Hierarchical()) {
			if (func((CLVListItem*)fFullItemList.ItemAt(Counter),arg2) == true)
				return;
		} else {
			if (func((CLVListItem*)ItemAt(Counter),arg2) == true)
				return;
		}
}


CLVListItem* ColumnListView::Superitem(const CLVListItem* item) const
{
	AssertWindowLocked();
	int32 SuperItemPos;
	uint32 ItemLevel = item->OutlineLevel();
	if(ItemLevel == 0)
		SuperItemPos = -1;
	else
		SuperItemPos = fFullItemList.IndexOf((CLVListItem*)item) - 1;
	CLVListItem* SuperItem;
	while(SuperItemPos >= 0)
	{
		SuperItem = (CLVListItem*)fFullItemList.ItemAt(SuperItemPos);
		if(SuperItem)
		{
			if(SuperItem->OutlineLevel() >= ItemLevel)
				SuperItemPos--;
			else
				break;
		}
		else
			return NULL;
	}
	if(SuperItemPos >= 0)
		return SuperItem;
	else
		return NULL;
}


int32 ColumnListView::FullListNumberOfSubitems(const CLVListItem* item) const
{
	AssertWindowLocked();
	if(!fHierarchical)
		return 0;
	int32 ItemPos = FullListIndexOf(item);
	int32 SubItemPos;
	uint32 SuperItemLevel = item->OutlineLevel();
	if(ItemPos >= 0)
	{
		for(SubItemPos = ItemPos + 1; SubItemPos >= 1; SubItemPos++)
		{
			CLVListItem* TheItem = FullListItemAt(SubItemPos);
			if(TheItem == NULL || TheItem->OutlineLevel() <= SuperItemLevel)
				break;
		}
	}
	else
		return 0;
	return SubItemPos-ItemPos-1;
}


void ColumnListView::Expand(CLVListItem* item)
{
	AssertWindowLocked();
	if(!(item->IsSuperItem()))
		item->SetSuperItem( true);
	if(item->IsExpanded())
		return;
	item->SetExpanded(true);
	if(!fHierarchical)
		return;

	int32 DisplayIndex = IndexOf(item);
	if(DisplayIndex >= 0)
	{
		if(fExpanderColumn >= 0)
		{
			//Change the state of the arrow
			item->DrawItem(this,ItemFrame( IndexOf( item)),true);
		}

		//Add the items under it
		int32 FullListIndex = fFullItemList.IndexOf(item);
		uint32 ItemLevel = item->OutlineLevel();
		int32 Counter = FullListIndex + 1;
		int32 AddPos = DisplayIndex + 1;
		while(true)
		{
			CLVListItem* NextItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
			if(NextItem == NULL)
				break;
			if(NextItem->OutlineLevel() > ItemLevel)
			{
				BListView::AddItem((BListItem*)NextItem,AddPos++);
				if(NextItem->IsSuperItem() && !NextItem->IsExpanded())
				{
					//The item I just added is collapsed, so skip all its children
					uint32 SkipLevel = NextItem->OutlineLevel() + 1;
					while(true)
					{
						Counter++;
						NextItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
						if(NextItem == NULL)
							break;
						if(NextItem->OutlineLevel() < SkipLevel)
							break;
					}
				}
				else
					Counter++;
			}
			else
				break;
		}
	}
	UpdateDataRect();
	ExpansionChanged( item, true);
							// [zooey]: give subclasses a chance to act upon change
}


void ColumnListView::Collapse(CLVListItem* item)
{
	AssertWindowLocked();
	if(!(item->IsSuperItem()))
		item->SetSuperItem( true);
	if(!(item->IsExpanded()))
		return;
	item->SetExpanded(false);
	if(!fHierarchical)
		return;

	int32 DisplayIndex = IndexOf((BListItem*)item);
	if(DisplayIndex >= 0)
	{
		if(fExpanderColumn >= 0)
		{
			//Change the state of the arrow
			item->DrawItem(this,ItemFrame( IndexOf( item)),true);
		}

		//Remove the items under it
		uint32 ItemLevel = item->OutlineLevel();
		int32 NextItemIndex = DisplayIndex+1;
		while(true)
		{
			CLVListItem* NextItem = (CLVListItem*)ItemAt(NextItemIndex);
			if(NextItem)
			{
				if(NextItem->OutlineLevel() > ItemLevel)
					BListView::RemoveItem(NextItemIndex);
				else
					break;
			}
			else
				break;
		}
	}
	UpdateDataRect();
	ExpansionChanged( item, false);
							// [zooey]: give subclasses a chance to act upon change
}


bool ColumnListView::IsExpanded(int32 fullListIndex) const
{
	AssertWindowLocked();
	BListItem* TheItem = (BListItem*)fFullItemList.ItemAt(fullListIndex);
	if(TheItem)
		return TheItem->IsExpanded();
	else
		return false;
}


void ColumnListView::SetSortFunction(CLVCompareFuncPtr compare)
{
	AssertWindowLocked();
	fCompare = compare;
}


int ColumnListView::CompareItems(const CLVListItem *item1, const CLVListItem *item2)
{
	int32 SortDepth = fSortKeyList.CountItems();
	int CompareResult = 0;
	for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
	{
		CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(SortIteration);
		CompareResult = fCompare(item1, item2, fColumnList.IndexOf(Column), Column->Flags());
		if(Column->fSortMode == Descending)
			CompareResult = 0-CompareResult;
	}
	return CompareResult;
}
	

void ColumnListView::SortItems()
{
	AssertWindowLocked();
	
	BListItem* selectedItem = NULL;
	if (ListType() == B_SINGLE_SELECTION_LIST) {
		int32 selPos = CurrentSelection( 0);
		if (selPos>=0)
			selectedItem = ItemAt( selPos);
	}

	int32 NumberOfItems;
	if(!fHierarchical)
		NumberOfItems = CountItems();
	else
		NumberOfItems = fFullItemList.CountItems();
	if(NumberOfItems == 0)
		return;
	if(!fHierarchical)
	{
		//Plain sort
		//Do the actual sort
		BListView::SortItems((int (*)(const void*, const void*))ColumnListView::PlainBListSortFunc);
	}
	else
	{
		//Block-by-block sort
		BList NewList;
		SortFullListSegment(0,0,&NewList);
		fFullItemList = NewList;
		//Do the actual sort
		BListView::SortItems((int (*)(const void*, const void*))ColumnListView::HierarchicalBListSortFunc);
	}
	if (selectedItem)
		Select( IndexOf( selectedItem));
}


int ColumnListView::PlainBListSortFunc(BListItem** a_item1, BListItem** a_item2)
{
	CLVListItem* item1 = (CLVListItem*)*a_item1;
	CLVListItem* item2 = (CLVListItem*)*a_item2;
	ColumnListView* SortingContext = item1->fOwner;
	int32 SortDepth = SortingContext->fSortKeyList.CountItems();
	int CompareResult = 0;
	if(SortingContext->fCompare)
		for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
		{
			CLVColumn* Column = (CLVColumn*)SortingContext->fSortKeyList.ItemAt(SortIteration);
			CompareResult = SortingContext->fCompare(item1,item2,SortingContext->fColumnList.IndexOf(Column),Column->Flags());
			if(Column->fSortMode == Descending)
				CompareResult = 0-CompareResult;
		}
	return CompareResult;
}


int ColumnListView::HierarchicalBListSortFunc(BListItem** a_item1, BListItem** a_item2)
{
	CLVListItem* item1 = (CLVListItem*)*a_item1;
	CLVListItem* item2 = (CLVListItem*)*a_item2;
	if(item1->fOwner->fFullItemList.IndexOf(item1) < item1->fOwner->fFullItemList.IndexOf(item2))
		return -1;
	else
		return 1;
}


void ColumnListView::SortFullListSegment(int32 OriginalListStartIndex, int32 InsertionPoint,
	BList* NewList)
{
	//Identify and sort the items at this level
	BList* ItemsInThisLevel = SortItemsInThisLevel(OriginalListStartIndex);
	int32 NewItemsStopIndex = InsertionPoint + ItemsInThisLevel->CountItems();
	NewList->AddList(ItemsInThisLevel,InsertionPoint);
	delete ItemsInThisLevel;

	//Identify and sort the subitems
	for(int32 Counter = InsertionPoint; Counter < NewItemsStopIndex; Counter++)
	{
		CLVListItem* ThisItem = (CLVListItem*)NewList->ItemAt(Counter);
		CLVListItem* NextItem = (CLVListItem*)fFullItemList.ItemAt(fFullItemList.IndexOf(ThisItem)+1);
		if(ThisItem->IsSuperItem() && NextItem && ThisItem->OutlineLevel() < NextItem->OutlineLevel())
		{
			int32 OldListSize = NewList->CountItems();
			SortFullListSegment(fFullItemList.IndexOf(ThisItem)+1,Counter+1,NewList);
			int32 NewListSize = NewList->CountItems();
			NewItemsStopIndex += NewListSize - OldListSize;
			Counter += NewListSize - OldListSize;
		}
	}
}


BList* ColumnListView::SortItemsInThisLevel(int32 OriginalListStartIndex)
{
	uint32 ThisLevel = ((CLVListItem*)fFullItemList.ItemAt(OriginalListStartIndex))->OutlineLevel();

	//Create a new BList of the items in this level
	int32 Counter = OriginalListStartIndex;
	int32 ItemsInThisLevel = 0;
	BList* ThisLevelItems = new BList(16);
	while(true)
	{
		CLVListItem* ThisItem = (CLVListItem*)fFullItemList.ItemAt(Counter);
		if(ThisItem == NULL)
			break;
		uint32 ThisItemLevel = ThisItem->OutlineLevel();
		if(ThisItemLevel == ThisLevel)
		{
			ThisLevelItems->AddItem(ThisItem);
			ItemsInThisLevel++;
		}
		else if(ThisItemLevel < ThisLevel)
			break;
		Counter++;
	}

	//Sort the BList of the items in this level
	CLVListItem** SortArray = new CLVListItem*[ItemsInThisLevel];
	CLVListItem** ListItems = (CLVListItem**)ThisLevelItems->Items();
	for(Counter = 0; Counter < ItemsInThisLevel; Counter++)
		SortArray[Counter] = ListItems[Counter];
	SortListArray(SortArray,ItemsInThisLevel);
	for(Counter = 0; Counter < ItemsInThisLevel; Counter++)
		ListItems[Counter] = SortArray[Counter];
	delete[] SortArray;
	return ThisLevelItems;
}

void ColumnListView::SortListArray(CLVListItem** SortArray, int32 NumberOfItems)
{
	if(fCompare == NULL)
		//No sorting function
		return;
	int32 SortDepth = fSortKeyList.CountItems();
	for(int32 Counter1 = 0; Counter1 < NumberOfItems-1; Counter1++)
		for(int32 Counter2 = Counter1+1; Counter2 < NumberOfItems; Counter2++)
		{
			int CompareResult = 0;
			for(int32 SortIteration = 0; SortIteration < SortDepth && CompareResult == 0; SortIteration++)
			{
				CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt(SortIteration);
				CompareResult = fCompare(SortArray[Counter1],SortArray[Counter2],fColumnList.IndexOf(Column),Column->Flags());
				if(CompareResult > 0)
					CompareResult = 1;
				else if(CompareResult < 0)
					CompareResult = -1;
				if(Column->fSortMode == Descending)
					CompareResult = 0-CompareResult;
			}
			if(CompareResult == 1)
			{
				CLVListItem* Temp = SortArray[Counter1];
				SortArray[Counter1] = SortArray[Counter2];
				SortArray[Counter2] = Temp;
			}
		}
}


void ColumnListView::SetDisconnectScrollView( bool disconnect) {
	// temporarily disconnects vertical scrollbar from listview in
	// order to speed up addition of a large group of items:
	if (disconnect)
		fDeactivatedVerticalBar = ScrollBar( B_VERTICAL);
	if (!fDeactivatedVerticalBar)
		return;
	if (disconnect) {
		fDeactivatedVerticalBar->SetTarget( (BView*)NULL);
	} else {
		fDeactivatedVerticalBar->SetTarget( this);
		fDeactivatedVerticalBar = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::DrawColumn( BRect updateRect, int32 column_index) {
	int32 index = GetDisplayIndexForColumn( column_index);
	if (index % 2) {
		SetLowColor( fDarkColumnCol);
	} else {
		SetLowColor( fLightColumnCol);
	}
	FillRect( updateRect, B_SOLID_LOW);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::Draw( BRect updateRect) {
	int32 firstItemIndex = IndexOf( updateRect.LeftTop());
	int32 lastItemIndex = IndexOf( updateRect.LeftBottom());
	
	if (firstItemIndex >=0) {
		//Draw the visible items:
		if (lastItemIndex < 0)
			lastItemIndex = CountItems()-1;
		BRect ThisItemRect;
		for( int32 idx=firstItemIndex; idx<=lastItemIndex; ++idx) {
			ThisItemRect = ItemFrame( idx);
			ItemAt( idx)->DrawItem( this, ThisItemRect, false);
		}
		updateRect.top = ThisItemRect.bottom+1;
	}

	if (fStripedBackground) {
		BList* DisplayList = &fColumnDisplayList;
		int32 NumberOfColumns = DisplayList->CountItems();
		CLVColumn* ThisColumn;
		BRect ThisColumnRect = updateRect;
		BRegion ClippingRegion;
		ClippingRegion.Set(updateRect);
	
		//Draw the columns
		for(int32 Counter = 0; Counter < NumberOfColumns; Counter++)
		{
			ThisColumn = (CLVColumn*)DisplayList->ItemAt(Counter);
			if(!ThisColumn->IsShown())
				continue;
			ThisColumnRect.left = ThisColumn->ColumnBegin();
			ThisColumnRect.right = ThisColumn->ColumnEnd();
			if(ClippingRegion.Intersects(ThisColumnRect))
				DrawColumn( ThisColumnRect, fColumnList.IndexOf(ThisColumn));
		}
		// draw space extending last column:
		ThisColumnRect.left = ThisColumnRect.right+1;
		ThisColumnRect.right = updateRect.right;
		if (NumberOfColumns % 2)
			SetLowColor( fDarkColumnCol);
		else
			SetLowColor( fLightColumnCol);
		FillRect( ThisColumnRect, B_SOLID_LOW);
	} else {
		SetLowColor( fLightColumnCol);
		FillRect( updateRect, B_SOLID_LOW);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::KeyDown(const char *bytes, int32 numBytes) 
{ 
	if ( numBytes == 1 ) {
		switch( bytes[0]) {
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW: {
				// allow user to expand/collapse the subitems by pressing
				// a modifier and CursorRight/CursorLeft
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_CONTROL_KEY | B_SHIFT_KEY)) {
					// expand / collapse the superitem
					int32 currIdx = CurrentSelection();
					if (currIdx < 0)
						return;			// no item selected
					CLVListItem *currItem = cast_as( ItemAt( currIdx), CLVListItem);
					if (!currItem || !currItem->IsSuperItem())
						return;
					if (bytes[0] == B_LEFT_ARROW)
						Collapse( currItem);
					else if (bytes[0] == B_RIGHT_ARROW)
						Expand( currItem);
				} else {
					// move horizontal scrollbar:
					float min, max, smallStep, bigStep, value;
					BScrollBar* bar = ScrollBar( B_HORIZONTAL);
					if (!bar) 	return;
					bar->GetRange( &min, &max);
					bar->GetSteps( &smallStep, &bigStep);
					value = bar->Value();
					if (bytes[0] == B_LEFT_ARROW) {
						value = MAX( value-smallStep, min);
					} else if (bytes[0] == B_RIGHT_ARROW) {
						value = MIN( value+smallStep, max);
					}
					bar->SetValue( value);
				}
				break;
			}
			case B_PAGE_DOWN: {
				// avoid flicker when we are already at the bottom:
				BScrollBar* vscroller = ScrollBar( B_VERTICAL);
				if (vscroller) {
					float min, max;
					vscroller->GetRange( &min, &max);
					if (max == vscroller->Value())
						return;
					
				}
				break;
			}
			case B_UP_ARROW:
			case B_DOWN_ARROW: {
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & B_SHIFT_KEY && ListType() == B_MULTIPLE_SELECTION_LIST) {
					int32 currIdx = -1;
					int32 maxSelIdx = -1;
					int32 minSelIdx = -1;
					int32 count;
					for( count=0; (currIdx=CurrentSelection(count))>=0; ++count) {
						if (maxSelIdx < currIdx)
							maxSelIdx = currIdx;
						if (minSelIdx == -1 || minSelIdx > currIdx)
							minSelIdx = currIdx;
					}
					if (count == 1) {
						// we are extending a single selection, so here we decide if we
						// are extending upwards or downwards:
						fExtendingDownwards = (bytes[0] == B_DOWN_ARROW);
					}
					if (bytes[0] == B_DOWN_ARROW) {
						if (fExtendingDownwards) {
							if (maxSelIdx >= 0 && maxSelIdx+1<CountItems()) {
								Select( ++maxSelIdx, true);
								// In order to correctly scroll downwards when the user 
								// extends the selection via cursor-keys, we do the 
								// selection stuff ourselves (since BListView::KeyDown 
								// insists on calling ScrollToSelection(), which
								// always shows the *top* of the selection [but we want
								// to see the bottom!]) and then scroll the listview in
								// such a way that the freshly selected item is at the 
								// bottom of the view:
								BRect frame = ItemFrame( maxSelIdx);
								float newYPos = frame.bottom-Bounds().Height();
								if (newYPos > Bounds().top || frame.top < Bounds().top)
									ScrollTo( BPoint( 0, MAX( 0, newYPos)));
							}
						} else if (minSelIdx >= 0) {
							Deselect( minSelIdx);
							ScrollToSelection();
						}
					} else {
						// (bytes[0] == B_UP_ARROW)
						if (!fExtendingDownwards) {
							if (minSelIdx > 0) {
								Select( --minSelIdx, true);
								ScrollToSelection();
							}
						} else if (maxSelIdx >= 0) {
							Deselect( maxSelIdx);
							if (maxSelIdx > 0) {
								// show the item that has just been deselected:
								BRect frame = ItemFrame( maxSelIdx-1);
								float newYPos = frame.top;
								if (newYPos < Bounds().top)
									ScrollTo( BPoint( 0, MAX( 0, newYPos)));
							}
						}
					}
					return;
				}
				break;
			}
			case B_ESCAPE: {
				DeselectAll();
				break;
			}
			default:
				break;
		}
	}
	BListView::KeyDown( bytes, numBytes);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::ShowColumn( int32 col_index) {
	AssertWindowLocked();
	CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( col_index);
	if (column && !fColumnDisplayList.HasItem( column)) {
		fColumnDisplayList.AddItem( column);
		ColumnsChanged();
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::HideColumn( int32 col_index) {
	AssertWindowLocked();
	if (fColumnDisplayList.CountItems() > 1) {
		// only remove column if there is another left...
		CLVColumn* column = (CLVColumn*)fColumnList.ItemAt( col_index);
		if (column && fColumnDisplayList.HasItem( column)) {
			fColumnDisplayList.RemoveItem( column);
			ColumnsChanged();
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void ColumnListView::MessageReceived(BMessage* msg) {
	switch( msg->what) {
		case 'PSTE': {
			// handle color-drops:
			struct rgb_color *col;
			ssize_t siz;
			const void *data;
			msg->FindData( "RGBColor", B_RGB_COLOR_TYPE, &data, &siz);
			col = (rgb_color*)data;
			BPoint point;
			msg->FindPoint("_drop_point_", &point);
			int num = fColumnDisplayList.CountItems();
			for( int i=0; i<num; i++) {
				CLVColumn* column = (CLVColumn*)fColumnDisplayList.ItemAt(i);
				if (column->ColumnBegin()<point.x && column->ColumnEnd()>=point.x) {
					if (i%2==0)
						fDarkColumnCol = *col;
					else
						fLightColumnCol = *col;
					Invalidate();
				}
			}
			break;
		}
		case B_SELECT_ALL: {
			Select( 0, FullListCountItems()-1);
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t ColumnListView::Archive(BMessage* archive, bool) const {
	AssertWindowLocked();
	status_t ret = B_OK;
	int i;

	// display order of columns:
	const int32 numCols = CountColumns();
#ifdef __POWERPC__
	int32 displayCols[100];
#else
	int32 displayCols[numCols];
#endif
	GetDisplayOrder( displayCols);
	for( i=0; i<numCols && ret==B_OK; ++i) { 
		ret = archive->AddInt32( MSG_DISPLAYORDER, displayCols[i]);
	}

	if (ret == B_OK) {
		// sortkeys and -modes:
		int32 numSortKeys = fSortKeyList.CountItems();
		ret =	archive->AddInt32( MSG_NUMSORTKEYS, numSortKeys);
		for( i = 0; i<numSortKeys && ret==B_OK; ++i) {
			CLVColumn* Column = (CLVColumn*)fSortKeyList.ItemAt( i);
			ret =	archive->AddInt32( MSG_SORTKEY, IndexOfColumn( Column))
				|| archive->AddInt32( MSG_SORTMODE, Column->SortMode());
		}
	}
		
	if (ret == B_OK) {
		// store each column's width:
		for( i=0; i<numCols && ret==B_OK; ++i) { 
			ret = archive->AddFloat( MSG_COLWIDTH, 
											 ((CLVColumn*)fColumnList.ItemAt(i))->Width());
		}
	}

	if (ret == B_OK) {
		// store layout locked state:
		ret = archive->AddBool(MSG_LAYOUTLOCKED, fColumnLabelView->LayoutLocked());
	}

	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t ColumnListView::Unarchive(const BMessage* archive, bool) {
	AssertWindowLocked();
	int i;
	
	if (!archive || archive->IsEmpty()) return B_OK;
	
	// display order of columns:
	const int32 numCols = CountColumns();
#ifdef __POWERPC__
	int32 displayCols[100];
#else
	int32 displayCols[numCols];
#endif
	for( i=0; i<numCols; ++i) { 
		archive->FindInt32( MSG_DISPLAYORDER, i, &displayCols[i]);
	}

	// sortkeys and -modes:
	int32 numSortKeys = 0;
	archive->FindInt32( MSG_NUMSORTKEYS, &numSortKeys);
#ifdef __POWERPC__
	int32 sortKeys[100];
	CLVSortMode sortModes[100];
#else
	int32 sortKeys[numSortKeys];
	CLVSortMode sortModes[numSortKeys];
#endif
	for( i = 0; i<numSortKeys; ++i) {
		archive->FindInt32( MSG_SORTKEY, i, &sortKeys[i]);
		archive->FindInt32( MSG_SORTMODE, i, (int32*)&sortModes[i]);
	}
	SetSorting( numSortKeys, sortKeys, sortModes);

	// set each column's width:
	float width;
	for( i=0; i<numCols; ++i) { 
		archive->FindFloat( MSG_COLWIDTH, i, &width);
		((CLVColumn*)fColumnList.ItemAt(i))->SetWidth( width);
	}
	SetDisplayOrder( displayCols);

	// restore layout locked state:
	bool layoutIsLocked = false;
	archive->FindBool(MSG_LAYOUTLOCKED, &layoutIsLocked);
	fColumnLabelView->SetLayoutLocked(layoutIsLocked);

	return B_OK;
}

void ColumnListView::SetExtendedSelectionPolicy( bool likeTracker) {
	if (likeTracker) {
		fExtendSelMask = B_OPTION_KEY|B_COMMAND_KEY;
		fToggleSelMask = B_SHIFT_KEY;
	} else {
		fExtendSelMask = B_SHIFT_KEY;
		fToggleSelMask = B_OPTION_KEY|B_COMMAND_KEY;
	}
}
