/*
	BmMultiLineTextControl.h
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


#ifndef _BmMultiLineTextControl_h
#define _BmMultiLineTextControl_h

#include <MenuField.h>

#include <layout.h>

#include "SantaPartsForBeam.h"
#include "MultiLineTextControl.h"

class HGroup;

#define BM_MULTILINE_TEXTFIELD_MODIFIED 'bmfn'

class IMPEXPSANTAPARTSFORBEAM BmMultiLineTextControl : public MView, public MultiLineTextControl
{
	typedef MultiLineTextControl inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMultiLineTextControl( const char* label, bool labelIsMenu=false,
									int32 lineCount = 4, int32 minTextLen=0, 
									bool fixedHeight=false);
	~BmMultiLineTextControl();
	
	// native methods:
	void SetTextSilently( const char* text);

	// overrides of MultiLineMultiLineTextControl:
	void FrameResized( float new_width, float new_height);
	void SetDivider( float divider);
	void SetEnabled( bool enabled);
	void SetText( const char* text);

	// getters:
	inline BTextView* TextView() const 	{ return mTextView; }
	inline BMenuField* MenuField() const	{ return mMenuField; }
	inline BMenu* Menu() const 			{ return mMenuField ? mMenuField->Menu() : NULL; }

private:
	minimax layoutprefs();
	BRect layout(BRect frame);

	bool mLabelIsMenu;
	BTextView* mTextView;
	BMenuField* mMenuField;

	// Hide copy-constructor and assignment:
	BmMultiLineTextControl( const BmMultiLineTextControl&);
	BmMultiLineTextControl operator=( const BmMultiLineTextControl&);
};


#endif
