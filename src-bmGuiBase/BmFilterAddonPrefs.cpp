/*
	BmFilterAddonPrefs.cpp

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


#include "BmFilterAddonPrefs.h"

/********************************************************************************\
	BmFilterAddonPrefsView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmFilterAddonPrefsView()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilterAddonPrefsView::BmFilterAddonPrefsView(float minX, float minY, 
															  float maxX, float maxY)
	:	inherited( minimax( minX, minY, maxX, maxY), NULL)
{
}

/*------------------------------------------------------------------------------*\
	~BmFilterAddonPrefsView()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilterAddonPrefsView::~BmFilterAddonPrefsView() {
}

/*------------------------------------------------------------------------------*\
	PropagateChange()
		-	informs any observers (the prefs-view) about a modification
\*------------------------------------------------------------------------------*/
void BmFilterAddonPrefsView::PropagateChange() {
	BMessage msg( BM_NTFY_FILTER_ADDON_MODIFIED);
	SendNotices( BM_NTFY_FILTER_ADDON_MODIFIED, &msg);
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	default implementation, does nothing
\*------------------------------------------------------------------------------*/
void BmFilterAddonPrefsView::Initialize() {
}

/*------------------------------------------------------------------------------*\
	Activate()
		-	default implementation, does nothing
\*------------------------------------------------------------------------------*/
void BmFilterAddonPrefsView::Activate() {
}
