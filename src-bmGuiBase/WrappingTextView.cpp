//		$Id$
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


#include <ScrollBar.h>
#include <StringView.h>

#include "WrappingTextView.h"

WrappingTextView::WrappingTextView(BRect a_frame,const char* a_name,int32 a_resize_mode,int32 a_flags)
: BTextView(a_frame, a_name, BRect(4.0,4.0,a_frame.right-a_frame.left-4.0,a_frame.bottom-a_frame.top-4.0),
	a_resize_mode,a_flags)
{
	m_vertical_offset = 0;
	m_modified = false;
	m_modified_disabled = false;
	ResetTextRect();
}	


WrappingTextView::~WrappingTextView()
{ }


void WrappingTextView::DetachedFromWindow()
{
	//This is sort of what the destructor should do, but...  Derived class's destructors get called before
	//WrappingTextView's destructor so StoreChange won't get to the derived class's version of it.
	if(m_modified)
	{
		StoreChange();
		m_modified = false;
	}
}


void WrappingTextView::FrameResized(float a_width, float a_height)
{
	BTextView::FrameResized(a_width,a_height);
	ResetTextRect();
}


void WrappingTextView::MakeFocus(bool a_focused)
{
	BTextView::MakeFocus(a_focused);
	if(!a_focused && m_modified)
	{
		StoreChange();
		m_modified = false;
	}
}


void WrappingTextView::InsertText(const char *a_text, int32 a_length, int32 a_offset,
	const text_run_array *a_runs)
{
	BTextView::InsertText(a_text, a_length, a_offset, a_runs);
	if(!m_modified_disabled)
		Modified();
}


void WrappingTextView::Modified()
{
	m_modified = true;
}


void WrappingTextView::SetText(const char *text, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,length,runs);
	m_modified_disabled = false;
}


void WrappingTextView::SetText(const char *text, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,runs);
	m_modified_disabled = false;
}


void WrappingTextView::SetText(BFile *file, int32 offset, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(file,offset,length,runs);
	m_modified_disabled = false;
}


void WrappingTextView::StoreChange()
{ }


void WrappingTextView::ResetTextRect()
{
	BRect textRect = Bounds();
	textRect.left = 4.0;
	textRect.top = m_vertical_offset + 4.0;
	textRect.right -= 4.0;
	textRect.bottom -= 4.0;
	SetTextRect(textRect);
	// we have to readjust the scrollbar-proportion, since
	// the BTextView doesn't do it correctly when we have
	// fooled in a vertical_offset:
	float small, big;
	BScrollBar* bar = ScrollBar( B_VERTICAL);
	if (!bar) 	return;
	bar->GetSteps( &small, &big);
	float height = TextHeight( 0, TextLength());
	if (height+m_vertical_offset)
		bar->SetProportion( MIN( 1.0, big/(height+m_vertical_offset)));
	else 
		bar->SetProportion( 1.0);

	bar = ScrollBar( B_HORIZONTAL);
	if (!bar) 	return;
	int numChildren = CountChildren();
	float width = textRect.Width();
	float maxWidth = width;
	for( int i=0; i<numChildren; ++i) {
		float w = ChildAt( i)->Frame().Width();
		if (w > maxWidth)
			maxWidth = w;
	}
	bar->SetSteps( width/10, width);
	bar->SetRange( 0, MAX(0, maxWidth-width-20));
	bar->SetProportion( MIN( 1.0, width/maxWidth));
}


bool WrappingTextView::HasBeenModified()
{
	return m_modified;
}

void WrappingTextView::CalculateVerticalOffset() {
	float offset = 0;
	int max=CountChildren();
	for( int i=0; i<max; ++i) {
		BView* child = ChildAt(i);
		child->MoveTo( 0, offset);
		BRect frame = child->Frame();
		offset += 1+frame.Height();
	}
	m_vertical_offset = MAX( offset, 0);
	ResetTextRect();
}

void WrappingTextView::KeyDown(const char *bytes, int32 numBytes) 
{ 
	if (IsEditable()) {
		// in editable mode, we simply forward cursor-keys:
		BTextView::KeyDown( bytes, numBytes);
	} else if ( numBytes == 1 ) {
		// in read-only mode, we use cursor-keys to move scrollbar:
		switch( bytes[0]) {
			case B_PAGE_UP:
			case B_PAGE_DOWN:
			case B_UP_ARROW:
			case B_DOWN_ARROW: {
					// move vertical scrollbar:
					float min, max, smallStep, bigStep, value;
					BScrollBar* bar = ScrollBar( B_VERTICAL);
					if (!bar) 	return;
					bar->GetRange( &min, &max);
					bar->GetSteps( &smallStep, &bigStep);
					value = bar->Value();
					if (bytes[0] == B_UP_ARROW) {
						value = MAX( value-smallStep, min);
					} else if (bytes[0] == B_DOWN_ARROW) {
						value = MIN( value+smallStep, max);
					} else if (bytes[0] == B_PAGE_UP) {
						value = MAX( value-bigStep, min);
					} else if (bytes[0] == B_PAGE_DOWN) {
						value = MIN( value+bigStep, max);
					}
					bar->SetValue( value);
				}
				break;
			default:
				BTextView::KeyDown( bytes, numBytes);
				break;
		}
	}
}

