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

/*------------------------------------------------------------------------------*\
	AddRef()
		-	add one reference to object
\*------------------------------------------------------------------------------*/
void BmRefObj::AddRef() {
	BmProxy* proxy = GetProxy( ProxyName());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		if (!lock.IsLocked())
			return;
		++mRefCount;
		if (mRefCount == 1)
			proxy->ObjectMap[RefName()] = this;
		BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> added, ref-count is "<<mRefCount);
	} else
		BM_SHOWERR(BString("AddRef(): Proxy ")<<ProxyName()<<" could not be created");
}

/*------------------------------------------------------------------------------*\
	RemoveRef()
		-	removes one reference from object and deletes the object
			if the new reference count is zero
\*------------------------------------------------------------------------------*/
void BmRefObj::RemoveRef() {
	BmProxy* proxy = GetProxy( ProxyName());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		int32 currRefCount = --mRefCount;
		BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> removed, ref-count is "<<currRefCount);
		if (currRefCount == 0) {
			proxy->ObjectMap.erase( RefName());
			// removed last reference, so we delete the object:
#ifdef BM_REF_DEBUGGING
			BM_LOG( BM_LogUtil, BString("RefManager: ... object <") << typeid(*this).name() << ":" << RefName() << ":"<<RefPrintHex()<<"> will be deleted");
#else
			BM_LOG( BM_LogUtil, BString("RefManager: ... object <") << RefName() << ":"<<RefPrintHex()<<"> will be deleted");
#endif
			delete this;
		}
	} else
		BM_SHOWERR(BString("RemoveRef(): Proxy ")<<ProxyName()<<" not found");
}

/*------------------------------------------------------------------------------*\
	GetProxy()
		-	
\*------------------------------------------------------------------------------*/
BmProxy* BmRefObj::GetProxy( const char* const proxyName) {
	BmProxyMap::iterator iter = nProxyMap.find( proxyName);
	if (iter == nProxyMap.end())
		return nProxyMap[proxyName] = new BmProxy( proxyName);
	else
		return iter->second;
}

#ifdef BM_REF_DEBUGGING
/*------------------------------------------------------------------------------*\
	PrintRefsLeft()
		-	helper-func that prints all existing references:
\*------------------------------------------------------------------------------*/
void BmRefObj::PrintRefsLeft() {
	int32 count = 0;
	BmProxyMap::const_iterator iter;
	BM_LOG2( BM_LogUtil, BString("RefManager: active list\n--------------------"));
	for( iter = nProxyMap.begin(); iter != nProxyMap.end(); ++iter) {
		BmProxy* proxy = iter->second;
		BAutolock lock( &proxy->Locker);
		BmObjectMap::const_iterator iter2;
		for( iter2=proxy->ObjectMap.begin(); iter2 != proxy->ObjectMap.end(); ++iter2) {
			BmRefObj* ref = iter2->second;
			BM_LOG2( BM_LogUtil, BString("\t<") << typeid(*ref).name() << " " << ref->RefName() << ":"<<ref->RefPrintHex()<<"> alive, ref-count is "<<ref->RefCount());
		}
	}
	BM_LOG2( BM_LogUtil, BString("--------------------\n(")<<count<<" refs)\n--------------------");
	if (count > 0)
		(new BAlert( "", "Reference-debugging showed that there are refs still alive, check logs.", "OK"))->Go();
}
#endif	/* BM_REF_DEBUGGING */


/*------------------------------------------------------------------------------*\
	RefPrintHex()
		-	helper-func that prints this-pointer as hex-number
\*------------------------------------------------------------------------------*/
BString BmRefObj::RefPrintHex() const { 
	char buf[20]; sprintf( buf, "%p", this);	return buf;	
}



/*------------------------------------------------------------------------------*\
	FetchObject()
		-	
\*------------------------------------------------------------------------------*/
BmRefObj* BmProxy::FetchObject( const BString& key) {
	if (Locker.IsLocked()) {
		BmObjectMap::const_iterator iter = ObjectMap.find( key);
		return (iter != ObjectMap.end() ? iter->second : NULL);
	} else
		BM_SHOWERR("FetchObject(): Proxy must be locked!");
	return NULL;
}

