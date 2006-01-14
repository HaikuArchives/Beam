/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <cstdarg>

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
	class BMessage;
	class BMessageRunner;
	class BRect;
	class BStatusBar;
#endif

#include <layout.h>

#include "BmDividable.h"


/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmDividable::DivideSame( MView* div1, ...)
{
	BmDividable* div = dynamic_cast< BmDividable*>( div1);
	if (!div)
		return;
	float maxWidth = div->Divider();
	float w;
	MView* v;
	va_list va;
	va_start( va, div1);
	while( (v = va_arg( va, MView*)) != 0) {
		if ((div = dynamic_cast< BmDividable*>( v)) != 0) {
			w = div->Divider();
			if (w > maxWidth)
				maxWidth = w;
		}
	}
	va_end( va);
	va_start( va, div1);
	div = dynamic_cast< BmDividable*>( div1);
	div->SetDivider( maxWidth);
	while( (v = va_arg( va, MView*)) != 0)
		if ((div = dynamic_cast< BmDividable*>( v)) != 0)
			div->SetDivider( maxWidth);
	va_end( va);
}
