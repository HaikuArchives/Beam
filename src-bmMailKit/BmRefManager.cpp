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

#include "BmRefManager.h"

BmRefObj::BmProxyMap BmRefObj::nProxyMap;
BLocker* BmRefObj::nGlobalLocker = NULL;

/*------------------------------------------------------------------------------*\
	AddRef()
		-	add one reference to object
\*------------------------------------------------------------------------------*/
void BmRefObj::AddRef() {
	int32 lastCount = atomic_add( &mRefCount, 1);
	if (lastCount == 0) {
		BmProxy* proxy = GetProxy( ProxyName());
		if (proxy) {
			BAutolock lock( &proxy->Locker);
			if (!lock.IsLocked())
				throw BM_runtime_error(BmString("AddRef(): Proxy ")<<ProxyName()<<" could not be locked");
			proxy->ObjectMap.insert( make_pair( RefName(), this));
		} else
			BM_SHOWERR(BmString("AddRef(): Proxy ")<<ProxyName()<<" could not be created");
	}
#ifdef BM_REF_DEBUGGING
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << typeid(*this).name() << ":" << RefName() << ":"<<RefPrintHex()<<"> added, ref-count is "<<lastCount+1);
#else
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> added, ref-count is "<<lastCount+1);
#endif
}

/*------------------------------------------------------------------------------*\
	RenameRef( newName)
		-	changes the name of the ref-obj (actually removing the item from the map
			and then reinsert it under different name:
\*------------------------------------------------------------------------------*/
void BmRefObj::RenameRef( const char* newName) {
#ifdef BM_REF_DEBUGGING
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << typeid(*this).name() << ":" << RefName() << ":"<<RefPrintHex()<<"> renamed to "<<newName);
#else
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> renamed to "<<newName);
#endif
	BmProxy* proxy = GetProxy( ProxyName());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		if (!lock.IsLocked())
			throw BM_runtime_error(BmString("RenameRef(): Proxy ")<<ProxyName()<<" could not be locked");
		BmObjectMap::iterator pos;
		// find object...
		for( pos = proxy->ObjectMap.find( RefName()); pos != proxy->ObjectMap.end(); ++pos) {
			if (pos->second == this)
				break;
		}
		// ...remove old entry...
		if (pos != proxy->ObjectMap.end())
			proxy->ObjectMap.erase( pos);
		// ...and insert under new name:
		proxy->ObjectMap.insert( make_pair( newName, this));
	} else
		BM_SHOWERR(BmString("RemoveRef(): Proxy ")<<ProxyName()<<" not found");
}

/*------------------------------------------------------------------------------*\
	RemoveRef()
		-	removes one reference from object and deletes the object
			if the new reference count is zero
\*------------------------------------------------------------------------------*/
void BmRefObj::RemoveRef() {
	int32 lastCount = atomic_add( &mRefCount, -1);
#ifdef BM_REF_DEBUGGING
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << typeid(*this).name() << ":" << RefName() << ":"<<RefPrintHex()<<"> removed, ref-count is "<<lastCount-1);
#else
	BM_LOG2( BM_LogUtil, BmString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> removed, ref-count is "<<lastCount-1);
#endif
	if (lastCount == 1) {
		// removed last reference, so we delete the object:
		BmProxy* proxy = GetProxy( ProxyName());
		if (proxy) {
			BAutolock lock( &proxy->Locker);
			if (!lock.IsLocked())
				throw BM_runtime_error(BmString("RemoveRef(): Proxy ")<<ProxyName()<<" could not be locked");
			BmObjectMap::iterator pos;
			for( pos = proxy->ObjectMap.find( RefName()); pos != proxy->ObjectMap.end(); ++pos) {
				if (pos->second == this)
					break;
			}
			if (pos != proxy->ObjectMap.end())
				proxy->ObjectMap.erase( pos);
#ifdef BM_REF_DEBUGGING
			BM_LOG( BM_LogUtil, BmString("RefManager: ... object <") << typeid(*this).name() << ":" << RefName() << ":"<<RefPrintHex()<<"> will be deleted");
#else
			BM_LOG2( BM_LogUtil, BmString("RefManager: ... object <") << RefName() << ":"<<RefPrintHex()<<"> will be deleted");
#endif
			delete this;
		} else
			BM_SHOWERR(BmString("RemoveRef(): Proxy ")<<ProxyName()<<" not found");
	}
}

/*------------------------------------------------------------------------------*\
	GetProxy()
		-	
\*------------------------------------------------------------------------------*/
BmProxy* BmRefObj::GetProxy( const char* const proxyName) {
	BAutolock lock( GlobalLocker());
	BmProxyMap::iterator iter = nProxyMap.find( proxyName);
	if (iter == nProxyMap.end())
		return nProxyMap[proxyName] = new BmProxy( proxyName);
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
	int32 count = 0;
	BmProxyMap::const_iterator iter;
	try {
		BAutolock lock( GlobalLocker());
		if (!lock.IsLocked())
			throw BM_runtime_error("PrintRefsLeft(): Could not acquire global lock");
		BM_LOG( BM_LogUtil, BmString("RefManager: active list\n--------------------"));
		for( iter = nProxyMap.begin(); iter != nProxyMap.end(); ++iter) {
			BmProxy* proxy = iter->second;
			BAutolock lock( &proxy->Locker);
			if (!lock.IsLocked())
				throw BM_runtime_error(BmString("PrintRefsLeft(): Proxy ")<<iter->first<<" could not be locked");
			BmObjectMap::const_iterator iter2;
			for( iter2=proxy->ObjectMap.begin(); iter2 != proxy->ObjectMap.end(); ++iter2, ++count) {
				BmRefObj* ref = iter2->second;
				BM_LOG( BM_LogUtil, BmString("\t<") << typeid(*ref).name() << " " << ref->RefName() << ":"<<ref->RefPrintHex()<<"> alive, ref-count is "<<ref->RefCount());
			}
		}
		BM_LOG( BM_LogUtil, BmString("--------------------\n(")<<count<<" refs)\n--------------------");
	} catch( BM_runtime_error &err) {
		BM_SHOWERR( err.what());
	}
	if (count > 0)
		(new BAlert( "", "Reference-debugging showed that there are refs still alive, check logs.", "OK"))->Go();
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
BmRefObj* BmProxy::FetchObject( const BmString& key, void* ptr) {
	if (Locker.IsLocked()) {
		for(  BmObjectMap::const_iterator pos = ObjectMap.find( key);
				pos!=ObjectMap.end(); ++pos) {
			if (pos->first != key)
				break;
			if (pos->second == ptr || ptr==NULL)
				return pos->second;
		}
		return NULL;
	} else
		BM_SHOWERR("FetchObject(): Proxy must be locked!");
	return NULL;
}

