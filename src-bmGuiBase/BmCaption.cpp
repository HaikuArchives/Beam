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
	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor( B_UI_PANEL_BACKGROUND_COLOR);
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
void BmCaption::Draw( BRect updateRect) {
	BRect r = Bounds();
	SetLowColor( ui_color( B_UI_PANEL_BACKGROUND_COLOR));
	FillRect( BRect(1.0,1.0,r.right-1,r.bottom-1), B_SOLID_LOW);
	SetHighColor( ui_color( B_UI_SHINE_COLOR));
	StrokeLine( BPoint(0.0,1.0), BPoint(r.right-1,1.0));
	StrokeLine( BPoint(0.0,1.0), r.LeftBottom());
	SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
	StrokeLine( r.LeftTop(), r.RightTop());
	StrokeLine( r.RightTop(), r.RightBottom());
	StrokeLine( r.LeftBottom(), r.RightBottom());
	SetHighColor( ui_color( B_UI_PANEL_TEXT_COLOR));
	font_height fInfo;
	be_plain_font->GetHeight( &fInfo);
	float offset = (1+r.Height()-(fInfo.ascent+fInfo.descent))/2.0;
	float width = be_plain_font->StringWidth( Text());
	BPoint pos( r.Width()-width-2.0, fInfo.ascent+offset);
	DrawString( Text(), pos);
}
