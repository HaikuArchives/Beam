/*
	BmCaption.cpp
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


#ifdef __POWERPC__
#define BM_BUILDING_SANTAPARTSFORBEAM 1
#endif

#include "Colors.h"

#include "BmCaption.h"

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::BmCaption( BRect frame, const char* text) 
	:	inherited( frame, NULL, text)
{
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmCaption::~BmCaption() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmCaption::Draw( BRect) {
	BRect rect = Bounds();
	SetHighColor( tint_color(BeBackgroundGrey, 1.072F));
	FillRect( rect);
	SetHighColor( White);
	StrokeRect( BRect(1.0,1.0,rect.right,rect.bottom));
	SetHighColor( BeShadow);
	StrokeRect( rect);
	SetLowColor( BeLightShadow);
	SetHighColor( Black);
	font_height fInfo;
	be_plain_font->GetHeight( &fInfo);
	float offset = (1+rect.Height()-(fInfo.ascent+fInfo.descent))/2.0;
	float width = be_plain_font->StringWidth( Text());
	BPoint pos( rect.Width()-width-2.0, fInfo.ascent+offset);
	DrawString( Text(), pos);
}
