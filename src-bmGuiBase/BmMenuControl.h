/*
	BmMenuControl.h
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


#ifndef _BmMenuControl_h
#define _BmMenuControl_h

#include <MenuField.h>

#include <layout.h>

#include "SantaPartsForBeam.h"
#include "BmDividable.h"

class HGroup;

class IMPEXPSANTAPARTSFORBEAM BmMenuControl : public MView, 
															 public BMenuField,
															 public BmDividable
{
	typedef BMenuField inherited;

public:
	// creator-func, c'tors and d'tor:
	BmMenuControl( const char* label, BMenu* menu, float weight=1.0, 
						float maxWidth=1E5, const char* fitText=NULL);
	~BmMenuControl();
	
	// native methods:
	void MarkItem( const char* label);
	void ClearMark();

	// overrides:
	void SetEnabled( bool enabled);
	void SetDivider( float divider);
	float Divider() const;

private:

	minimax layoutprefs();
	BRect layout(BRect frame);
	
	BMenu* mMenu;
	
	// Hide copy-constructor and assignment:
	BmMenuControl( const BmMenuControl&);
	BmMenuControl operator=( const BmMenuControl&);
};


#endif
