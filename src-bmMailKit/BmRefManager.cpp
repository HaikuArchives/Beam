/*
	BmRefManager.cpp

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


#include <Alert.h>

#include "BmLogHandler.h"
#include "BmRefManager.h"
#include "BmUtil.h"

BmRefObj::BmProxyMap BmRefObj::nProxyMap;
BLocker* BmRefObj::nGlobalLocker = NULL;

/*------------------------------------------------------------------------------*\
	AddRef()
		-	add one reference to object
\*------------------------------------------------------------------------------*/
void BmRefObj::AddRef() {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked())
		throw BM_runtime_error( "AddRef(): Could not acquire global lock!");
	BmProxy* proxy = GetProxy( ProxyName());
	BM_ASSERT( proxy!=NULL && mRefCount >= 0);
	int32 lastCount = atomic_add( &mRefCount, 1);
	if (lastCount == 0) {
		proxy->ObjectMap.insert( pair<const BmString, BmRefObj*>( RefName(), 
																					 this));
	}
#ifdef BM_REF_DEBUGGING
	// check again to ensure no-one has clobbered with ref-count...
	BM_ASSERT( mRefCount > 0 && mRefCount == lastCount+1);
	BM_LOG2( BM_LogRefCount, 
				BmString("RefManager: reference to <") << typeid(*this).name() 
					<< ":" << RefName() << ":"<<RefPrintHex() 
					<< "> added, ref-count is "<<mRefCount);
#else
	BM_LOG2( BM_LogRefCount, 
				BmString("RefManager: reference to <") << RefName() << ":" 
					<< RefPrintHex()<<"> added, ref-count is "<<mRefCount);
#endif
}

/*------------------------------------------------------------------------------*\
	RenameRef( newName)
		-	changes the name of the ref-obj (actually removing the item from the map
			and then reinserting it under a different name):
\*------------------------------------------------------------------------------*/
void BmRefObj::RenameRef( const char* newName) {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked())
		throw BM_runtime_error( "RenameRef(): Could not acquire global lock!");
	BmProxy* proxy = GetProxy( ProxyName());
	BM_ASSERT( proxy!=NULL && mRefCount >= 0);
#ifdef BM_REF_DEBUGGING
	BM_LOG2( BM_LogRefCount, 
				BmString("RefManager: reference to <") << typeid(*this).name() 
					<< ":" << RefName() << ":" << RefPrintHex() << "> renamed to "
					<< newName);
#else
	BM_LOG2( BM_LogRefCount, 
				BmString("RefManager: reference to <") << RefName() << ":" 
					<< RefPrintHex() << "> renamed to " << newName);
#endif
	// find object...
	BmObjectMap::iterator pos;
	for(	pos = proxy->ObjectMap.find( RefName()); 
			pos != proxy->ObjectMap.end(); ++pos) {
		if (pos->second == this)
			break;
	}
	// ...remove old entry...
	if (pos != proxy->ObjectMap.end())
		proxy->ObjectMap.erase( pos);
	// ...and insert under new name:
	proxy->ObjectMap.insert( pair< const BmString, BmRefObj*>( newName, this));
}

