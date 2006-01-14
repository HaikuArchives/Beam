/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BMessageRunner;
	class BRect;
	class BStatusBar;
#endif

#include <HGroup.h>

#include "BmCheckControl.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCheckControl::BmCheckControl( const char* label, ulong id, bool state) 
	:	inherited( label, id, state)
{
	ResizeToPreferred();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCheckControl::BmCheckControl( const char* label, BMessage* msg, 
										  BHandler* target, bool state)
	:	inherited( label, msg, target, state)
{
	ResizeToPreferred();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCheckControl::~BmCheckControl() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
float BmCheckControl::LabelWidth() {
	const char* label = Label();
	if (!label)
		return 0;
	BFont font;
	GetFont( &font);
	return font.StringWidth( label);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCheckControl::AdjustToMaxLabelWidth( float maxWidth) {
	ct_mpm.maxi.x = ct_mpm.mini.x = mpm.maxi.x = mpm.mini.x = maxWidth;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCheckControl::SetValueSilently( bool val) {
	BMessenger msnger = Messenger();
	SetTarget( NULL);
	SetValue( val);
	SetTarget( msnger);
}

