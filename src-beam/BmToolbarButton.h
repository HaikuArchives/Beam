/*
	BmToolbarButton.h
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


#ifndef _BmToolbarButton_h
#define _BmToolbarButton_h

#include <MPictureButton.h>

class BmToolbarButton : public MPictureButton
{
	typedef MPictureButton inherited;

public:
	// creator-func, c'tors and d'tor:
	BmToolbarButton( const char *label, BBitmap* image, 
						  BMessage *message, BHandler *handler, 
						  const char* tipText=NULL);
	~BmToolbarButton();

	// native methods:
	BPicture* CreateOnPictureFor( const char* label, BBitmap* image);
	BPicture* CreateOffPictureFor( const char* label, BBitmap* image);

	// overrides of Button base:
	void Draw( BRect bounds);

private:
	bool mHighlighted;

	// Hide copy-constructor and assignment:
	BmToolbarButton( const BmToolbarButton&);
	BmToolbarButton operator=( const BmToolbarButton&);
};


#endif