/*------------------------------------------------------------------------------*\
	RemoveRef()
		-	removes one reference from object and deletes the object
			if the new reference count is zero
\*------------------------------------------------------------------------------*/
void BmRefObj::RemoveRef() {
	bool needsDelete = false;
	{	// scope for lock
		BAutolock lock( GlobalLocker());
		if (!lock.IsLocked())
			throw BM_runtime_error( "RemoveRef(): Could not acquire global lock!");
		BmProxy* proxy = GetProxy( ProxyName());
		BM_ASSERT( proxy!=NULL && mRefCount >= 0);

		int32 lastCount = atomic_add( &mRefCount, -1);
	
#ifdef BM_REF_DEBUGGING
		BM_ASSERT( lastCount > 0);
		BM_LOG2( BM_LogRefCount, 
					BmString("RefManager: reference to <") << typeid(*this).name() 
						<< ":" << RefName() << ":" << RefPrintHex()
						<< "> removed, new ref-count is "<<mRefCount);
#else
		BM_LOG2( BM_LogRefCount, 
					BmString("RefManager: reference to <") << RefName() << ":"
						<< RefPrintHex() << "> removed, new ref-count is "
						<< mRefCount);
#endif

		if (lastCount == 1) {
			// removed last reference, so we delete the object:
			BmObjectMap::iterator pos;
			for( 	pos = proxy->ObjectMap.find( RefName()); 
					pos != proxy->ObjectMap.end(); ++pos) {
				if (pos->second == this)
					break;
			}
			if (pos != proxy->ObjectMap.end())
				proxy->ObjectMap.erase( pos);
#ifdef BM_REF_DEBUGGING
			BM_LOG( BM_LogRefCount, 
					  BmString("RefManager: ... object <") << typeid(*this).name() 
					  	<< ":" << RefName() << ":" << RefPrintHex()
					  	<< "> will be deleted");
			// check again to ensure no-one has clobbered with ref-count...
			BM_ASSERT( mRefCount == 0);
			// ...and make sure anyone trying to addref this object will 
			// visit the debugger
			mRefCount = -1;
#else
			BM_LOG2( BM_LogRefCount, 
						BmString("RefManager: ... object <") << RefName() << ":"
							<< RefPrintHex() << "> will be deleted");
#endif
			needsDelete = true;
		}
	}
	if (needsDelete)
		delete this;
}

/*------------------------------------------------------------------------------*\
	GetProxy()
		-	
\*------------------------------------------------------------------------------*/
const char* BmRefObj::ProxyName() const {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked())
		throw BM_runtime_error( "RemoveRef(): Could not acquire global lock!");
	return typeid(*this).name();
}

/*------------------------------------------------------------------------------*\
	GetProxy()
		-	
\*------------------------------------------------------------------------------*/
BmProxy* BmRefObj::GetProxy( const char* const proxyName) {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked())
		throw BM_runtime_error( "GetProxy(): Could not acquire global lock!");
	BmProxyMap::iterator iter = nProxyMap.find( proxyName);
	if (iter == nProxyMap.end())
		return nProxyMap[proxyName] = new BmProxy();
	else
		return iter->second;
}

/*------------------------------------------------------------------------------*\
	GetProxy()
		-	
\*------------------------------------------------------------------------------*/
BLocker* BmRefObj::GlobalLocker() { 
	if (!nGlobalLocker)
		nGlobalLocker = new BLocker("GlobalRefLock");
	return nGlobalLocker;
}


#ifdef BM_REF_DEBUGGING
/*------------------------------------------------------------------------------*\
	PrintRefsLeft()
		-	helper-func that prints all existing references:
\*------------------------------------------------------------------------------*/
void BmRefObj::PrintRefsLeft() {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked())
		throw BM_runtime_error("PrintRefsLeft(): Could not acquire global lock!");
	int32 count = 0;
	BmProxyMap::const_iterator iter;
	try {
		BM_LOG( BM_LogRefCount, 
				  BmString("RefManager: active list\n--------------------"));
		for( iter = nProxyMap.begin(); iter != nProxyMap.end(); ++iter) {
			BmProxy* proxy = iter->second;
			BmObjectMap::const_iterator iter2;
			for(  iter2=proxy->ObjectMap.begin(); 
					iter2 != proxy->ObjectMap.end(); ++iter2, ++count) {
				BmRefObj* ref = iter2->second;
				BM_LOG( BM_LogRefCount, 
						  BmString("\t<") << typeid(*ref).name() << " " 
						  		<< ref->RefName() << ":" << ref->RefPrintHex()
						  		<< "> alive, ref-count is "<<ref->mRefCount);
			}
		}
		BM_LOG( BM_LogRefCount, 
				  BmString("--------------------\n(") 
				  		<< count	<< " refs)\n--------------------");
	} catch( BM_runtime_error &err) {
		BM_SHOWERR( err.what());
	}
	if (count > 0)
		(new BAlert( 
			"", 
			"Reference-debugging showed that there are refs still "
			"alive, check logs.", 
			"OK"
		))->Go();
}
#endif	/* BM_REF_DEBUGGING */


