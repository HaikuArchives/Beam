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

#include <typeinfo>

#include <Autolock.h>
#include <Locker.h>
#include "BmString.h"

#include "BmBasics.h"

#pragma interface

template <class T> class BmRef;
class BmObjectList;
/*------------------------------------------------------------------------------*\
	BmRefObj
		-	an object that can be reference-managed
\*------------------------------------------------------------------------------*/
class BmRefObj {
	
public:
	BmRefObj() : mRefCount(0) 				{}
	virtual ~BmRefObj() 						{}

	// native methods:
	void AddRef();
	void RenameRef( const char* newName);
	void RemoveRef();
	//
	const char* ObjectListName() const;
	static BmRefObj* FetchObject( const char* objListName, 
											const BmString& objName, 
											BmRefObj* ptr = NULL);
	BmString RefPrintHex() const;

	// statics:
	static inline BLocker* GlobalLocker();
	static BmString RefPrintHex( const void* ptr);
	static void CleanupObjectLists();
#ifdef BM_REF_DEBUGGING
	static void PrintRefsLeft();
#endif

private:
	virtual const BmString& RefName() const = 0;

	int32 mRefCount;
	static BLocker* nGlobalLocker;

	// Hide copy-constructor and assignment:
	BmRefObj( const BmRefObj&);
#ifndef __POWERPC__
	BmRefObj operator=( const BmRefObj&);
#endif
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
		*this = NULL;
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
		-	smart-pointer class that implements weak-referencing (via a set 
			of BmRefObj)
		-	a weak reference is not included in reference-counting, but it 
			transparently checks whether the weakly referenced object still 
			exists or not.
\*------------------------------------------------------------------------------*/
// helper function to keep logging out of header-file:
void LogHelper( const BmString& text);

template <class T> class BmWeakRef {

	BmString mName;
	T* mPtr;
	const char* mObjectListName;

public:
	inline BmWeakRef(T* p = 0) 
	:	mName( p ? p->RefName() : BM_DEFAULT_STRING) 
	,	mPtr( p)
	,	mObjectListName( p ? p->ObjectListName() : "") 
	{
		LogHelper( BmString("RefManager: weak-reference to <") << mName 
						<< ":" << BmRefObj::RefPrintHex(mPtr) << "> created");
	}
	inline BmWeakRef<T>& operator= ( T* p) {
		mName = p ? p->RefName() : BM_DEFAULT_STRING;
		mPtr = p;
		mObjectListName = p ? p->ObjectListName() : "";
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
		LogHelper( BmString("RefManager: weak-reference to <") << mName 
						<< ":" << BmRefObj::RefPrintHex(mPtr) << "> dereferenced");
		BAutolock lock( BmRefObj::GlobalLocker());
		return dynamic_cast<T*>( 
			BmRefObj::FetchObject( mObjectListName, mName, mPtr)
		);
	}

private:
};


/*------------------------------------------------------------------------------*\*\
	wrapper around BAutolock that enhances profiling output and debugging
\*------------------------------------------------------------------------------*/
#ifdef BM_REF_DEBUGGING
	// during profiling/debugging we use this:
class BLocker;
class BLooper;
class BmAutolockCheckGlobal {
public:
	BmAutolockCheckGlobal( BLooper* l);
	BmAutolockCheckGlobal( BLocker* l);
	BmAutolockCheckGlobal( BLocker& l);
	~BmAutolockCheckGlobal();
	void Init();
	bool IsLocked();
private:
	BLocker* mLocker;
	BLooper* mLooper;
};
#else
	// otherwise, we use this:
#define BmAutolockCheckGlobal BAutolock
#endif


#endif
