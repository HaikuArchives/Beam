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

#include <algorithm>

#include <Message.h>

#include "BmBasics.h"
#include "BmPrefs.h"
#include "BmFilter.h"
#include "BmFilterChain.h"
#include "BmLogHandler.h"
#include "BmRosterBase.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/********************************************************************************\
	BmChainedFilter
\********************************************************************************/

const char* const BmChainedFilter::MSG_POSITION = 	"bm:pos";
const char* const BmChainedFilter::MSG_FILTERNAME = "bm:fname";

const int16 BmChainedFilter::nArchiveVersion = 2;

BmString BM_OutboundLabel("<outbound>");

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

static const int32 BM_UNDEFINED_POSITION = 9999;	
							// a number larger than the number of chained filters
							// so that this filter is appended to the list

/*------------------------------------------------------------------------------*\
	BmChainedFilter()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( const char* name, BmChainedFilterList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mPosition( BM_UNDEFINED_POSITION)
{
	if (model)
		model->RenumberPos();
}

/*------------------------------------------------------------------------------*\
	BmChainedFilter( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( BMessage* archive, BmChainedFilterList* model)
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
	BmChainedFilterList
\********************************************************************************/

const int16 BmChainedFilterList::nArchiveVersion = 1;

const char* const BmChainedFilterList::MSG_NAME = "bm:name";
/*------------------------------------------------------------------------------*\
	BmChainedFilterList()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmChainedFilterList::BmChainedFilterList( const char* name) 
	:	inherited( name)
{
	// tell filter-list that our items have a foreign-key on its items:
	TheFilterList->AddForeignKey( BmChainedFilter::MSG_FILTERNAME,	this);
}

/*------------------------------------------------------------------------------*\
	~BmChainedFilterList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmChainedFilterList::~BmChainedFilterList() {
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmChainedFilterList::ForeignKeyChanged( const BmString& key, 
															const BmString& oldVal, 
															const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ) {
		BmChainedFilter* filter 
			= dynamic_cast< BmChainedFilter*>( iter++->second.Get());
							// need iter++ here because RenameItem will destroy iter!
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
struct LessPos {
	bool operator() ( const BmChainedFilter* left, 
							const BmChainedFilter* right) {
		return left->Position() < right->Position();
	}
};
void BmChainedFilterList::RenumberPos() {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	// first fill pos-vect from chained filters:
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmChainedFilter* filter 
			= dynamic_cast< BmChainedFilter*>( iter->second.Get());
		mPosVect.push_back(filter);
	}
	sort(mPosVect.begin(), mPosVect.end(), LessPos());
	// now renumber all elements in mPosVect:
	int32 currPos = 1;
	BmFilterPosVect::iterator posIter;
	for( posIter = mPosVect.begin(); posIter != mPosVect.end(); ++posIter) {
		BmChainedFilter* filter = *posIter;
		if (filter->Position() != currPos)
			filter->Position( currPos);
		currPos += 2;
	}
}

/*------------------------------------------------------------------------------*\
	AddItemToList( item)
		-	extends normal behaviour with renumbering of item-positions
\*------------------------------------------------------------------------------*/
bool BmChainedFilterList::AddItemToList( BmListModelItem* item, 
													  BmListModelItem* parent) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	bool res = inherited::AddItemToList( item, parent);
	BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( item);
	if (filter->Position() == BM_UNDEFINED_POSITION)
		RenumberPos();
	return res;
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList( item)
		-	extends normal behaviour with renumbering of item-positions
