/*
	BmDividable.cpp
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

#include <cstdarg>

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
