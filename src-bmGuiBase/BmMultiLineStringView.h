/*
	BmMultiLineStringView.h
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


#ifndef _BmMultiLineStringView_h
#define _BmMultiLineStringView_h

#include "BmGuiBase.h"

#include <List.h>
#include <View.h>

#include "BmString.h"

class IMPEXPBMGUIBASE BmMultiLineStringView : public BView
{
	typedef BView inherited;
	struct TextLine;
public:
	BmMultiLineStringView( BRect frame, const char* name, const char *text, 
								  uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,
								  uint32 flags = B_WILL_DRAW);
	virtual ~BmMultiLineStringView();
	
	// native methods:
	void MakeSelectable(bool s) 			{ mIsSelectable = s; }
	bool IsSelectable() const				{ return mIsSelectable; }

	void SetText(const char* text);
	const BmString& Text() const			{ return mText; }

	float TextHeight();
	float LineHeight();

	// overrides of BView:
	void Draw( BRect updateRect);
	void SetFont(const BFont *font, uint32 properties = B_FONT_ALL);
	//
	void MouseDown(BPoint point);
	void MouseMoved(BPoint point, uint32 transit, const BMessage *message);
	//
	void MessageReceived( BMessage* msg);
	//
	void MakeFocus(bool focused = true);
	//
	void Select(int32 from, int32 to);
	void GetSelection(int32* from, int32* to);
	
private:
	void _InitText();
	void _InitFont();
	int32 _ConvertPointToIndex(const BPoint& point, bool closestCaretPos = true,
										TextLine** textLinePtr = NULL);
	void _InvalidateRange(int32 from, int32 to);
	void _DrawString(const char* text, int32 len);

	BmString mText;
	float mLineHeight;
	float mAscent;

	bool mIsSelectable;
	int32 mSelectionStart;
	int32 mSelectionEnd;

	BList mTextLines;

	static const float nLeftOffset;

	// Hide copy-constructor and assignment:
	BmMultiLineStringView( const BmMultiLineStringView&);
	BmMultiLineStringView operator=( const BmMultiLineStringView&);
};


#endif
