/*
	BmFilter.cpp

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


#include <FindDirectory.h>
#include <Message.h>

#include "BubbleHelper.h"

#include "BmBasics.h"
#include "BmApp.h"
#include "BmPrefs.h"
#include "BmFilter.h"
#include "BmLogHandler.h"
#include "BmMailFilter.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

/********************************************************************************\
	BmFilter
\********************************************************************************/

const char* const BmFilter::MSG_NAME = 			"bm:name";
const char* const BmFilter::MSG_KIND = 			"bm:kind";
const char* const BmFilter::MSG_ADDON_ARCHIVE = "bm:addonarc";
const int16 BmFilter::nArchiveVersion = 6;

/*------------------------------------------------------------------------------*\
	BmFilter()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilter::BmFilter( const char* name, const BmString& kind, BmFilterList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mAddon( NULL)
	,	mKind( kind)
{
	SetupAddonPart();
}

/*------------------------------------------------------------------------------*\
	BmFilter( archive)
		-	c'tor
		-	constructs a BmFilter from a BMessage
\*------------------------------------------------------------------------------*/
BmFilter::BmFilter( BMessage* archive, BmFilterList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
	,	mAddon( NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	if (version>5)
		archive->FindMessage( MSG_ADDON_ARCHIVE, &mAddonArchive);
	else {
		// take along the contents from our old (SIEVE) filters:
		BmString content = archive->FindString("bm:content");
		if (content.Length())
			mAddonArchive.AddString( "bm:content", content.String());
		archive->AddString( MSG_KIND, "Sieve-Script");
	}
	mKind = archive->FindString( MSG_KIND);
	SetupAddonPart();
}

/*------------------------------------------------------------------------------*\
	~BmFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilter::~BmFilter() {
}

/*------------------------------------------------------------------------------*\
	SetupAddonPart()
		-	
\*------------------------------------------------------------------------------*/
void BmFilter::SetupAddonPart() {
	if (mKind.Length()) {
		mKind.CapitalizeEachWord();
		BmInstantiateFilterFunc instFunc = FilterAddonMap[mKind].instantiateFilterFunc;
		if (instFunc && (mAddon = (*instFunc)( Key(), &mAddonArchive, mKind)) != NULL)
			BM_LOG2( BM_LogFilter, BmString("Instantiated Filter-Addon <") << mKind << ":" << Key() << ">");
		else
			BM_LOG2( BM_LogFilter, BmString("Unable to instantiate Filter-Addon <") << mKind << ":" << Key() << ">. This filter will be disabled.");
	} else
		BM_LOG2( BM_LogFilter, BmString("Unable to instantiate Filter-Addon <") << Key() << "> since it has no kind!. This filter will be disabled.");
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmFilter::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String())
		||	archive->AddString( MSG_KIND, mKind.String()));
	if (ret==B_OK) {
		if (mAddon) {
			mAddonArchive.MakeEmpty();
			ret = mAddon->Archive( &mAddonArchive, deep);
		}
		ret = archive->AddMessage( MSG_ADDON_ARCHIVE, &mAddonArchive);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	SanityCheck()
		-	checks if the current values make sense and returns error-info through
			given out-params
		-	returns true if values are ok, false (and error-info) if not
		-	double-dispatches check to addon
\*------------------------------------------------------------------------------*/
bool BmFilter::SanityCheck( BmString& complaint, BmString& fieldName) {
	if (Addon())
		return Addon()->SanityCheck( complaint, fieldName);
	return true;
}



/********************************************************************************\
	BmFilterList
\********************************************************************************/

BmRef< BmFilterList> BmFilterList::theInstance( NULL);

const int16 BmFilterList::nArchiveVersion = 1;

BmFilterAddonMap FilterAddonMap;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmFilterList* BmFilterList::CreateInstance() {
	if (!theInstance)
		theInstance = new BmFilterList( "FilterList");
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmFilterList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmFilterList::BmFilterList( const char* name)
	:	inherited( name)
{
	LoadAddons();
}

/*------------------------------------------------------------------------------*\
	~BmFilterList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmFilterList::~BmFilterList() {
	UnloadAddons();
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the signature-list
\*------------------------------------------------------------------------------*/
const BmString BmFilterList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/Filters";
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	we pass the info about the changed foreign-key on to each add-on:
\*------------------------------------------------------------------------------*/
void BmFilterList::ForeignKeyChanged( const BmString& key, 
												  const BmString& oldVal, 
												  const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmFilter* filter = dynamic_cast< BmFilter*>( iter->second.Get());
		if (filter && filter->Addon())
			filter->Addon()->ForeignKeyChanged( key, oldVal, newVal);
	}
}

/*------------------------------------------------------------------------------*\
	LoadAddons()
		-	loads all available filter-addons
\*------------------------------------------------------------------------------*/
void BmFilterList::LoadAddons() {
	BDirectory addonDir;
	BPath path;
	BEntry entry;
	status_t err;

	BM_LOG2( BM_LogFilter, BmString("Start of LoadAddons() for FilterList"));

	// determine the path to the user-config-directory:
	if (find_directory( B_USER_ADDONS_DIRECTORY, &path) != B_OK)
		BM_THROW_RUNTIME( "Sorry, could not determine user's addon-dir !?!");
	BmString addonPath = bmApp->AppPath() + "/add-ons/Filters";
	TheResources->GetFolder( addonPath, addonDir);

	// ...and scan through all its entries for other mail-folders:
	while ( addonDir.GetNextEntry( &entry, true) == B_OK) {
		if (entry.IsFile()) {
			char nameBuf[B_FILE_NAME_LENGTH];
			entry.GetName( nameBuf);
			// try to load addon:
			const char** filterKinds;
			BmFilterAddonDescr ao;
			ao.name = nameBuf;
			ao.name.CapitalizeEachWord();
			entry.GetPath( &path);
			if ((ao.image = load_add_on( path.Path())) < 0) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")<<ao.name<<"\n\nError:\n\t"<<strerror( ao.image));
				continue;
			}
			if ((err = get_image_symbol( ao.image, "InstantiateFilter", 
												  B_SYMBOL_TYPE_ANY, (void**)&ao.instantiateFilterFunc)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")<<ao.name<<"\n\nMissing symbol 'InstantiateFilter'");
				continue;
			}
			if ((err = get_image_symbol( ao.image, "InstantiateFilterPrefs", 
												  B_SYMBOL_TYPE_ANY, (void**)&ao.instantiateFilterPrefsFunc)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")<<ao.name<<"\n\nMissing symbol 'InstantiateFilterPrefs'");
				continue;
			}
			if ((err = get_image_symbol( ao.image, "FilterKinds", 
												  B_SYMBOL_TYPE_ANY, (void**)&filterKinds)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")<<ao.name<<"\n\nMissing symbol 'FilterKinds'");
				continue;
			}
			// we try to set TheBubbleHelper and TheLogHandler globals inside the addon to our current
			// values:
			BubbleHelper** bhPtr;
			if (get_image_symbol( ao.image, "TheBubbleHelper", B_SYMBOL_TYPE_ANY, 
										 (void**)&bhPtr) == B_OK) {
				*bhPtr = TheBubbleHelper;
			}
			BmLogHandler** lhPtr;
			if (get_image_symbol( ao.image, "TheLogHandler", B_SYMBOL_TYPE_ANY, 
										 (void**)&lhPtr) == B_OK) {
				*lhPtr = TheLogHandler;
			}
			// now we add the addon to our map (one entry per filter-kind):
			while( *filterKinds)
				FilterAddonMap[*filterKinds++] = ao;
			BM_LOG( BM_LogFilter, BmString("Successfully loaded addon ") << ao.name);
		}
	}
	BM_LOG2( BM_LogFilter, BmString("End of LoadAddons() for FilterList"));
}

/*------------------------------------------------------------------------------*\
	UnloadAddons()
		-	unloads all loaded filter-addons
\*------------------------------------------------------------------------------*/
void BmFilterList::UnloadAddons() {
	BmFilterAddonMap::const_iterator iter;
	for( iter = FilterAddonMap.begin(); iter != FilterAddonMap.end(); ++iter) {
		unload_add_on( iter->second.image);
	}
	FilterAddonMap.clear();
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the signature-list from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogFilter, BmString("Start of InstantiateItems() for FilterList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		if ((err = archive->FindMessage( 
			BmListModelItem::MSG_CHILDREN, i, &msg
		)) != B_OK)
			BM_THROW_RUNTIME(BmString("Could not find signature nr. ") << i+1 
										<< " \n\nError:" << strerror(err));
		BmFilter* newFilter = new BmFilter( &msg, this);
		AddItemToList( newFilter);
	}
	BM_LOG2( BM_LogFilter, BmString("End of InstantiateItems() for FilterList"));
	mInitCheck = B_OK;
}

