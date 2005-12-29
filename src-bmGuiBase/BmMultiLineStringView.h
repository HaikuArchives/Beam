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
public:
	BmMultiLineStringView( BRect frame, const char* name, const char *text, 
								  uint32 resizingMode = B_FOLLOW_LEFT|B_FOLLOW_TOP,
								  uint32 flags = B_WILL_DRAW);
	~BmMultiLineStringView();
	
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

private:
	void _InitText();
	void _InitFont();

	BmString mText;
	BPoint mDrawPT;
	float mLineHeight;
	bool mIsSelectable;

	struct TextLine {
		TextLine(const char* t, int32 l)
			: text(t), len(l)					{}
		const char* text;
		int32 len;
	};
	BList mTextLines;

	// Hide copy-constructor and assignment:
	BmMultiLineStringView( const BmMultiLineStringView&);
	BmMultiLineStringView operator=( const BmMultiLineStringView&);
};


#endif
