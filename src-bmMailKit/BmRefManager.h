/*
	BmRefManager.h
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


#ifndef _BmRefManager_h
#define _BmRefManager_h

#include <stdio.h>

#include <map>
#include <typeinfo>

#include <Autolock.h>
#include <Locker.h>
#include "BmString.h"

#include "BmBasics.h"
#include "BmLogHandler.h"

template <class T> class BmRef;
class BmProxy;
/*------------------------------------------------------------------------------*\
	BmRefObj
		-	an object that can be reference-managed
\*------------------------------------------------------------------------------*/
class BmRefObj {
	typedef map<BmString,BmProxy*> BmProxyMap;

public:
	BmRefObj() : mRefCount(0) 				{}
	virtual ~BmRefObj() 						{}

	virtual const BmString& RefName() const = 0;
	inline const char* ProxyName() const	{ return typeid(*this).name(); }

	// native methods:
#ifdef BM_REF_DEBUGGING
	virtual void AddRef();
	virtual void RenameRef( const char* newName);
	virtual void RemoveRef();
#else
	void AddRef();
	void RenameRef( const char* newName);
	void RemoveRef();
#endif // BM_REF_DEBUGGING

	BmString RefPrintHex() const;
	static BmString RefPrintHex( const void* ptr);
	
	// getters:
	inline int32 RefCount() const			{ return mRefCount; }

	static inline BLocker* GlobalLocker();
	
	// statics:
	static BmProxy* GetProxy( const char* const proxyName);
#ifdef BM_REF_DEBUGGING
	static void PrintRefsLeft();
#endif

private:
	int32 mRefCount;
	static BmProxyMap nProxyMap;
	static BLocker* nGlobalLocker;

	// Hide copy-constructor and assignment:
	BmRefObj( const BmRefObj&);
#ifndef __POWERPC__
	BmRefObj operator=( const BmRefObj&);
#endif
};



typedef multimap<BmString,BmRefObj*> BmObjectMap;
/*------------------------------------------------------------------------------*\
	BmProxy
		-	an object that manages all instances of a specific class
\*------------------------------------------------------------------------------*/
class BmProxy {

public:
	inline BmProxy( BmString name) {}
	BmObjectMap ObjectMap;
	BmRefObj* FetchObject( const BmString& key, void* ptr=NULL);
};



/*------------------------------------------------------------------------------*\
	BmRef
		-	smart-pointer class that implements reference-counting (via BmRefObj)
\*------------------------------------------------------------------------------*/
template <class T> class BmRef {

	T* mPtr;

public:
	inline BmRef(T* p = 0) : mPtr( p) {
		AddRef( mPtr);
	}
	inline BmRef( const BmRef<T>& ref) : mPtr( ref.Get()) {
		AddRef( mPtr);
	}
	inline ~BmRef() {
		RemoveRef( mPtr);
	}
	inline BmRef<T>& operator= ( const BmRef<T>& ref) {
		if (mPtr != ref.Get()) {
			// in order to behave correctly when being called recursively,
			// we set new value before deleting old, so that a recursive call
			// will skip this block (because of the condition above).
			T* old = mPtr;
			mPtr = ref.Get();
			RemoveRef( old);
			AddRef( mPtr);
		}
		return *this;
	}
	inline BmRef<T>& operator= ( T* p) {
		if (mPtr != p) {
			// in order to behave correctly when being called recursively,
			// we set new value before deleting old, so that a recursive call
			// will skip this block (because of the condition above).
			T* old = mPtr;
			mPtr = p;
			RemoveRef( old);
			AddRef( mPtr);
		}
		return *this;
	}
	inline bool operator== ( const BmRef<T>& ref) {
		return mPtr == ref.Get();
	}
	inline bool operator== ( const T* p) {
		return mPtr == p;
	}
	inline bool operator!= ( const T* p) {
		return mPtr != p;
	}
	inline void Clear() {
		if (mPtr) {
			// in order to behave correctly when being called recursively,
			// we set new value before deleting old, so that a recursive call
			// will skip this block (because of the condition above).
			T* p = mPtr;
			mPtr = NULL;
			RemoveRef( p);
		}
	}
	inline T* operator->() const   		{ return mPtr; }
	inline T* Get() const 					{ return mPtr; }
	inline operator bool() const 			{ return mPtr!=NULL; }

private:
	inline void AddRef(T* p) const   	{ if (p)	p->AddRef(); }
	inline void RemoveRef(T* p) const 	{ if (p)	p->RemoveRef(); }
};



/*------------------------------------------------------------------------------*\
	BmWeakRef
		-	smart-pointer class that implements weak-referencing (via a set of BmRefObj)
		-	a weak reference is not included in reference-counting, but it transparently
			checks whether the weakly referenced object still exists or not.
\*------------------------------------------------------------------------------*/
template <class T> class BmWeakRef {

	BmString mName;
	T* mPtr;
	const char* mProxyName;

public:
	inline BmWeakRef(T* p = 0) 
	:	mName( p ? p->RefName() : BM_DEFAULT_STRING) 
	,	mPtr( p)
	,	mProxyName( p ? p->ProxyName() : "") 
	{
		BM_LOG2( BM_LogUtil, BmString("RefManager: weak-reference to <") << mName << ":" << BmRefObj::RefPrintHex(mPtr) << "> created");
	}
	inline BmWeakRef<T>& operator= ( T* p) {
		mName = p ? p->RefName() : BM_DEFAULT_STRING;
		mPtr = p;
		mProxyName = p ? p->ProxyName() : "";
		return *this;
	}
	inline bool operator== ( const T* p) {
		return (p == mPtr) && (p ? p->RefName() == mName : false);
	}
	inline bool operator!= ( const T* p) {
		return (p != mPtr) || (p ? p->RefName() != mName : true);
	}
	inline operator bool() const 			{ return Get(); }
	inline BmRef<T> Get() const 			{
		BM_LOG2( BM_LogUtil, BmString("RefManager: weak-reference to <") << mName << ":" << BmRefObj::RefPrintHex(mPtr) << "> dereferenced");
		BAutolock lock( BmRefObj::GlobalLocker());
		BmProxy* proxy = BmRefObj::GetProxy( mProxyName);
		if (proxy) {
			return static_cast<T*>(proxy->FetchObject( mName, mPtr));
		} else 
			return NULL;
	}

private:
};



#endif
