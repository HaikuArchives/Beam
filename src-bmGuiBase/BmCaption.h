/*
	BmCaption.h
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


#ifndef _BmCaption_h
#define _BmCaption_h

#include <StringView.h>

#include "BmGuiBase.h"

class IMPEXPBMGUIBASE BmCaption : public BStringView
{
	typedef BStringView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmCaption( BRect frame, const char* text);
	~BmCaption();

	// overrides of BStringView base:
	void Draw( BRect bounds);

private:
	// Hide copy-constructor and assignment:
	BmCaption( const BmCaption&);
	BmCaption operator=( const BmCaption&);

};


#endif
