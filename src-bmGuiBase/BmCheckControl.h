/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmCheckControl_h
#define _BmCheckControl_h

#include <MCheckBox.h>

#include "BmGuiBase.h"

class HGroup;

class IMPEXPBMGUIBASE BmCheckControl : public MCheckBox
{
	typedef MCheckBox inherited;

public:
	// creator-func, c'tors and d'tor:
	BmCheckControl( const char* label, ulong id=0, bool state=false);
	BmCheckControl( const char* label, BMessage* msg, BHandler* target=NULL, bool state=false);
	~BmCheckControl();
	
	// native methods:
	float LabelWidth();
	void AdjustToMaxLabelWidth( float maxWidth);
	void SetValueSilently( bool val);

private:
	// Hide copy-constructor and assignment:
	BmCheckControl( const BmCheckControl&);
	BmCheckControl operator=( const BmCheckControl&);
};


#endif
