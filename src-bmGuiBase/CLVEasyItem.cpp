//		$Id$
//CLVListItem source file

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
#include <string.h>
#include <Region.h>
#include <ClassInfo.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES
//******************************************************************************************************
#include "CLVEasyItem.h"
#include "Colors.h"
#include "CLVColumn.h"
#include "ColumnListView.h"
#include "NewStrings.h"


//******************************************************************************************************
//**** CLVEasyItem CLASS DEFINITION
//******************************************************************************************************
CLVEasyItem::CLVEasyItem(uint32 level, bool superitem, bool expanded, float minheight)
: CLVListItem(level,superitem,expanded,minheight)
{
	text_offset = 0.0;
	style_flags = 0;
}


CLVEasyItem::~CLVEasyItem()
{
	int num_columns = m_column_types.CountItems();
	for(int column = 0; column < num_columns; column++)
	{
		int32 type = (int32)m_column_types.ItemAt(column);
		type &= CLVColTypesMask;
		if(type == 	CLVColStaticText)
			delete[] ((char*)m_column_content.ItemAt(column));
	}
}


void CLVEasyItem::PrepListsForSet(int column_index)
{
	int cur_num_columns = m_column_types.CountItems();
	bool delete_old = (cur_num_columns >= column_index-1);
	while(cur_num_columns <= column_index)
	{
		m_column_types.AddItem((void*)CLVColNone);
		m_column_content.AddItem(NULL);
		cur_num_columns++;
	}
	if(delete_old)
	{
		//Column content exists already so delete the old entries
		int32 old_type = (int32)m_column_types.ItemAt(column_index);
		old_type &= CLVColTypesMask;

		void* old_content = m_column_content.ItemAt(column_index);
		if(old_type == CLVColStaticText)
			delete[] ((char*)old_content);
	}
}


