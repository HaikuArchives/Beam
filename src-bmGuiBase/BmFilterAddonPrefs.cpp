/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

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
	:	inherited( minimax( int(minX), int(minY), int(maxX), int(maxY)), NULL)
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
