/*
	BmCheckControl.h
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


#ifndef _BmCheckControl_h
#define _BmCheckControl_h

#include <MCheckBox.h>

#include "BmGuiBase.h"

class HGroup;

class IMPEXPBMGUIBASE BmCheckControl : public MCheckBox
{
	typedef MCheckBox inherited;

public:
	// creator-func, c'tors and d'tor:
	BmCheckControl( const char* label, ulong id=0, bool state=false);
	BmCheckControl( const char* label, BMessage* msg, BHandler* target=NULL, bool state=false);
	~BmCheckControl();
	
	// native methods:
	float LabelWidth();
	void AdjustToMaxLabelWidth( float maxWidth);
	void SetValueSilently( bool val);

private:
	// Hide copy-constructor and assignment:
	BmCheckControl( const BmCheckControl&);
	BmCheckControl operator=( const BmCheckControl&);
};


#endif
