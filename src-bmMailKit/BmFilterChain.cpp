/*
	BmFilterChain.cpp

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


#include <Message.h>

#include "BmBasics.h"
#include "BmPrefs.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmLogHandler.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmChainedFilter
\********************************************************************************/

const char* const BmChainedFilter::MSG_POSITION = 	"bm:pos";
const char* const BmChainedFilter::MSG_FILTERNAME = "bm:fname";

const int16 BmChainedFilter::nArchiveVersion = 2;

BmString BM_DefaultOutItemLabel("<outbound>");

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

/*------------------------------------------------------------------------------*\
	BmChainedFilter()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( const char* name, BmFilterChain* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mPosition( 9999)		// a number larger than the number of chained filters
									// so that this filter is appended to the list
{
	if (model)
		model->RenumberPos();
}

/*------------------------------------------------------------------------------*\
	BmChainedFilter( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( BMessage* archive, BmFilterChain* model) 
	:	inherited( BmString()<<FindMsgString( archive, MSG_FILTERNAME), model, 
					  (BmListModelItem*)NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mPosition = FindMsgInt32( archive, MSG_POSITION);
}

/*------------------------------------------------------------------------------*\
	~BmFilterChain()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmChainedFilter::~BmChainedFilter() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmChainedFilter into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmChainedFilter::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inherited::Archive( archive, deep)
		||	archive->AddString( MSG_FILTERNAME, Key().String())
		||	archive->AddInt32( MSG_POSITION, mPosition));
	return ret;
}



/********************************************************************************\
	BmFilterChain
\********************************************************************************/

const int16 BmFilterChain::nItemArchiveVersion = 1;
const int16 BmFilterChain::nListArchiveVersion = 1;

const char* const BmFilterChain::MSG_NAME = "bm:name";

