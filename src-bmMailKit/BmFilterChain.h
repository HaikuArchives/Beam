/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmFilterChain_h
#define _BmFilterChain_h

#include "BmMailKit.h"

#include <Archivable.h>

#include "BmString.h"
#include "BmDataModel.h"

#include <vector>

using std::vector;

extern IMPEXPBMMAILKIT BmString BM_OutboundLabel;

class BmFilterChainList;
class BmChainedFilterList;
class BmFilterChain;
/*------------------------------------------------------------------------------*\
	BmChainedFilter 
		-	describes the position of a filter within a chain
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmChainedFilter : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmChainedFilter( const char* filterName, BmChainedFilterList* model);
	BmChainedFilter( BMessage* archive, BmChainedFilterList* model);
	virtual ~BmChainedFilter();
	
	// native methods:

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline int32 Position() const			{ return mPosition; }

	// setters:
	inline void Position( int32 p)		{ mPosition = p; }

	// archivable components:
	static const char* const MSG_POSITION;
	static const char* const MSG_FILTERNAME;
	static const int16 nArchiveVersion;

private:
	BmChainedFilter();						// hide default constructor
	// Hide copy-constructor and assignment:
	BmChainedFilter( const BmChainedFilter&);
	BmChainedFilter operator=( const BmChainedFilter&);
	
	int mPosition;
							// position of chained filter in chain
							// numbering uses step of two, (i.e. 1,3,5,7, etc.)
							// in order to be able to squeeze in an element
							// during drag'n'drop operations.
};

typedef vector< BmChainedFilter*> BmFilterPosVect;

/*------------------------------------------------------------------------------*\
	BmChainedFilterList
		- 	represents a list of chained filters.
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmChainedFilterList : public BmListModel {
	typedef BmListModel inherited;

	// archivable components:
	static const char* const MSG_NAME;
	static const int16 nArchiveVersion;

public:
	BmChainedFilterList( const char* name);
	BmChainedFilterList( BMessage* archive);
	virtual ~BmChainedFilterList();
	
	// native methods:
	void RenumberPos();
	//
	inline BmFilterPosVect::const_iterator posBegin() const 
													{ return mPosVect.begin(); }
	inline BmFilterPosVect::const_iterator posEnd() const 
													{ return mPosVect.end(); }

	// overrides of listmodel base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	const BmString SettingsFileName();
	bool AddItemToList( BmListModelItem* item, BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	bool StartJob();
	void InstantiateItem( BMessage* archive);
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, 
									const BmString& newVal);

private:

	BmChainedFilterList();					// hide default constructor
	// Hide copy-constructor and assignment:
	BmChainedFilterList( const BmChainedFilterList&);
	BmChainedFilterList operator=( const BmChainedFilterList&);
	
	BmFilterPosVect mPosVect;

};


/*------------------------------------------------------------------------------*\
	BmFilterChain
		- 	represents an item within the filter-chain-list.
		-	additionally, each of these items contains a ordered list of chained
			filters, so each item represents (contains) a list-model, too
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmFilterChain : public BmListModelItem {
	typedef BmListModelItem inherited;

	// archivable components:
	static const char* const MSG_NAME;
	static const int16 nArchiveVersion;

public:
	BmFilterChain( const char* name, BmFilterChainList* model);
	BmFilterChain( BMessage* archive, BmFilterChainList* model);
	virtual ~BmFilterChain();
	
	// native methods:

	// overrides of item base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	const BmString& RefName() const		{ return Key(); }

	// double-dispatches for convenience:
	inline BmString ModelNameNC() const	{ return mChainedFilters->ModelName(); }
	inline BLocker& ModelLocker() const	{ return mChainedFilters->ModelLocker(); }
	inline BmFilterPosVect::const_iterator posBegin() const 
													{ return mChainedFilters->posBegin(); }
	inline BmFilterPosVect::const_iterator posEnd() const 
													{ return mChainedFilters->posEnd(); }

	// getters:
	inline const BmString &Name() const	{ return Key(); }
	inline BmChainedFilterList* ChainedFilters()
													{ return mChainedFilters.Get(); }

private:
	BmFilterChain();							// hide default constructor
	// Hide copy-constructor and assignment:
	BmFilterChain( const BmFilterChain&);
	BmFilterChain operator=( const BmFilterChain&);
	
	BmRef<BmChainedFilterList> mChainedFilters;

};



/*------------------------------------------------------------------------------*\
	BmFilterChainList 
		-	holds list of all filter-chains
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmFilterChainList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmFilterChainList* CreateInstance();
	BmFilterChainList( const char* name);
	~BmFilterChainList();
	
	// native methods:
	void ForeignKeyChanged( const BmString& key, 
								   const BmString& oldVal, const BmString& newVal);
	void RemoveFilterFromAllChains( const BmString& filterName);
	
	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItem( BMessage* archive);
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	bool StartJob();

	static BmRef<BmFilterChainList> theInstance;

private:
	// Hide copy-constructor and assignment:
	BmFilterChainList( const BmFilterChainList&);
	BmFilterChainList operator=( const BmFilterChainList&);
	
};

#define TheFilterChainList BmFilterChainList::theInstance

#endif
