/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMenuControl_h
#define _BmMenuControl_h

#include <MenuField.h>

#include <layout.h>

#include "BmGuiBase.h"
#include "BmDividable.h"

class HGroup;
class BMenuBar;

class IMPEXPBMGUIBASE BmMenuControl : public MView, 
															 public BMenuField,
															 public BmDividable
{
	typedef BMenuField inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMenuControl( const char* label, BMenu* menu, float weight=1.0, 
						float maxWidth=1E5, const char* fitText=NULL);
	~BmMenuControl();
	
	// native methods:
	void MakeEmpty();
	void MarkItem( const char* label);
	void ClearMark();

	// overrides:
	void SetEnabled( bool enabled);
	void SetDivider( float divider);
	float Divider() const;
	void AllAttached();

private:

	minimax layoutprefs();
	BRect layout(BRect frame);
	
	BMenuBar* mMenuBar;
	
	// Hide copy-constructor and assignment:
	BmMenuControl( const BmMenuControl&);
	BmMenuControl operator=( const BmMenuControl&);
};


#endif
