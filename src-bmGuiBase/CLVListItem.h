//		$Id$
//CLVListItem header file

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


#ifndef _CLV_LIST_ITEM_H_
#define _CLV_LIST_ITEM_H_


//******************************************************************************************************
//**** SYSTEM HEADER FILES
//******************************************************************************************************
#include <ListItem.h>


//******************************************************************************************************
//**** PROJECT HEADER FILES AND CLASS NAME DECLARATIONS
//******************************************************************************************************
class ColumnListView;
class CLVColumn;
class BList;

#include "BmGuiBase.h"

struct CLVDrawingContext
{
	virtual ~CLVDrawingContext() {}
};

//******************************************************************************************************
//**** CLVListItem CLASS DECLARATION
//******************************************************************************************************
class IMPEXPBMGUIBASE CLVListItem : public BListItem
{
		static const uint8 CLV_STYLE_SUPERITEM = 1<<0;
	public:
		//Constructor and destructor
		CLVListItem(uint32 level = 0, bool superitem = false, bool expanded = false);
		virtual ~CLVListItem();

		//Archival stuff
		/* Not implemented yet
		CLVItem(BMessage* archive);
		static CLVItem* Instantiate(BMessage* data);
		virtual	status_t Archive(BMessage* data, bool deep = true) const;
		*/
		
		virtual void DrawItem(BView*, BRect itemRect, bool complete);
		virtual void DrawColumn(BRect item_column_rect, int32 column_index, 
										CLVDrawingContext* ctx) = 0;
								//column_index (0-N) is based on the order in which the columns were added
								//to the ColumnListView, not the display order.  An index of -1 indicates
								//that the program needs to draw a blank area beyond the last column.  The
								//main purpose is to allow the highlighting bar to continue all the way to
								//the end of the ColumnListView, even after the end of the last column.

		virtual CLVDrawingContext* CreateDrawingContext();
		virtual void SetupDrawingContext(CLVDrawingContext* drawingContext);

		BRect ItemColumnFrame(int32 column_index);
		float ExpanderShift(int32 column_index);
		virtual void Update(BView* owner, const BFont* font);
		inline bool IsSuperItem() const	{ return fItemFlags & CLV_STYLE_SUPERITEM;}
		inline void SetSuperItem( bool superitem) 
													{ SetStyleFlag( CLV_STYLE_SUPERITEM, 
																		 superitem); 
													}
		bool ExpanderRectContains(BPoint where);
		virtual void ColumnWidthChanged(int32 column_index, float column_width);
		virtual void FrameChanged(int32 column_index, BRect new_frame);
		void InvalidateColumn( int32 column_index);
		CLVColumn* ColumnAt( int32 column_index);

	protected:
		friend class ColumnListView;
		void SetStyleFlag( uint8 style, bool on);

		ColumnListView* fOwner;
		uint8 fItemFlags;
};


#endif
