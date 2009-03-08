/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Stephan Aßmus, <superstippi@gmx.de>
 */

#ifndef _BetterButton_h
#define _BetterButton_h

#include <MButton.h>

#include "BmGuiBase.h"

class IMPEXPBMGUIBASE BetterButton : public MButton
{
	typedef MButton inherited;

public:
	// creator-func, c'tors and d'tor:
	BetterButton( const char* label, ulong id = 0,
		minimax size = minimax(-1, -1 , 1E6, 1E6, 1));
	BetterButton( const char* label, BMessage* message, BHandler* handler = NULL,
		minimax size = minimax(-1, -1 , 1E6, 1E6, 1));
	virtual ~BetterButton();

	virtual void Draw( BRect updateRect);
};


#endif
