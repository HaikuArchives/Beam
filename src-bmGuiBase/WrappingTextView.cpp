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


#include <Looper.h>
#include <Message.h>
#include <ScrollBar.h>
#include <StringView.h>
#include <Window.h>

#include "BmBasics.h"
#include "Colors.h"
#include "WrappingTextView.h"

WrappingTextView::UndoInfo::UndoInfo()
	:	text_runs( NULL)
{ 
}

WrappingTextView::UndoInfo::UndoInfo( bool ins, const char* t, int32 len, uint32 offs,
			 			  const text_run_array* runs, bool delToRight)
	:	isInsertion( ins)
	,	text( t, len)
	,	offset( offs)
	,	text_runs( NULL)
	,	fixed( false)
	,	deleteToRight( delToRight)
{ 
	if (runs) {
		int32 sz = sizeof(int32)+runs->count*sizeof(text_run);
		text_runs = (text_run_array*)malloc( sz);
		memcpy( text_runs, runs, sz);
	}
}

WrappingTextView::UndoInfo::UndoInfo( const UndoInfo& ui)
	:	isInsertion( ui.isInsertion)
	,	text( ui.text)
	,	offset( ui.offset)
	,	text_runs( NULL)
	,	fixed( ui.fixed)
	,	deleteToRight( ui.deleteToRight)
{ 
	if (ui.text_runs) {
		int32 sz = sizeof(int32)+ui.text_runs->count*sizeof(text_run);
		text_runs = (text_run_array*)malloc( sz);
		memcpy( text_runs, ui.text_runs, sz);
	}
}

WrappingTextView::UndoInfo WrappingTextView::UndoInfo::operator=( const UndoInfo& ui) {
	isInsertion = ui.isInsertion;
	text = ui.text;
	offset = ui.offset;
	fixed = ui.fixed;
	deleteToRight = ui.deleteToRight;
	if (ui.text_runs) {
		int32 sz = sizeof(int32)+ui.text_runs->count*sizeof(text_run);
		text_runs = (text_run_array*)malloc( sz);
		memcpy( text_runs, ui.text_runs, sz);
	} else
		text_runs = NULL;
	return *this;
}

WrappingTextView::UndoInfo::~UndoInfo() { 
	if (text_runs)
		free( text_runs); 
}

void WrappingTextView::UndoInfo::JoinTextRuns( const text_run_array* a_runs, int32 len, bool atFront) {
	if (a_runs) {
		if (text_runs) {
			// need to join text-runs:
			int32 old_sz = a_runs->count*sizeof(text_run);
			int32 add_sz = text_runs->count*sizeof(text_run);
			text_run_array* new_runs = (text_run_array*)malloc( sizeof(int32) + old_sz + add_sz);
			new_runs->count = a_runs->count + text_runs->count;
			const text_run_array* front;
			const text_run_array* back;
			int32 offs;
			if (atFront) {
				offs = len;
				front = a_runs;
				back = text_runs;
				offset -= len;
			} else {
				offs = text.Length();
				front = text_runs;
				back = a_runs;
			}
			int32 idx = 0;
			for( int i=0; i<front->count; ++i) {
				new_runs->runs[idx].font = front->runs[i].font;
				new_runs->runs[idx].color = front->runs[i].color;
				new_runs->runs[idx].offset = front->runs[i].offset;
				idx++;
			}
			for( int i=0; i<back->count; ++i) {
				new_runs->runs[idx].font = back->runs[i].font;
				new_runs->runs[idx].color = back->runs[i].color;
				new_runs->runs[idx].offset = back->runs[i].offset + offs;
				idx++;
			}
			free( text_runs);
			text_runs = new_runs;
		} else {
			// copy given textrun:
			int32 sz = sizeof(int32)+a_runs->count*sizeof(text_run);
			text_runs = (text_run_array*)malloc( sz);
			memcpy( text_runs, a_runs, sz);
		}
	}
}



WrappingTextView::WrappingTextView(BRect a_frame,const char* a_name,int32 a_resize_mode,int32 a_flags)
: BTextView(a_frame, a_name, BRect(4.0f,4.0f,a_frame.right-a_frame.left-4.0f,a_frame.bottom-a_frame.top-4.0f),
	a_resize_mode,a_flags)
{
	m_vertical_offset = 0;
	m_modified = false;
	m_modified_disabled = false;
	m_fixed_width = 0;
	m_modification_msg = NULL;
	m_curr_undo_index = 0;
	m_max_undo_index = 0;
	m_in_undo_redo = false;
	m_undo_context = NULL;
	m_last_key_was_del = false;
	m_separator_chars = " ";			// just a default, will be overridden by Beam-Prefs
	ResetTextRect();

	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);														
}	


