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

const int16 BmChainedFilter::nArchiveVersion = 1;

BmString BM_DefaultOutItemLabel("<outbound>");

// standard logfile-name for this class:
#undef BM_LOGNAME
#define BM_LOGNAME "Filter"

/*------------------------------------------------------------------------------*\
	BmChainedFilter()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( const char* name, BmFilterChain* model) 
	:	inherited( BmString()<<model->NextPosition(), model, (BmListModelItem*)NULL)
	,	mFilterName( name)
{
}

/*------------------------------------------------------------------------------*\
	BmChainedFilter( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmChainedFilter::BmChainedFilter( BMessage* archive, BmFilterChain* model) 
	:	inherited( BmString()<<FindMsgInt32( archive, MSG_POSITION), model, (BmListModelItem*)NULL)
{
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 0;
	mFilterName = FindMsgString( archive, MSG_FILTERNAME);
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
		||	archive->AddString( MSG_FILTERNAME, mFilterName.String())
		||	archive->AddInt32( MSG_POSITION, atoi(Key().String())));
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
}

/*------------------------------------------------------------------------------*\
	BmFilterChain( archive)
		-	c'tor
		-	constructs a BmFilterChain from a BMessage
\*------------------------------------------------------------------------------*/
BmFilterChain::BmFilterChain( BMessage* archive, BmFilterChainList* model) 
	:	inheritedItem( FindMsgString( archive, MSG_NAME), model, (BmListModelItem*)NULL)
	,	inheritedList( FindMsgString( archive, MSG_NAME))
{
	int16 version;
	if (archive->FindInt16( inheritedItem::MSG_VERSION, &version) != B_OK)
		version = 0;
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
	if (ret == B_OK) {
		BmAutolockCheckGlobal lock( ModelLocker());
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
		BMessage msg;
		BmModelItemMap::const_iterator iter;
		for( iter = inheritedList::begin(); iter != inheritedList::end(); ++iter) {
			BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( iter->second.Get());
			if ((ret = filter->Archive( &msg, true)) != B_OK)
				break;
			archive->AddMessage( MSG_CHILDREN, &msg); 
		}
		archive->AddInt32( MSG_NUMCHILDREN, inheritedList::size()); 
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	MoveUp()
		-	
		-	
\*------------------------------------------------------------------------------*/
void BmFilterChain::MoveUp( int32 oldPos) {
	if (oldPos <= 1)
		return;
	BmString oldKey = BmString() << oldPos;
	BmString newKey = BmString() << oldPos-1;
	BmString tmpKey( "tmp");
	RenameItem( oldKey, tmpKey);
	RenameItem( newKey, oldKey);
	RenameItem( tmpKey, newKey);
}

/*------------------------------------------------------------------------------*\
	MoveDown()
		-	
		-	
\*------------------------------------------------------------------------------*/
void BmFilterChain::MoveDown( int32 oldPos) {
	if (oldPos >= (int32)inheritedList::size())
		return;
	BmString oldKey = BmString() << oldPos;
	BmString newKey = BmString() << oldPos+1;
	BmString tmpKey( "tmp");
	RenameItem( oldKey, tmpKey);
	RenameItem( newKey, oldKey);
	RenameItem( tmpKey, newKey);
}

/*------------------------------------------------------------------------------*\
	NextPosition()
		-	
		-	
\*------------------------------------------------------------------------------*/
int32 BmFilterChain::NextPosition() {
	BmAutolockCheckGlobal lock( ModelLocker());
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
	BmModelItemMap::const_iterator iter;
	int32 highestPos = 0;
	for( iter = inheritedList::begin(); iter != inheritedList::end(); ++iter) {
		BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( iter->second.Get());
		if (filter->Position() > highestPos)
			highestPos = filter->Position();
	}
	return highestPos+1;
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList( item)
		-	extends normal behaviour with renumbering of item-positions
\*------------------------------------------------------------------------------*/
void BmFilterChain::RemoveItemFromList( BmListModelItem* item) {
	BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( item);
	if (filter) {
		BmAutolockCheckGlobal lock( mModelLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( ModelNameNC() << ":RemoveItemFromList(): Unable to get lock");
		int32 removedPos = filter->Position();
		inheritedList::RemoveItemFromList( filter);
		BmModelItemMap::const_iterator iter;
		for( iter = inheritedList::begin(); iter != inheritedList::end(); ) {
			BmChainedFilter* filter = dynamic_cast< BmChainedFilter*>( iter++->second.Get());
			if (filter->Position() > removedPos)
				RenameItem( BmString()<<filter->Position(), BmString()<<(filter->Position()-1));
		}
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
	BM_LOG2( BM_LogUtil, BmString("Start of InstantiateItems() for FilterChain") << Key());
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find item nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmChainedFilter* newFilter = new BmChainedFilter( &msg, this);
		BM_LOG3( BM_LogUtil, BmString("ChainedFilter <") << newFilter->Key() << "> read");
		AddItemToList( newFilter);
	}
	BM_LOG2( BM_LogUtil, BmString("End of InstantiateItems() for FilterChain") << Key());
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
	InstantiateItems( archive)
		-	initializes the signature-list from the given archive
\*------------------------------------------------------------------------------*/
void BmFilterChainList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogUtil, BmString("Start of InstantiateItems() for FilterChainList"));
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find item nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmFilterChain* newChain = new BmFilterChain( &msg, this);
		BM_LOG3( BM_LogUtil, BmString("FilterChain <") << newChain->Name() << "> read");
		newChain->InstantiateItems( &msg);
		AddItemToList( newChain);
	}
	BM_LOG2( BM_LogUtil, BmString("End of InstantiateItems() for FilterChainList"));
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

