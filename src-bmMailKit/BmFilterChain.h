/*
	BmFilterChain.h

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


#ifndef _BmFilterChain_h
#define _BmFilterChain_h

#include <Archivable.h>

#include "BmString.h"
#include "BmDataModel.h"

#include <map.h>

extern BmString BM_DefaultOutItemLabel;

class BmFilterChainList;
class BmFilterChain;
/*------------------------------------------------------------------------------*\
	BmChainedFilter 
		-	describes the position of a filter within a chain
\*------------------------------------------------------------------------------*/
class BmChainedFilter : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmChainedFilter( const char* filterName, BmFilterChain* model);
	BmChainedFilter( BMessage* archive, BmFilterChain* model);
	virtual ~BmChainedFilter();
	
	// native methods:

	// stuff needed for Archival:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline int32 Position() const			{ return mPosition; }

	// setters:
	inline void Position( int32 p)		{ mPosition = p;  
													  TellModelItemUpdated( UPD_ALL | UPD_SORT); }

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



typedef multimap< int32, BmChainedFilter*> BmFilterPosMap;

/*------------------------------------------------------------------------------*\
	BmFilterChain
		-	represents a ordered list of filter-references (BmChainedFilters)
		- 	additionally represents an item within the filter-chain-list 
			(so an object of this class is an item as well as a list!).
\*------------------------------------------------------------------------------*/
class BmFilterChain : virtual public BmListModelItem, virtual public BmListModel {
	typedef BmListModelItem inheritedItem;
	typedef BmListModel inheritedList;

	// archivable components:
	static const char* const MSG_NAME;
	static const int16 nListArchiveVersion;
	static const int16 nItemArchiveVersion;

public:
	BmFilterChain( const char* name, BmFilterChainList* model);
	BmFilterChain( BMessage* archive, BmFilterChainList* model);
	virtual ~BmFilterChain();
	
	// native methods:
	void RenumberPos();
	//
	inline BmFilterPosMap::const_iterator posBegin() const { return mPosMap.begin(); }
	inline BmFilterPosMap::const_iterator posEnd() const { return mPosMap.end(); }

	// overrides of item base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nItemArchiveVersion; }
	const BmString& RefName() const		{ return Key(); }

	// overrides of listmodel base:
	const BmString SettingsFileName();
	void InstantiateItems( BMessage* archive);
	int16 ListArchiveVersion() const		{ return nListArchiveVersion; }
	bool AddItemToList( BmListModelItem* item, BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	bool StartJob();
	void ForeignKeyChanged( const BmString& key, 
									const BmString& oldVal, 
									const BmString& newVal);

	// getters:
	inline const BmString &Name() const	{ return Key(); }

	// setters:

private:
	BmFilterChain();							// hide default constructor
	// Hide copy-constructor and assignment:
	BmFilterChain( const BmFilterChain&);
	BmFilterChain operator=( const BmFilterChain&);
	
	BmFilterPosMap mPosMap;

};



/*------------------------------------------------------------------------------*\
	BmFilterChainList 
		-	holds list of all filter-chains
\*------------------------------------------------------------------------------*/
class BmFilterChainList : public BmListModel {
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
	void InstantiateItems( BMessage* archive);
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
