/*
	BmFilterAddonPrefs.h

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


#ifndef _BmFilterAddonPrefs_h
#define _BmFilterAddonPrefs_h

#include <BeBuild.h>
#ifdef B_BEOS_VERSION_DANO
	class BFont;
#endif

#include <Message.h>

#include <VGroup.h>

#include "SantaPartsForBeam.h"
#include "BmString.h"

class BmFilterAddon;
/*------------------------------------------------------------------------------*\
	BmFilterAddonPrefsView
		-	base class for all filter-addon prefs-views, this is used as the
			filter-addon-view-API
\*------------------------------------------------------------------------------*/
#define BM_NTFY_FILTER_ADDON_MODIFIED 'bmFX'
							// the filter-addon has been changed
class IMPEXPSANTAPARTSFORBEAM BmFilterAddonPrefsView : public VGroup {
	typedef VGroup inherited;

public:
	BmFilterAddonPrefsView( minimax minmax);
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

typedef BmFilterAddonPrefsView* (*BmInstantiateFilterPrefsFunc)( minimax minmax, 
																					  const BmString& kind);


#endif
