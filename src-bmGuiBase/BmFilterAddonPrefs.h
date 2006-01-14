/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmFilterAddonPrefs_h
#define _BmFilterAddonPrefs_h

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
#endif

#include <Message.h>

#include <VGroup.h>

#include "BmGuiBase.h"
#include "BmString.h"

class BmFilterAddon;
/*------------------------------------------------------------------------------*\
	BmFilterAddonPrefsView
		-	base class for all filter-addon prefs-views, this is used as the
			filter-addon-view-API
\*------------------------------------------------------------------------------*/
#define BM_NTFY_FILTER_ADDON_MODIFIED 'bmFX'
							// the filter-addon has been changed
class IMPEXPBMGUIBASE BmFilterAddonPrefsView : public VGroup {
	typedef VGroup inherited;

public:
	BmFilterAddonPrefsView(float minX, float minY, float maxX, float maxY);
	virtual ~BmFilterAddonPrefsView();
	
	// native methods:
	void PropagateChange();

	// abtract methods, need to be implemented by every filter-addon-view:
	virtual const char* Kind() const = 0;
	virtual void ShowFilter( BmFilterAddon* addon) = 0;
	virtual void Initialize();
	virtual void Activate();

private:

	// Hide copy-constructor and assignment:
	BmFilterAddonPrefsView( const BmFilterAddonPrefsView&);
// PPC-codewarrior doesn't like the following...
//	BmFilterAddonPrefsView operator=( const BmFilterAddonPrefsView&);
};

#endif