/*------------------------------------------------------------------------------*\
	RefPrintHex()
		-	helper-func that prints this-pointer as hex-number
\*------------------------------------------------------------------------------*/
BmString BmRefObj::RefPrintHex() const { 
	return RefPrintHex( this);	
}

/*------------------------------------------------------------------------------*\
	RefPrintHex()
		-	helper-func that prints this-pointer as hex-number
\*------------------------------------------------------------------------------*/
BmString BmRefObj::RefPrintHex( const void* ptr) { 
	char buf[20]; sprintf( buf, "%p", ptr);	return buf;	
}



/*------------------------------------------------------------------------------*\
	FetchObject()
		-	
\*------------------------------------------------------------------------------*/
BmRefObj* BmProxy::FetchObject( const BmString& key, BmRefObj* ptr) {
	if (BmRefObj::GlobalLocker()->IsLocked()) {
		for(  BmObjectMap::const_iterator pos = ObjectMap.find( key);
				pos!=ObjectMap.end(); ++pos) {
			if (pos->first != key)
				break;
			if (pos->second == ptr || ptr==NULL)
				return pos->second;
		}
		return NULL;
	} else
		BM_SHOWERR("FetchObject(): BmRefObj::GlobalLocker() must be locked!");
	return NULL;
}

// helper function to keep logging out of header-file:
void LogHelper( const BmString& text) {
	BM_LOG2( BM_LogRefCount, text);
}


#ifdef BM_REF_DEBUGGING
/*------------------------------------------------------------------------------*\
	BmAutolockCheckGlobal()
		-	
\*------------------------------------------------------------------------------*/
BmAutolockCheckGlobal::BmAutolockCheckGlobal( BLooper* l) 
	:	mLooper( l)
	,	mLocker( NULL) 
{
	Init();
}

/*------------------------------------------------------------------------------*\
	BmAutolockCheckGlobal()
		-	
\*------------------------------------------------------------------------------*/
BmAutolockCheckGlobal::BmAutolockCheckGlobal( BLocker* l)
	:	mLooper( NULL)
	,	mLocker( l) 
{
	Init();
}

/*------------------------------------------------------------------------------*\
	BmAutolockCheckGlobal()
		-	
\*------------------------------------------------------------------------------*/
BmAutolockCheckGlobal::BmAutolockCheckGlobal( BLocker& l)
	:	mLooper( NULL)
	,	mLocker( &l) 
{
	Init();
}

/*------------------------------------------------------------------------------*\
	~BmAutolockCheckGlobal()
		-	
\*------------------------------------------------------------------------------*/
BmAutolockCheckGlobal::~BmAutolockCheckGlobal() {
	if (mLocker)
		mLocker->Unlock();
	if (mLooper)
		mLooper->Unlock();
}

/*------------------------------------------------------------------------------*\
	IsLocked()
		-	
\*------------------------------------------------------------------------------*/
bool BmAutolockCheckGlobal::IsLocked() { 
	return mLocker && mLocker->IsLocked()
			|| mLooper && mLooper->IsLocked();
}

/*------------------------------------------------------------------------------*\
	Init()
		-	
\*------------------------------------------------------------------------------*/
void BmAutolockCheckGlobal::Init() {
	if (BmRefObj::GlobalLocker()->IsLocked()) {
		DEBUGGER( "GlobalLocker must not be locked when using "
					 "BmAutolockCheckGlobal!");
		mLocker = NULL;
		mLooper = NULL;
	} else {
		if (mLocker)
			mLocker->Lock();
		if (mLooper)
			mLooper->Lock();
	}
}
#endif
