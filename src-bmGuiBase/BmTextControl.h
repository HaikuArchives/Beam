/*
	BmTextControl.h
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


#ifndef _BmTextControl_h
#define _BmTextControl_h

#include <MenuField.h>
#include <TextControl.h>

#include <layout.h>

#include "SantaPartsForBeam.h"
#include "BmDividable.h"

class HGroup;

#define BM_TEXTFIELD_MODIFIED 'bmfm'

class BmMenuControllerBase;

class IMPEXPBMGUIBASE BmTextControl : public MView, 
															 public BTextControl,
															 public BmDividable
{
	typedef BTextControl inherited;

public:
	// creator-func, c'tors and d'tor:
	BmTextControl( const char* label, bool labelIsMenu=false,
						int32 fixedTextLen=0, int32 minTextLen=0);
	BmTextControl( const char* label, BmMenuControllerBase* menu, 
						int32 fixedTextLen=0, int32 minTextLen=0);
	~BmTextControl();
	
	// native methods:
	void InitSize( const char* label, int32 fixedTextLen, int32 minTextLen,
						BmMenuControllerBase* popup);
	void SetTextSilently( const char* text);

	// overrides of BTextControl:
	void FrameResized( float new_width, float new_height);
	void SetDivider( float divider);
	float Divider() const;
	void SetEnabled( bool enabled);
	void SetText( const char* text);

	// getters:
	inline BTextView* TextView() const 	{ return mTextView; }
	inline BMenuField* MenuField() const
													{ return mMenuField; }
	BmMenuControllerBase* Menu() const;

private:
	minimax layoutprefs();
	BRect layout(BRect frame);

	bool mLabelIsMenu;
	BTextView* mTextView;
	BMenuField* mMenuField;

	// Hide copy-constructor and assignment:
	BmTextControl( const BmTextControl&);
	BmTextControl operator=( const BmTextControl&);
};


#endif