/*------------------------------------------------------------------------------*\
	BmFilterChain()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilterChain::BmFilterChain( const char* name, BmFilterChainList* model) 
	:	inheritedItem( name, model, (BmListModelItem*)NULL)
	,	inheritedList( name)
{
	// tell filter-list that our items have a foreign-key on its items:
	TheFilterList->AddForeignKey( BmChainedFilter::MSG_FILTERNAME,	this);
}

/*------------------------------------------------------------------------------*\
	BmFilterChain( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmFilterChain::BmFilterChain( BMessage* archive, BmFilterChainList* model) 
	:	inheritedItem( FindMsgString( archive, MSG_NAME), 
							model, (BmListModelItem*)NULL)
	,	inheritedList( FindMsgString( archive, MSG_NAME))
{
	int16 version;
	if (archive->FindInt16( inheritedItem::MSG_VERSION, &version) != B_OK)
		version = 0;
	// tell filter-list that our items have a foreign-key on its items:
	TheFilterList->AddForeignKey( BmChainedFilter::MSG_FILTERNAME,	this);
}

/*------------------------------------------------------------------------------*\
	~BmFilterChain()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmFilterChain::~BmFilterChain() {
}

/*------------------------------------------------------------------------------*\
	Archive( archive, deep)
		-	writes BmFilterChain into archive
		-	parameter deep makes no difference...
\*------------------------------------------------------------------------------*/
status_t BmFilterChain::Archive( BMessage* archive, bool deep) const {
	status_t ret = (inheritedList::Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String()));
	return ret;
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmFilterChain::ForeignKeyChanged( const BmString& key, 
													const BmString& oldVal, 
													const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = inheritedList::begin(); iter != inheritedList::end(); ++iter) {
		BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( iter->second.Get());
		if (key == BmChainedFilter::MSG_FILTERNAME) {
			if (filter && filter->Key() == oldVal)
				RenameItem( oldVal, newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
	RenumberPos()
		-	makes sure that there are no holes in the numerical order of filters
			belonging to this chain
		-	N.B.: In fact there are holes, but regularly, such that the
			items are numbered by a step of 2.
\*------------------------------------------------------------------------------*/
void BmFilterChain::RenumberPos() {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	// first create a temporary map that is sorted by position:
	BmFilterPosMap tempMap;
	BmModelItemMap::const_iterator iter;
	for( iter = inheritedList::begin(); iter != inheritedList::end(); ++iter) {
		BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( iter->second.Get());
		tempMap[filter->Position()] = filter;
	}
	// now copy over to mPosMap and renumber while we're at it:
	int32 currPos = 1;
	mPosMap.clear();
	BmFilterPosMap::iterator posIter;
	for( posIter = tempMap.begin(); posIter != tempMap.end(); ++posIter) {
		BmChainedFilter* filter = posIter->second;
		if (filter->Position() != currPos)
			filter->Position( currPos);
		mPosMap[currPos] = filter;
		currPos += 2;
	}
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList( item)
		-	extends normal behaviour with renumbering of item-positions
\*------------------------------------------------------------------------------*/
void BmFilterChain::RemoveItemFromList( BmListModelItem* item) {
	BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( item);
	if (filter) {
		inheritedList::RemoveItemFromList( filter);
		RenumberPos();
	}
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the filterchain-list
\*------------------------------------------------------------------------------*/
const BmString BmFilterChain::SettingsFileName() {
	// should never be called, since a filter-chain lives in the settings file of
	// the filterchain-list
	BM_ASSERT( 0);
	return "";
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	we just set init-ok
\*------------------------------------------------------------------------------*/
bool BmFilterChain::StartJob() {
	mInitCheck = B_OK;
	return true;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmFilterChain::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogFilter, BmString("Start of InstantiateItems() for FilterChain") << Key());
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find item nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmChainedFilter* newFilter = new BmChainedFilter( &msg, this);
		BM_LOG3( BM_LogFilter, BmString("ChainedFilter <") << newFilter->Key() << "> read");
		AddItemToList( newFilter);
	}
	BM_LOG2( BM_LogFilter, BmString("End of InstantiateItems() for FilterChain") << Key());
	RenumberPos();
	mInitCheck = B_OK;
}



/********************************************************************************\
	BmFilterChainList
\********************************************************************************/

BmRef< BmFilterChainList> BmFilterChainList::theInstance( NULL);

const int16 BmFilterChainList::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	initialiazes object by reading info from settings file (if any)
\*------------------------------------------------------------------------------*/
BmFilterChainList* BmFilterChainList::CreateInstance() {
	if (!theInstance)
		theInstance = new BmFilterChainList( "FilterChainList");
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmFilterChainList()
		-	default constructor, creates empty list
\*------------------------------------------------------------------------------*/
BmFilterChainList::BmFilterChainList( const char* name)
	:	inherited( name)
{
}

/*------------------------------------------------------------------------------*\
	~BmFilterChainList()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmFilterChainList::~BmFilterChainList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the filterchain-list
\*------------------------------------------------------------------------------*/
const BmString BmFilterChainList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/FilterChains";
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmFilterChainList::ForeignKeyChanged( const BmString& key, 
														 const BmString& oldVal, 
														 const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	if (key == BmChainedFilter::MSG_FILTERNAME) {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); iter != end(); ++iter) {
			BmFilterChain* chain = dynamic_cast< BmFilterChain*>( iter->second.Get());
			chain->RenameItem( oldVal, newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
	RemoveFilterFromAllChains( filterName)
		-	removes filter with given name from all filter-chains.
\*------------------------------------------------------------------------------*/
void BmFilterChainList::RemoveFilterFromAllChains( const BmString& filterName) {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmFilterChain* chain = dynamic_cast< BmFilterChain*>( iter->second.Get());
		chain->RemoveItemByKey( filterName);
	}
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the signature-list from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterChainList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogFilter, BmString("Start of InstantiateItems() for FilterChainList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find item nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmFilterChain* newChain = new BmFilterChain( &msg, this);
		BM_LOG3( BM_LogFilter, BmString("FilterChain <") << newChain->Name() << "> read");
		newChain->InstantiateItems( &msg);
		AddItemToList( newChain);
	}
	BM_LOG2( BM_LogFilter, BmString("End of InstantiateItems() for FilterChainList"));
	mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	we just set init-ok
\*------------------------------------------------------------------------------*/
bool BmFilterChainList::StartJob() {
	bool res = inherited::StartJob();
	if (!FindItemByKey( BM_DefaultItemLabel))
		AddItemToList( new BmFilterChain( BM_DefaultItemLabel.String(), this));
	if (!FindItemByKey( BM_DefaultOutItemLabel))
		AddItemToList( new BmFilterChain( BM_DefaultOutItemLabel.String(), this));
	return res;
}

