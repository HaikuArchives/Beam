/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <FindDirectory.h>
#include <Directory.h>
#include <Message.h>
#include <Path.h>

//#include "BubbleHelper.h"

#include "BmBasics.h"
#include "BmPrefs.h"
#include "BmFilter.h"
#include "BmLogHandler.h"
#include "BmMailFilter.h"
#include "BmRosterBase.h"
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
	:	inherited( FindMsgString( archive, MSG_NAME), model, 
					  (BmListModelItem*)NULL)
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
		archive->AddString( MSG_KIND, "Sieve-script");
	}
	mKind = archive->FindString( MSG_KIND);
	SetupAddonPart();
}

/*------------------------------------------------------------------------------*\
	~BmFilter()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilter::~BmFilter() {
	delete mAddon;
}

/*------------------------------------------------------------------------------*\
	SetupAddonPart()
		-	
\*------------------------------------------------------------------------------*/
void BmFilter::SetupAddonPart() {
	if (mKind.Length()) {
		mKind.CapitalizeEachWord();
		BmInstantiateFilterFunc instFunc 
			= FilterAddonMap[mKind].instantiateFilterFunc;
		if (instFunc 
		&& (mAddon = (*instFunc)( Key(), &mAddonArchive, mKind)) != NULL) {
			BM_LOG2( BM_LogFilter, 
						BmString("Instantiated Filter-Addon <") << mKind << ":" 
							<< Key() << ">");
			mAddon->Initialize();
		} else
			BM_LOG2( BM_LogFilter, 
						BmString("Unable to instantiate Filter-Addon <") << mKind 
							<< ":" << Key() << ">. This filter will be disabled.");
	} else
		BM_LOG2( BM_LogFilter, 
					BmString("Unable to instantiate Filter-Addon <") << Key() 
						<< "> since it has no kind!. This filter will be disabled.");
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
bool BmFilter::SanityCheck( BmString& complaint, BmString& fieldName) const
{
	if (mAddon)
		return mAddon->SanityCheck( complaint, fieldName);
	return true;
}

/*------------------------------------------------------------------------------*\
	Execute()
		-	double-dispatches execution to addon
\*------------------------------------------------------------------------------*/
bool BmFilter::Execute( BmMsgContext* msgContext)
{
	if (!mAddon)
		return false;
	return mAddon->Execute( msgContext, &mJobSpecifier);
}


/********************************************************************************\
	BmFilterList
\********************************************************************************/

BmRef< BmFilterList> BmFilterList::theInstance( NULL);

const int16 BmFilterList::nArchiveVersion = 1;

BmFilterAddonMap FilterAddonMap;

const char* const BmFilterList::LEARN_AS_SPAM_NAME = "<<<LearnAsSpam>>>";
const char* const BmFilterList::LEARN_AS_TOFU_NAME = "<<<LearnAsTofu>>>";

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
	:	inherited( name, BM_LogFilter)
{
	LoadAddons();
}

/*------------------------------------------------------------------------------*\
	~BmFilterList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmFilterList::~BmFilterList() {
	Cleanup();
	UnloadAddons();
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the signature-list
\*------------------------------------------------------------------------------*/
const BmString BmFilterList::SettingsFileName() {
	return BmString( BeamRoster->SettingsPath()) << "/Filters";
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
	BmString addonPath = BmString(BeamRoster->AppPath()) + "/add-ons/Filters";
	SetupFolder( addonPath.String(), &addonDir);

	// ...and scan through all its entries for filter-add-ons:
	while ( addonDir.GetNextEntry( &entry, true) == B_OK) {
		if (entry.IsFile()) {
			char nameBuf[B_FILE_NAME_LENGTH];
			entry.GetName( nameBuf);
			// try to load addon:
			const char** filterKinds;
			const char** defaultFilterName;
			BmFilterAddonDescr ao;
			ao.name = nameBuf;
			ao.name.CapitalizeEachWord();
			entry.GetPath( &path);
			if ((ao.image = load_add_on( path.Path())) < 0) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")
									<<ao.name<<"\n\nError:\n\t"<<strerror( ao.image));
				continue;
			}
			if ((err = get_image_symbol( 
				ao.image, "InstantiateFilter", B_SYMBOL_TYPE_ANY, 
				(void**)&ao.instantiateFilterFunc
			)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")
								<<ao.name<<"\n\nMissing symbol 'InstantiateFilter'");
				continue;
			}
			if ((err = get_image_symbol( 
				ao.image, "InstantiateFilterPrefs", B_SYMBOL_TYPE_ANY, 
				(void**)&ao.instantiateFilterPrefsFunc
			)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")
								<<ao.name
								<<"\n\nMissing symbol 'InstantiateFilterPrefs'");
				continue;
			}
			if ((err = get_image_symbol( 
				ao.image, "FilterKinds", B_SYMBOL_TYPE_ANY, 
				(void**)&filterKinds
			)) != B_OK) {
				BM_SHOWERR( BmString("Unable to load filter-addon\n\t")
								<<ao.name<<"\n\nMissing symbol 'FilterKinds'");
				continue;
			}
			if ((err = get_image_symbol( 
				ao.image, "DefaultFilterName", B_SYMBOL_TYPE_ANY, 
				(void**)&defaultFilterName
			)) == B_OK)
				ao.defaultFilterName = *defaultFilterName;
			else
				ao.defaultFilterName = "new filter";
#if 0
			// we try to set TheBubbleHelper and TheLogHandler globals inside 
			// the addon to our current values:
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
#endif
			// now we add the addon to our map (one entry per filter-kind):
			while( *filterKinds) {
				BmString kind(*filterKinds);
				FilterAddonMap[*filterKinds++] = ao;
				if (kind.ICompare("Spam") == 0) {
					// a spam-filter requires two internal filters (learnAsSpam
					// and learnAsTofu) which don't appear as part of filter-list:
					mLearnAsSpamFilter 
						= new BmFilter( LEARN_AS_SPAM_NAME, "Spam", NULL);
					BMessage learnAsSpamJob;
					learnAsSpamJob.AddString("jobSpecifier", "LearnAsSpam");
					mLearnAsSpamFilter->JobSpecifier(learnAsSpamJob);
					mLearnAsTofuFilter 
						= new BmFilter( LEARN_AS_TOFU_NAME, "Spam", NULL);
					BMessage learnAsTofuJob;
					learnAsTofuJob.AddString("jobSpecifier", "LearnAsTofu");
					mLearnAsTofuFilter->JobSpecifier(learnAsTofuJob);
				}
			}
			BM_LOG( BM_LogFilter, BmString("Successfully loaded addon ") 
						<< ao.name);

		}
	}
	BM_LOG2( BM_LogFilter, BmString("End of LoadAddons() for FilterList"));
}

