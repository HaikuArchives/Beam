/*
	BmCheckControl.cpp
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

