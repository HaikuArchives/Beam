/*
	BmFilter.h

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


#ifndef _BmFilter_h
#define _BmFilter_h

#include <Archivable.h>
#include <Message.h>

#include "BmFilterAddon.h"
#include "BmFilterAddonPrefs.h"
#include "BmDataModel.h"
#include "BmString.h"

class BmFilterList;

#define BM_JOBWIN_FILTER					'bmed'
						// sent to JobMetaController in order to start filter-job

/*------------------------------------------------------------------------------*\
	BmFilterAddonDescr
		-	holds the known info about a single addon
\*------------------------------------------------------------------------------*/
struct BmFilterAddonDescr {
	BmFilterAddonDescr()
		:	image( 0)
		,	instantiateFilterFunc( NULL)
		,	instantiateFilterPrefsFunc( NULL)
		,	addonPrefsView( NULL)			{}

	image_id image;
	BmString name;
	BmInstantiateFilterFunc instantiateFilterFunc;
	BmInstantiateFilterPrefsFunc instantiateFilterPrefsFunc;
	BmFilterAddonPrefsView* addonPrefsView;
};

/*------------------------------------------------------------------------------*\
	BmFilter 
		-	base class for all filters
\*------------------------------------------------------------------------------*/
class BmFilter : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmFilter( const char* name, const BmString& kind, BmFilterList* model);
	BmFilter( BMessage* archive, BmFilterList* model);
	virtual ~BmFilter();
	
	// native methods:
	bool SanityCheck( BmString& complaint, BmString& fieldName);

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline bool IsDisabled() const		{ return mAddon == NULL; }
	inline const BmString &Name() const	{ return Key(); }
	inline const BmString &Kind() const	{ return mKind; }

	inline BmFilterAddon* Addon()			{ return mAddon; }
	
	// setters:
	inline void Addon( BmFilterAddon* fa) { mAddon = fa; }

	// archivable components:
	static const char* const MSG_NAME;
	static const char* const MSG_KIND;
	static const char* const MSG_ADDON_ARCHIVE;
	static const int16 nArchiveVersion;

protected:
	void SetupAddonPart();
							// instantiates addon-part of filter
	BmFilterAddon* mAddon;
							// the addon-part that implements this filter.
							// This is NULL if addon could not be loaded
	mutable BMessage mAddonArchive;
							// the last archived state of the addon (this is used to 
							// save the data when the addon can not be loaded)
	BmString mKind;
							// type of filter (name of addon)
private:
	BmFilter();									// hide default constructor
	
	// Hide copy-constructor and assignment:
	BmFilter( const BmFilter&);
	BmFilter operator=( const BmFilter&);
};



typedef map< BmString, BmFilterAddonDescr> BmFilterAddonMap;
extern BmFilterAddonMap FilterAddonMap;

/*------------------------------------------------------------------------------*\
	BmFilterList 
		-	holds list of all Filters
\*------------------------------------------------------------------------------*/
class BmFilterList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmFilterList* CreateInstance();
	BmFilterList( const char* name);
	~BmFilterList();
	
	// native methods:
	void LoadAddons();
	void UnloadAddons();

	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef< BmFilterList> theInstance;

private:

	// Hide copy-constructor and assignment:
	BmFilterList( const BmFilterList&);
	BmFilterList operator=( const BmFilterList&);
};

#define TheFilterList BmFilterList::theInstance

#endif
