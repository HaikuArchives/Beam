/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Stephan Aßmus, <superstippi@gmx.de>
 */

#include "BetterButton.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BetterButton::BetterButton( const char* label, ulong id, minimax size)
	:	inherited( label, id, size)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BetterButton::BetterButton( const char* label, BMessage* message,
		BHandler* handler, minimax size)
	:	inherited( label, message, handler, size)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BetterButton::~BetterButton() {
}

/*------------------------------------------------------------------------------*\
	Draw( )
		-	
\*------------------------------------------------------------------------------*/
void BetterButton::Draw( BRect updateRect)
{
	// Bypass MButton implementation
	BButton::Draw( updateRect);
}

