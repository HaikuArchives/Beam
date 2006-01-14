/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmFilter_h
#define _BmFilter_h

#include "BmMailKit.h"

#include <Archivable.h>
#include <Message.h>

#include "BmFilterAddon.h"
#include "BmDataModel.h"
#include "BmString.h"

class BmFilterList;

enum {
	BM_JOBWIN_FILTER 				= 'bmed'
						// sent to JobMetaController in order to start filter-job
};

class BmFilterAddonPrefsView;

/*------------------------------------------------------------------------------*\
	BmFilterAddonDescr
		-	holds the known info about a single addon
\*------------------------------------------------------------------------------*/
struct IMPEXPBMMAILKIT BmFilterAddonDescr {
	BmFilterAddonDescr()
		:	image( 0)
		,	instantiateFilterFunc( NULL)
		,	instantiateFilterPrefsFunc( NULL)
		,	addonPrefsView( NULL)			{}

	image_id image;
	BmString name;
	BmString defaultFilterName;
	BmInstantiateFilterFunc instantiateFilterFunc;
	BmInstantiateFilterPrefsFunc instantiateFilterPrefsFunc;
	BmFilterAddonPrefsView* addonPrefsView;
};

/*------------------------------------------------------------------------------*\
	BmFilter 
		-	base class for all filters
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmFilter : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmFilter( const char* name, const BmString& kind, BmFilterList* model);
	BmFilter( BMessage* archive, BmFilterList* model);
	virtual ~BmFilter();
	
	// native methods:
	bool SanityCheck( BmString& complaint, BmString& fieldName) const;
	bool Execute( BmMsgContext* msgContext);

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline bool IsDisabled() const		{ return mAddon == NULL; }
	inline const BmString &Name() const	{ return Key(); }
	inline const BmString &Kind() const	{ return mKind; }

	inline BmFilterAddon* Addon()			{ return mAddon; }
	
	// setters
	void JobSpecifier(BMessage& jobSpecs)
													{ mJobSpecifier = jobSpecs; }

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
	BMessage mJobSpecifier;
							// specific job-types (learnAsSpam) can be specified here
							// normally, this is empty
private:
	BmFilter();									// hide default constructor
	
	// Hide copy-constructor and assignment:
	BmFilter( const BmFilter&);
	BmFilter operator=( const BmFilter&);
};



typedef map< BmString, BmFilterAddonDescr> BmFilterAddonMap;
extern IMPEXPBMMAILKIT BmFilterAddonMap FilterAddonMap;

/*------------------------------------------------------------------------------*\
	BmFilterList 
		-	holds list of all Filters
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmFilterList : public BmListModel {
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
	BmString DefaultNameForFilterKind( const BmString& filterKind);
	bool HaveSpamFilter() const;
	BmRef<BmFilter>& LearnAsSpamFilter()
													{ return mLearnAsSpamFilter; }
	BmRef<BmFilter>& LearnAsTofuFilter()
													{ return mLearnAsTofuFilter; }

	// overrides of listmodel base:
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, const BmString& newVal);
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	static BmRef< BmFilterList> theInstance;

	static const char* const LEARN_AS_SPAM_NAME;
	static const char* const LEARN_AS_TOFU_NAME;

private:

	BmRef< BmFilter> mLearnAsSpamFilter;
	BmRef< BmFilter> mLearnAsTofuFilter;

	// Hide copy-constructor and assignment:
	BmFilterList( const BmFilterList&);
	BmFilterList operator=( const BmFilterList&);
};

#define TheFilterList BmFilterList::theInstance

#endif