WrappingTextView::~WrappingTextView()
{ 
	delete m_modification_msg;
}


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
	if (IsEditable()) {
		if (!m_in_undo_redo) {
			uint32 sz = m_undo_vect.size();
			if (m_curr_undo_index >= sz)
				m_undo_vect.resize( MAX( 25, sz*2));
			bool canConcat = false;
			UndoInfo* lastAction = (m_curr_undo_index ? &m_undo_vect[m_curr_undo_index-1] : NULL);
			if (lastAction && lastAction->isInsertion && !lastAction->fixed
			&& lastAction->offset+lastAction->text.Length() == a_offset)
				canConcat = true;
			if (canConcat) {
				lastAction->text << a_text;
				lastAction->JoinTextRuns( a_runs, a_length, false);
				if (strlen( a_text)==1 && m_separator_chars.FindFirst( a_text)!=B_ERROR)
					lastAction->fixed = true;
			} else
				m_undo_vect[m_curr_undo_index++] = UndoInfo( true, a_text, a_length, a_offset, a_runs);
			m_max_undo_index = MAX( m_max_undo_index, m_curr_undo_index);
		}
		if(!m_modified_disabled)
			Modified();
	}
}

void WrappingTextView::DeleteText(int32 start, int32 finish)
{
	if (IsEditable()) {
		if (!m_in_undo_redo) {
			bool deleteToRight = m_last_key_was_del;
			uint32 sz = m_undo_vect.size();
			if (m_curr_undo_index >= sz)
				m_undo_vect.resize( MAX( 25, sz*2));
			text_run_array* runs = RunArray( start, finish);
			bool canConcat = false;
			UndoInfo* lastAction = (m_curr_undo_index ? &m_undo_vect[m_curr_undo_index-1] : NULL);
			if (lastAction && !lastAction->isInsertion && !lastAction->fixed
			&& lastAction->deleteToRight==deleteToRight 
			&& (lastAction->offset == start || lastAction->offset == finish))
				canConcat = true;
			int32 len = finish-start;
			BmString a_text( Text()+start, len);
			if (canConcat) {
				bool atFront = lastAction->offset==finish;
				if (atFront)
					lastAction->text.Prepend( a_text);
				else
					lastAction->text << a_text;
				lastAction->JoinTextRuns( runs, len, atFront);
				if (len==1 && m_separator_chars.FindFirst( a_text)!=B_ERROR)
					lastAction->fixed = true;
			} else
				m_undo_vect[m_curr_undo_index++] = UndoInfo( false, Text()+start, finish-start, start, runs, deleteToRight);
			if (runs)
				free( runs);
			m_max_undo_index = MAX( m_max_undo_index, m_curr_undo_index);
		}
		if(!m_modified_disabled)
			Modified();
	}
	BTextView::DeleteText( start, finish);
}

void WrappingTextView::ResetUndoBuffer() {
	// throw away undo-buffer if context changes:
	m_undo_vect.clear();
	m_curr_undo_index = 0;
	m_max_undo_index = 0;
}

void WrappingTextView::UndoChange() {
	if (!m_curr_undo_index)
		return;
	UndoInfo info = m_undo_vect[--m_curr_undo_index];
	m_in_undo_redo = true;
	int32 len = info.text.Length();
	if (info.isInsertion) {
		BTextView::Delete( info.offset, info.offset+len);
		Select( info.offset, info.offset);
	} else {
		BTextView::Insert( info.offset, info.text.String(), info.text.Length(), info.text_runs);
		if (info.deleteToRight)
			Select( info.offset, info.offset);
		else
			Select( info.offset+len, info.offset+len);
	}
	ScrollToSelection();
	m_in_undo_redo = false;
	if(!m_modified_disabled)
		Modified();
}

void WrappingTextView::RedoChange() {
	if (m_curr_undo_index >= m_max_undo_index)
		return;
	UndoInfo info = m_undo_vect[m_curr_undo_index++];
	m_in_undo_redo = true;
	if (info.isInsertion) {
		BTextView::Insert( info.offset, info.text.String(), info.text.Length(), info.text_runs);
		Select( info.offset+info.text.Length(), info.offset+info.text.Length());
	} else {
		BTextView::Delete( info.offset, info.offset+info.text.Length());
		Select( info.offset, info.offset);
	}
	ScrollToSelection();
	m_in_undo_redo = false;
	if(!m_modified_disabled)
		Modified();
}

