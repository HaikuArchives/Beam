/*
	BmRefManager.h
		$Id$
*/

#ifndef _BmRefManager_h
#define _BmRefManager_h

#include <stdio.h>

#include <map>
#include <typeinfo>

#include <Locker.h>
#include <String.h>

#include "BmLogHandler.h"


/*------------------------------------------------------------------------------*\
	BmRefManager
		-	manages all instances of objects (esp their deletion)
\*------------------------------------------------------------------------------*/
template <class T> class BmRefManager {
	typedef map< T*, int32> BmRefMap;

public:
	// c'tors & d'tor:
	BmRefManager()
		:	mLocker( "RefManager", false)	{}
	
	// native methods:
	void AddRef( T* ptr) {
		assert( ptr);
		BAutolock lock( mLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "AddRef(): Unable to get lock");
#ifdef BM_LOGGING
		int32 numRefs = ++mRefMap[ptr];
		BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << ptr->Name() << "> added, ref-count is "<<numRefs);
#else
		++mRefMap[ptr];
#endif
	}
	void RemoveRef( T* ptr) {
		assert( ptr);
		BAutolock lock( mLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "RemoveRef(): Unable to get lock");
		int32 numRefs = --mRefMap[ptr];
		BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << ptr->Name() << "> removed, ref-count is "<<numRefs);
		if (numRefs == 0) {
			// removed last reference, so we delete the object:
			mRefMap.erase( ptr);
			BM_LOG2( BM_LogUtil, BString("RefManager: ... object will be deleted"));
			delete ptr;
		} else if (numRefs < 0) {
			// was not referenced at all, we remove ref-item:
			mRefMap.erase( ptr);
			// M.B.: we refrain from deleting the object, possibly leaking memory, but
			//			this seems a safer bet then to do a delete on an object which is
			//			not referenced at all.
		}
	}
	
private:
	BmRefMap mRefMap;
	BLocker mLocker;
};



/*------------------------------------------------------------------------------*\
	BmRef
		-	smart-pointer class that implements reference-counting (via BmRefManager)
\*------------------------------------------------------------------------------*/
template <class T> class BmRef {
private:
	T* mPtr;

public:
	explicit BmRef(T* p = 0) : mPtr( p) {
		AddRef();
	}
	BmRef( BmRef& r) : mPtr( r.Get()) {
		AddRef();
	}
	~BmRef() {
		RemoveRef();
	}
	BmRef<T>& operator= ( T* p) {
		if (mPtr != p) {
			RemoveRef();
			mPtr = p;
			AddRef();
		}
		return *this;
	}
	T* operator->() const   				{ return mPtr; }
	T* Get() const 							{ return mPtr; }
	operator bool() const 					{ return mPtr!=NULL; }
private:
	void AddRef() const   					{ if (mPtr)	T::RefManager.AddRef( mPtr); }
	void RemoveRef() const 					{ if (mPtr)	T::RefManager.RemoveRef( mPtr); }

};


#endif
