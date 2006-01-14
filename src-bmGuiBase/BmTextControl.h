/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmTextControl_h
#define _BmTextControl_h

#include <MenuField.h>
#include <TextControl.h>

#include <layout.h>

#include "BmGuiBase.h"
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