/*------------------------------------------------------------------------------*\
	UnloadAddons()
		-	unloads all loaded filter-addons
\*------------------------------------------------------------------------------*/
void BmFilterList::UnloadAddons() {
	mLearnAsSpamFilter = NULL;
	mLearnAsTofuFilter = NULL;
	BmFilterAddonMap::const_iterator iter;
	for( iter = FilterAddonMap.begin(); iter != FilterAddonMap.end(); ++iter) {
		unload_add_on( iter->second.image);
	}
	FilterAddonMap.clear();
}

/*------------------------------------------------------------------------------*\
	DefaultNameForFilterKind()
		-	determines the default-name for the given filter kind
\*------------------------------------------------------------------------------*/
BmString BmFilterList::DefaultNameForFilterKind( const BmString& filterKind)
{
	return FilterAddonMap[filterKind].defaultFilterName;
}

/*------------------------------------------------------------------------------*\
	HaveSpamFilter()
		-	determines whether or not the SPAM-filter has been loaded
\*------------------------------------------------------------------------------*/
bool BmFilterList::HaveSpamFilter() const
{
	return FilterAddonMap.find("Spam") != FilterAddonMap.end();
}

/*------------------------------------------------------------------------------*\
	InstantiateItem( archive)
		-	instantiates a filter from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterList::InstantiateItem( BMessage* archive) {
	BmFilter* newFilter = new BmFilter( archive, this);
	AddItemToList( newFilter);
}