\*------------------------------------------------------------------------------*/
void BmChainedFilterList::RemoveItemFromList( BmListModelItem* item) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	inherited::RemoveItemFromList( item);
	RenumberPos();
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	returns the name of the settings-file for the filterchain-list
\*------------------------------------------------------------------------------*/
const BmString BmChainedFilterList::SettingsFileName() {
	// should never be called, since a filter-chain lives in the settings file of
	// the filterchain-list
	BM_ASSERT( 0);
	return "";
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	we just set init-ok
\*------------------------------------------------------------------------------*/
bool BmChainedFilterList::StartJob() {
	mInitCheck = B_OK;
	return true;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmChainedFilterList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogFilter, 
				BmString("Start of InstantiateItems() for ChainedFilterList") 
					<< ModelName());
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		if ((err = archive->FindMessage( 
			BmListModelItem::MSG_CHILDREN, i, &msg)
		) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not find item nr. ") << i+1 
										<< " \n\nError:" << strerror(err));
		BmChainedFilter* newFilter = new BmChainedFilter( &msg, this);
		BM_LOG3( BM_LogFilter, 
					BmString("ChainedFilter <") << newFilter->Key() << "> read");
		AddItemToList( newFilter);
	}
	BM_LOG2( BM_LogFilter, 
				BmString("End of InstantiateItems() for FilterChain") 
					<< ModelName());
	RenumberPos();
	mInitCheck = B_OK;
}








/********************************************************************************\
	BmFilterChain
\********************************************************************************/

const int16 BmFilterChain::nArchiveVersion = 3;

const char* const BmFilterChain::MSG_NAME = "bm:name";

/*------------------------------------------------------------------------------*\
	BmFilterChain()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmFilterChain::BmFilterChain( const char* name, BmFilterChainList* model) 
	:	inherited( name, model, (BmListModelItem*)NULL)
	,	mChainedFilters( new BmChainedFilterList( name))
{
}

/*------------------------------------------------------------------------------*\
	BmFilterChain( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmFilterChain::BmFilterChain( BMessage* archive, BmFilterChainList* model) 
	:	inherited( FindMsgString( archive, MSG_NAME), 
					  model, (BmListModelItem*)NULL)
	,	mChainedFilters( new BmChainedFilterList( FindMsgString( archive, 
																					MSG_NAME)))
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	if (version == 2) {
		// rename <outbound-pre-send> back to <outbound> (ahem):
		if (Key() == "<outbound-pre-send>")
			Key(BM_OutboundLabel);
	}
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
	status_t ret = (mChainedFilters->Archive( archive, deep)
		||	archive->AddString( MSG_NAME, Key().String()));
	return ret;
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
	return BmString( BeamRoster->SettingsPath()) << "/FilterChains";
}

/*------------------------------------------------------------------------------*\
	ForeignKeyChanged( keyName, oldVal, newVal)
		-	updates the specified foreign-key with the given new value
\*------------------------------------------------------------------------------*/
void BmFilterChainList::ForeignKeyChanged( const BmString& key, 
														 const BmString& oldVal, 
														 const BmString& newVal) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	if (key == BmChainedFilter::MSG_FILTERNAME) {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); iter != end(); ++iter) {
			BmFilterChain* chain = dynamic_cast< BmFilterChain*>( iter->second.Get());
			chain->ChainedFilters()->RenameItem( oldVal, newVal);
		}
	}
}

/*------------------------------------------------------------------------------*\
	RemoveFilterFromAllChains( filterName)
		-	removes filter with given name from all filter-chains.
\*------------------------------------------------------------------------------*/
void BmFilterChainList::RemoveFilterFromAllChains( const BmString& filterName) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ": Unable to get lock"
		);
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		BmFilterChain* chain = dynamic_cast< BmFilterChain*>( iter->second.Get());
		chain->ChainedFilters()->RemoveItemByKey( filterName);
	}
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	initializes the filterchain-list from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterChainList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogFilter, BmString("Start of InstantiateItems() for FilterChainList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		if ((err = archive->FindMessage( 
			BmListModelItem::MSG_CHILDREN, i, &msg)
		) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not find item nr. ") << i+1 
										<< " \n\nError:" << strerror(err));
		BmFilterChain* newChain = new BmFilterChain( &msg, this);
		BM_LOG3( BM_LogFilter, BmString("FilterChain <") << newChain->Name() << "> read");
		newChain->ChainedFilters()->InstantiateItems( &msg);
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
	RemoveItemByKey( "<outbound-pre-edit>");
	if (!FindItemByKey( BM_OutboundLabel))
		AddItemToList( new BmFilterChain( BM_OutboundLabel.String(), this));
	return res;
}