void CLVEasyItem::SetColumnContent(int column_index, const char *text, bool right_justify)
{
	PrepListsForSet(column_index);

	//Create the new entry
	if(text == NULL || text[0] == 0)
	{
		((int32*)m_column_types.Items())[column_index] = CLVColNone;
		((char**)m_column_content.Items())[column_index] = NULL;
	}
	else
	{
		((char**)m_column_content.Items())[column_index] = Strdup_new(text);
		((int32*)m_column_types.Items())[column_index] = CLVColStaticText;
		if(right_justify)
			((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
	}
}


void CLVEasyItem::SetColumnContent(int column_index, const BBitmap *bitmap, int8 horizontal_offset,
											  bool right_justify)
{
	PrepListsForSet(column_index);

	//Create the new entry
	if(bitmap == NULL)
	{
		((int32*)m_column_types.Items())[column_index] = CLVColNone;
		((char**)m_column_content.Items())[column_index] = NULL;
	}
	else
	{
		((int32*)m_column_types.Items())[column_index] = CLVColBitmap + (((int32)horizontal_offset)<<24);
		if(right_justify)
			((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
		((BBitmap**)m_column_content.Items())[column_index] = (BBitmap*)bitmap;
	}
}


void CLVEasyItem::SetColumnUserTextContent(int column_index, bool right_justify)
{
	PrepListsForSet(column_index);
	((int32*)m_column_types.Items())[column_index] = CLVColUserText;
	if(right_justify)
		((int32*)m_column_types.Items())[column_index] |= CLVColFlagRightJustify;
}


const char* CLVEasyItem::GetColumnContentText(int column_index)
{
	int32 type = ((int32)m_column_types.ItemAt(column_index)) & CLVColTypesMask;
	if(type == CLVColStaticText)
		return (char*)m_column_content.ItemAt(column_index);
	if(type == CLVColUserText)
		return GetUserText(column_index,-1);
	return NULL;
}


const BBitmap* CLVEasyItem::GetColumnContentBitmap(int column_index)
{
	int32 type = ((int32)m_column_types.ItemAt(column_index)) & CLVColTypesMask;
	if(type != CLVColBitmap)
		return NULL;
	return (BBitmap*)m_column_content.ItemAt(column_index);
}


void CLVEasyItem::DrawItemColumn(BView *owner, BRect item_column_rect, int32 column_index, bool)
{
	rgb_color color, tinted_color;
	bool selected = IsSelected();
	bool striped = ((ColumnListView*)owner)->StripedBackground();
	float offs = striped ? 5.0 : 0.0;

	if(selected) {
		color = ((ColumnListView*)owner)->ItemSelectColor();
		tinted_color = ((ColumnListView*)owner)->ItemSelectColorTinted();
	} else {
		color = ((ColumnListView*)owner)->LightColumnCol();
		tinted_color = ((ColumnListView*)owner)->DarkColumnCol();
	}
	if (Highlight()) {
		const float highlight_tint = 1.15F;
		color = tint_color( color, highlight_tint);
		tinted_color = tint_color( tinted_color, highlight_tint);
	}
	owner->SetDrawingMode(B_OP_COPY);

	int32 index = ((ColumnListView*)owner)->GetDisplayIndexForColumn( abs(column_index));
	if (column_index < 0)
		index = abs(column_index);
	if (striped && index % 2) {
		owner->SetHighColor( tinted_color);
		owner->SetLowColor( tinted_color);
	} else {
		owner->SetHighColor( color);
		owner->SetLowColor( color);
	}
	owner->FillRect( item_column_rect);
	
	if(column_index < 0)
		return;

	int32 type = ((int32)m_column_types.ItemAt(column_index));
	if(type == 0)
		return;
	bool right_justify = false;
	if(type & CLVColFlagRightJustify)
		right_justify = true;
	type &= CLVColTypesMask;

	BRegion Region;
	Region.Include(item_column_rect);
	owner->ConstrainClippingRegion(&Region);

	if(type == CLVColStaticText || type == CLVColUserText)
	{
		const char* text = NULL;

		BFont owner_font;
		owner->GetFont(&owner_font);
		if (Bold()) {
			owner->SetFont( be_bold_font);
		}
	
		owner->SetHighColor( Black);
		if(type == CLVColStaticText)
			text = (const char*)m_column_content.ItemAt(column_index);
		else if(type == CLVColUserText)
			text = GetUserText(column_index,-1);

		if(text != NULL)
		{
			BPoint draw_point;
			if(!right_justify)
				draw_point.Set(item_column_rect.left+(offs?offs:2.0),item_column_rect.top+text_offset);
			else
			{
				float string_width = owner_font.StringWidth(text);
				draw_point.Set(item_column_rect.right-(offs?offs:2.0)-string_width,item_column_rect.top+text_offset);
			}				
			owner->DrawString(text,draw_point);
		}

		if (Bold()) {
			owner->SetFont( &owner_font);
		}
	}
	else if(type == CLVColBitmap)
	{
		const BBitmap* bitmap = (BBitmap*)m_column_content.ItemAt(column_index);
		BRect bounds = bitmap->Bounds();
		float horizontal_offset = (float)(((int32)m_column_types.ItemAt(column_index))>>24);
		if(!right_justify)
		{
			item_column_rect.left += horizontal_offset;
			item_column_rect.right = item_column_rect.left + (bounds.right-bounds.left);
		}
		else
		{
			item_column_rect.left = item_column_rect.right-horizontal_offset-(bounds.right-bounds.left);
			item_column_rect.right -= horizontal_offset;
		}
		item_column_rect.top += ceil(((item_column_rect.bottom-item_column_rect.top)-(bounds.bottom-bounds.top))/2.0);
		item_column_rect.bottom = item_column_rect.top + (bounds.bottom-bounds.top);
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(bitmap,item_column_rect);
		owner->SetDrawingMode(B_OP_COPY);

	}
	owner->ConstrainClippingRegion(NULL);
}


void CLVEasyItem::Update(BView *owner, const BFont *font)
{
	CLVListItem::Update(owner,font);
	font_height FontAttributes;
	BFont owner_font;
	owner->GetFont(&owner_font);
	owner_font.GetHeight(&FontAttributes);
	float FontHeight = ceil(FontAttributes.ascent) + ceil(FontAttributes.descent);
	text_offset = ceil(FontAttributes.ascent) + (Height()-FontHeight)/2.0;
}


int CLVEasyItem::CompareItems(const CLVListItem *a_Item1, const CLVListItem *a_Item2, int32 KeyColumn,
										int32 col_flags)
{
	const CLVEasyItem* Item1 = cast_as(a_Item1,const CLVEasyItem);
	const CLVEasyItem* Item2 = cast_as(a_Item2,const CLVEasyItem);
	if(Item1 == NULL || Item2 == NULL || Item1->m_column_types.CountItems() <= KeyColumn ||
		Item2->m_column_types.CountItems() <= KeyColumn)
		return 0;
	
	int32 type1 = ((int32)Item1->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;
	int32 type2 = ((int32)Item2->m_column_types.ItemAt(KeyColumn)) & CLVColTypesMask;

	uint32 datatype = col_flags & CLV_COLDATAMASK;
	
	if (datatype == CLV_COLDATA_NUMBER) {
		int32 num1 = Item1->GetNumValueForColumn( KeyColumn);
		int32 num2 = Item2->GetNumValueForColumn( KeyColumn);

		return num1<num2 ? -1 : (num1>num2 ? 1 : 0);
	} else if (datatype == CLV_COLDATA_DATE) {
		time_t date1 = Item1->GetDateValueForColumn( KeyColumn);
		time_t date2 = Item2->GetDateValueForColumn( KeyColumn);

		return date1<date2 ? -1 : (date1>date2 ? 1 : 0);
	} else {
		const char* text1 = NULL;
		const char* text2 = NULL;

		if(type1 == CLVColStaticText)
			text1 = (const char*)Item1->m_column_content.ItemAt(KeyColumn);
		else if(type1 == CLVColUserText)
			text1 = Item1->GetUserText(KeyColumn,-1);

		if(type2 == CLVColStaticText)
			text2 = (const char*)Item2->m_column_content.ItemAt(KeyColumn);
		else if(type2 == CLVColUserText)
			text2 = Item2->GetUserText(KeyColumn,-1);

		if (!text1)	return -1;
		if (!text2)	return 1;
		return strcasecmp(text1,text2);
	}
}

const char* CLVEasyItem::GetUserText(int32, float) const
{
	return NULL;
}

void CLVEasyItem::Highlight( bool b) { 
	SetStyleFlag( CLV_STYLE_HIGHLIGHT, b); 
}

bool CLVEasyItem::Highlight( ) { 
	return (style_flags & CLV_STYLE_HIGHLIGHT) != 0; 
}

void CLVEasyItem::Bold( bool b) { 
	SetStyleFlag( CLV_STYLE_BOLD, b); 
}

bool CLVEasyItem::Bold( ) { 
	return (style_flags & CLV_STYLE_BOLD) != 0; 
}

void CLVEasyItem::SetStyleFlag( uint8 style, bool on) {
	if (on)
		style_flags |= style;
	else
		style_flags &= (0xFF ^ style);
}