void WrappingTextView::MessageReceived( BMessage* msg)
{
	switch( msg->what) {
		case B_UNDO: {
			UndoChange();
			break;
		}
		case B_REDO: {
			RedoChange();
			break;
		}
		case B_COPY: {
			int32 from, to;
			GetSelection( &from, &to);
			if (from != to)
				inherited::MessageReceived( msg);
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

void WrappingTextView::Modified()
{
	m_modified = true;
	if (m_modification_msg && Looper())
		Looper()->PostMessage( m_modification_msg);
}

void WrappingTextView::SetModificationMessage( BMessage* msg)
{
	delete m_modification_msg;
	m_modification_msg = msg;
}


void WrappingTextView::SetText(const char *text, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,length,runs);
	ResetUndoBuffer();
	m_modified_disabled = false;
}


void WrappingTextView::SetText(const char *text, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(text,runs);
	ResetUndoBuffer();
	m_modified_disabled = false;
}


void WrappingTextView::SetText(BFile *file, int32 offset, int32 length, const text_run_array *runs)
{
	m_modified_disabled = true;
	BTextView::SetText(file,offset,length,runs);
	ResetUndoBuffer();
	m_modified_disabled = false;
}

void WrappingTextView::StoreChange()
{ }


void WrappingTextView::ResetTextRect()
{
	BRect bounds = Bounds();
	BRect textRect = bounds;
	textRect.left = 4.0;
	textRect.top = m_vertical_offset + 4.0f;
	if (m_fixed_width)
		textRect.right = 8.0f + StringWidth( "0") * float(m_fixed_width);
	else
		textRect.right -= 4.0f;
	textRect.bottom -= 4.0f;
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
		bar->SetProportion( MIN( 1.0f, big/(height+m_vertical_offset)));
	else 
		bar->SetProportion( 1.0f);

	if (BeamOnDano) {
		// circumvent a special feature/bug in Zeta with respect to 
		// the scrollbar not always calling ValueChanged() on a click.
		// Seemingly, changing the value twice hides the bug:
		bar->SetValue(1);
		bar->SetValue(0);
	}

	bar = ScrollBar( B_HORIZONTAL);
	if (!bar) 	return;
	float width = bounds.Width();
	float maxWidth = MAX( width, textRect.right);
	int numChildren = IsEditable() ? 0 : CountChildren();
	for( int i=0; i<numChildren; ++i) {
		float w = ChildAt( i)->Frame().Width();
		if (w > maxWidth)
			maxWidth = w;
	}
	bar->SetSteps( width/10, width);
	bar->SetRange( 0, MAX(0, maxWidth-width));
	bar->SetProportion( MIN( 1.0f, width/maxWidth));
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
		offset += frame.Height();
	}
	m_vertical_offset = MAX( offset, 0);
	ResetTextRect();
}

void WrappingTextView::KeyDown(const char *bytes, int32 numBytes) 
{ 
	if (IsEditable() && numBytes==1) {
		m_last_key_was_del = (bytes[0]==B_DELETE);
		switch( bytes[0]) {
			case B_RIGHT_ARROW: {
				// implement word-wise movement:
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					int32 len=TextLength();
					int32 startPos, endPos;
					GetSelection( &startPos, &endPos);
					if (endPos==len)
						break;
					if (startPos==endPos && (mods & B_SHIFT_KEY))
						m_selection_start=B_RIGHT_ARROW;
					int32 wordStart, wordEnd;
					if (mods & B_SHIFT_KEY && m_selection_start==B_LEFT_ARROW) {
						do {
							FindWord( startPos, &wordStart, &wordEnd);
							if (wordEnd > wordStart+1)
								break;
							if (wordEnd == wordStart+1 && ByteAt( wordStart)!=' ')
								break;
						} while( ++startPos < len);
						Select( MIN(endPos, wordEnd), endPos);
					} else {
						do {
							FindWord( endPos, &wordStart, &wordEnd);
							if (wordEnd > wordStart+1)
								break;
							if (wordEnd == wordStart+1 && ByteAt( wordStart)!=' ')
								break;
						} while( ++endPos < len);
						if (mods & B_SHIFT_KEY) {
							Select( startPos, wordEnd);
						} else
							Select( wordEnd, wordEnd);
					}
					ScrollToSelection();
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			case B_LEFT_ARROW: {
				// implement word-wise movement:
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					int32 startPos, endPos;
					GetSelection( &startPos, &endPos);
					if (!startPos)
						break;
					if (startPos==endPos && (mods & B_SHIFT_KEY))
						m_selection_start=B_LEFT_ARROW;
					int32 wordStart, wordEnd;
					if (mods & B_SHIFT_KEY && m_selection_start==B_RIGHT_ARROW) {
						--endPos;
						do {
							FindWord( endPos, &wordStart, &wordEnd);
							if (wordEnd > wordStart+1)
								break;
							if (wordEnd == wordStart+1 && ByteAt( wordStart)!=' ')
								break;
						} while( --endPos > 0);
						Select( startPos, MAX( startPos, wordStart));
					} else {
						--startPos;
						do {
							FindWord( startPos, &wordStart, &wordEnd);
							if (wordEnd > wordStart+1)
								break;
							if (wordEnd == wordStart+1 && ByteAt( wordStart)!=' ')
								break;
						} while( --startPos > 0);
						if (mods & B_SHIFT_KEY)
							Select( wordStart, endPos);
						else
							Select( wordStart, wordStart);
					}
					ScrollToSelection();
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
	} else if ( numBytes == 1 ) {
		// in read-only mode, we use cursor-keys to move scrollbar, and
		// we remap HOME / END to the vertical scrollbar (not the horizontal,
		// which is default).
		switch( bytes[0]) {
			case B_PAGE_UP:
			case B_PAGE_DOWN:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_HOME: 
			case B_END: {
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
				} else if (bytes[0] == B_HOME) {
					value = min;
				} else if (bytes[0] == B_END) {
					value = max;
				}
				bar->SetValue( value);
				break;
			}
			default:
				BTextView::KeyDown( bytes, numBytes);
				break;
		}
	} else
		inherited::KeyDown( bytes, numBytes);
}

void WrappingTextView::SetFixedWidth( int32 i) { 
	m_fixed_width = i; 
	ResetTextRect();
}
