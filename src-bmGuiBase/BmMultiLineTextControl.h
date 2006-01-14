/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMultiLineTextControl_h
#define _BmMultiLineTextControl_h

#include <MenuField.h>

#include <layout.h>

#include "BmGuiBase.h"
#include "MultiLineTextControl.h"
#include "BmDividable.h"

class HGroup;

#define BM_MULTILINE_TEXTFIELD_MODIFIED 'bmfn'

class IMPEXPBMGUIBASE BmMultiLineTextControl 
	: public MView,
	  public MultiLineTextControl,
	  public BmDividable
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
	float Divider() const;
	void SetEnabled( bool enabled);
	void SetText( const char* text);
	minimax layoutprefs();
	BRect layout(BRect frame);

	// getters:
	inline BMenuField* MenuField() const	{ return mMenuField; }
	inline BMenu* Menu() const 			{ return mMenuField ? mMenuField->Menu() : NULL; }

private:
	bool mLabelIsMenu;
	BMenuField* mMenuField;

	// Hide copy-constructor and assignment:
	BmMultiLineTextControl( const BmMultiLineTextControl&);
	BmMultiLineTextControl operator=( const BmMultiLineTextControl&);
};


#endif
