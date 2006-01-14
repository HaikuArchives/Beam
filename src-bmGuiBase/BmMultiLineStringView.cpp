/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <cctype>

#include <Clipboard.h>
#include <Debug.h>
#include <Looper.h>
#include <Message.h>

#include "BmMultiLineStringView.h"

struct BmMultiLineStringView::TextLine 
{
	TextLine(int32 so, int32 l)
		:	startOffs(so)						
		,  len(l)								{}
	int32 startOffs;
	int32 len;
};

const float BmMultiLineStringView::nLeftOffset = 3.0;

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineStringView::BmMultiLineStringView( BRect frame, const char *name,
															 const char* text, 
															 uint32 resizingMode,
															 uint32 flags)
	:	inherited(frame, name, resizingMode, flags)
	,	mText(text)
	,	mIsSelectable(true)
	,	mSelectionStart(-1)
	,	mSelectionEnd(-1)
{
	_InitFont();
	_InitText();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMultiLineStringView::~BmMultiLineStringView()
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::SetText(const char* text)
{
	if (!Looper() || LockLooper()) {
		mText.SetTo(text);
		_InitText();
		if (Looper())
			UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::SetFont(const BFont *font, uint32 properties)
{
	inherited::SetFont(font, properties);
	if (!Looper() || LockLooper()) {
		_InitFont();
		if (Looper())
			UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::Select(int32 from, int32 to)
{
	if (!Looper() || LockLooper()) {
		int32 oldStart = mSelectionStart;
		int32 oldEnd = mSelectionEnd;
		mSelectionStart = from;
		mSelectionEnd = to;
		if (oldStart != mSelectionStart)
			_InvalidateRange(oldStart, mSelectionStart);
		if (oldEnd != mSelectionEnd)
			_InvalidateRange(oldEnd, mSelectionEnd);
		if (Looper())
			UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::GetSelection(int32* from, int32* to)
{
	if (!Looper() || LockLooper()) {
		if (from)
			*from = mSelectionStart;
		if (to)
			*to = mSelectionEnd;
		if (Looper())
			UnlockLooper();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMultiLineStringView::LineHeight()
{
	return mLineHeight;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmMultiLineStringView::TextHeight()
{
	float textHeight = 0.0;
	if (!Looper() || LockLooper()) {
		textHeight = mLineHeight * mTextLines.CountItems();
		if (Looper())
			UnlockLooper();
	}
	return textHeight;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::Draw(BRect updateRect)
{
	inherited::Draw(updateRect);
	BPoint pt(nLeftOffset, mAscent+1.0);
	rgb_color frontCol = HighColor();
	rgb_color backCol = LowColor();
	int32 selStart = MIN(mSelectionStart, mSelectionEnd);
	int32 selEnd = MAX(mSelectionStart, mSelectionEnd);
	BRect b(Bounds());
	for(int32 i=0; i<mTextLines.CountItems(); ++i) {
		MovePenTo(pt);
		TextLine* textLine = static_cast<TextLine*>(mTextLines.ItemAt(i));
		int32 lineStart = textLine->startOffs;
		int32 lineEnd = textLine->startOffs + textLine->len;
		const char* text = mText.String()+textLine->startOffs;
		int32 len = textLine->len;
		if (selStart != selEnd && selStart < lineEnd && selEnd > lineStart 
		&& IsFocus()) {
			// this line is at least partly selected, we need to highlight:
			int32 highlightStart = MAX(selStart, lineStart);
			int32 highlightEnd = MIN(selEnd, lineEnd);
			// we need to highlight the space to the right of a line's text if 
			// the selection includes the newline character:
			bool highlightNewline 
				= (highlightEnd && mText.ByteAt(highlightEnd-1) == '\n');
			if (highlightStart > lineStart) {
				// part before selection:
				len = highlightStart - lineStart;
				DrawString(text, len);
				text += len;
			}
			// selected part:
			BPoint pen = PenLocation();
			len = highlightEnd - highlightStart;
			BRect selRect( pen.x, pen.y - mAscent, 
								highlightNewline 
									? b.right
									: pen.x + StringWidth(text, len) - 1,
								pen.y - mAscent + mLineHeight - 1);
			SetHighColor(backCol);
			SetLowColor(frontCol);
			FillRect(selRect, B_SOLID_LOW);
			_DrawString(text, len);
			SetHighColor(frontCol);
			SetLowColor(backCol);
			text += len;
			len = lineEnd - highlightEnd;
			if (highlightEnd < lineEnd) {
				// part after selection:
				_DrawString(text, len);
			}
		} else {
			// simple case, no intersection with selection:
			if (len && text[len-1] == '\n')
				len--;
			_DrawString(text, len);
		}
		pt.y += mLineHeight;
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::MouseDown(BPoint point)
{
	inherited::MouseDown(point);
#ifdef BEAM_FOR_ZETA
	// on ZETA, views are automatically scrolled such that as much of their
	// area as possible is visible. We avoid this:
	MakeFocusNoScroll(true);
#else
	MakeFocus(true);
#endif
	BMessage* msg = Looper()->CurrentMessage();
	if (msg->FindInt32("buttons") == B_PRIMARY_MOUSE_BUTTON) {
		if (msg->FindInt32("clicks") == 2) {
			// double click, select word:
			TextLine* textLine = NULL;
			int32 index = _ConvertPointToIndex(point, false, &textLine);
			if (textLine) {
				const char* text = mText.String()+textLine->startOffs;
				int32 wordStart = index - textLine->startOffs;
				if (!isspace(text[wordStart]) && !ispunct(text[wordStart])
				&& isgraph(text[wordStart])) {
					while(wordStart > 0 && !isspace(text[wordStart-1]) 
					&& !ispunct(text[wordStart-1]) && isgraph(text[wordStart-1]))
						wordStart--;
				}
				int32 wordEnd = 1 + index - textLine->startOffs;
				if (!isspace(text[wordEnd-1]) && !ispunct(text[wordEnd-1])
				&& isgraph(text[wordEnd-1])) {
					while(wordEnd < textLine->len && !isspace(text[wordEnd])
					&& !ispunct(text[wordEnd]) && isgraph(text[wordEnd]))
						wordEnd++;
				}
				Select(textLine->startOffs+wordStart, textLine->startOffs+wordEnd);
			}
		} else if (msg->FindInt32("clicks") == 3) {
			// triple-click, select line:
			TextLine* textLine = NULL;
			_ConvertPointToIndex(point, false, &textLine);
			if (textLine)
				Select(textLine->startOffs, textLine->startOffs+textLine->len);
		} else {
			// single click, reset selection:
			int32 index = _ConvertPointToIndex(point);
			Select(index, index);
			SetMouseEventMask(B_POINTER_EVENTS, B_NO_POINTER_HISTORY);
		}
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::MouseMoved(BPoint point, uint32 transit, 
													const BMessage *message)
{
	inherited::MouseMoved(point, transit, message);
	BMessage* msg = Looper()->CurrentMessage();
	if (msg->FindInt32("buttons") == B_PRIMARY_MOUSE_BUTTON
	&& mSelectionStart != -1) {
		TextLine* textLine = NULL;
		int32 selEnd = _ConvertPointToIndex(point, true, &textLine);
		if (selEnd && textLine) {
			// if the selection ends with a newline character, we explicitly
			// exclude this character, so that the user needs to select one
			// more char after each newline to get the newline included into
			// the selection. This mimics standard BeOS behaviour:
			int32 lineEnd = textLine->startOffs + textLine->len;
			if (selEnd == lineEnd && mText.ByteAt(lineEnd-1) == '\n')
				selEnd--;
		}
		Select(mSelectionStart, selEnd);
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::MessageReceived( BMessage* msg)
{
	switch( msg->what) {
		case B_COPY: {
			if (mSelectionStart == mSelectionEnd)
				break;
			int32 selStart = MIN(mSelectionStart, mSelectionEnd);
			int32 selEnd = MAX(mSelectionStart, mSelectionEnd);
			BmString clip;
			mText.CopyInto(clip, selStart, selEnd-selStart);
			BMessage* clipMsg;
			if (be_clipboard->Lock()) {
				be_clipboard->Clear();
				if ((clipMsg = be_clipboard->Data())!=NULL) {
					clipMsg->AddData( "text/plain", B_MIME_TYPE, 
											clip.String(), clip.Length());
					be_clipboard->Commit();
				}
				be_clipboard->Unlock();
			}
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::MakeFocus(bool focused)
{
	inherited::MakeFocus(focused);
	if (mSelectionStart != mSelectionEnd)
		Invalidate();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_InitFont()
{
	if (Looper() && !Looper()->IsLocked())
		DEBUGGER(("The looper must be locked!"));

	struct font_height fh;
	GetFontHeight(&fh);
	mLineHeight = ceil(fh.ascent+fh.descent+fh.leading);
	mAscent = ceil(fh.ascent);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_InitText()
{
	if (Looper() && !Looper()->IsLocked())
		DEBUGGER(("The looper must be locked!"));

	int32 count = mTextLines.CountItems();
	while(count)
		delete static_cast<TextLine*>(mTextLines.RemoveItem(--count));

	// split the buffer into separate text lines, putting the resulting 
	// start-offsets and lengths into mTextLines:
	int32 lastPos = 0;
	int32 pos;
	while( (pos = mText.FindFirst('\n', lastPos)) >= B_OK) {
		mTextLines.AddItem(new TextLine(lastPos, pos-lastPos+1));
		lastPos = pos + 1;
	}
	mTextLines.AddItem(new TextLine(lastPos, mText.Length()-lastPos));
}	

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
int32 BmMultiLineStringView::_ConvertPointToIndex(const BPoint& point,
																  bool closestCaretPos,
																  TextLine** textLinePtr)
{
	if (Looper() && !Looper()->IsLocked())
		DEBUGGER(("The looper must be locked!"));

	int32 lineNo = int(point.y / mLineHeight);
	if (point.y < 0 || lineNo < 0)
		return 0;
	if (lineNo >= mTextLines.CountItems())
		return mText.Length();

	TextLine* textLine = static_cast<TextLine*>(mTextLines.ItemAt(lineNo));
	if (textLinePtr)
		*textLinePtr = textLine;
	if (!textLine)
		return 0;
	int32 len = textLine->len;
	float currWidth = 0;
	const char* text = mText.String()+textLine->startOffs;
	while(len) {
		currWidth = StringWidth(text, len);
		if (nLeftOffset + currWidth <= point.x)
			break;
		len--;
	}
	if (!len)
		currWidth = 0;
	// len now points to the index before the character we clicked on,
	// we need to find out if this character should be included or not:
	if (closestCaretPos && len < textLine->len) {
		// determine smaller distance:
		float nextWidth = StringWidth(text, len+1);
		float distToCurr = point.x - nLeftOffset - currWidth;
		float distToNext = nLeftOffset + nextWidth - point.x;
		if (distToNext <= distToCurr)
			len++;
	} else if (!closestCaretPos && len == textLine->len) {
		if (len && text[len-1] == '\n')
			len--;
	}
	return textLine->startOffs + len;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_InvalidateRange(int32 from, int32 to)
{
	if (Looper() && !Looper()->IsLocked())
		DEBUGGER(("The looper must be locked!"));

	if (from > to) {
		int32 t = from;
		from = to;
		to = t;
	}
	float x = nLeftOffset;
	float y = 0;
	BRect b(Bounds());
	for(int32 i=0; i < mTextLines.CountItems(); ++i) {
		TextLine* textLine = static_cast<TextLine*>(mTextLines.ItemAt(i));
		int32 lineStart = textLine->startOffs;
		int32 lineEnd = textLine->startOffs + textLine->len;
		const char* text = mText.String()+textLine->startOffs;
		if (from < lineEnd && to > lineStart) {
			int32 invalStart = MAX(from-lineStart, 0);
			int32 invalEnd = MIN(to-lineStart, textLine->len);
			// we need to invalidate the space to the right of a line's text if 
			// the selection includes the newline character:
			bool invalidateNewline 
				= (invalEnd && text[invalEnd-1] == '\n');
			BRect invalRect( x + StringWidth(text, invalStart), 
								  y, 
	  							  invalidateNewline 
										? b.right
										: x + StringWidth(text, invalEnd) - 1, 
								  y + mLineHeight);
			Invalidate(invalRect);
		}
		y += mLineHeight;
	}
}

/*------------------------------------------------------------------------------*\
	_DrawString()
		-	draws the given string but makes sure that the 
\*------------------------------------------------------------------------------*/
void BmMultiLineStringView::_DrawString(const char* text, int32 len)
{
	if (len && text[len-1] == '\n')
		len--;
	DrawString(text, len);
}

