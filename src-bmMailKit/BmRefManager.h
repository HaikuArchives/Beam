/*
	BmRefManager.h
		$Id$
*/

#ifndef _BmRefManager_h
#define _BmRefManager_h

#include <stdio.h>

#include <map>
#include <typeinfo>

#include <String.h>

#include "BmBasics.h"
#include "BmLogHandler.h"


/*------------------------------------------------------------------------------*\
	BmRefObj
		-	an object that can be reference-managed
\*------------------------------------------------------------------------------*/
class BmRefObj {
public:
	virtual const BString& RefName() const = 0;
};

/*------------------------------------------------------------------------------*\
	BmRefManager
		-	manages all instances of objects (esp their deletion)
\*------------------------------------------------------------------------------*/
template <class T> class BmRefManager {
	typedef map< T*, int32> BmRefMap;

public:
	// c'tors & d'tor:
	BmRefManager( const char* id)
		:	mLocker( id, false)
		,	mID( id)								{ }
	
	// helper that prints a pointer as hex-number
	BString Hex( T* ptr) { char buf[20]; sprintf( buf, "%p", ptr);	return buf;	}

	// native methods:
	void AddRef( T* ptr) {
		assert( ptr);
		BmAutolock lock( mLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "AddRef(): Unable to get lock");
#ifdef BM_LOGGING
		int32 numRefs = ++mRefMap[ptr];
		BM_LOG3( BM_LogUtil, BString("RefManager: reference to <") << ptr->RefName() << ":"<<Hex(ptr)<<"> added, ref-count is "<<numRefs);
#else
		++mRefMap[ptr];
#endif
	}

	void RemoveRef( T* ptr) {
		assert( ptr);
		BmAutolock lock( mLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "RemoveRef(): Unable to get lock");
		int32 numRefs = --mRefMap[ptr];
#ifdef BM_LOGGING
		BM_LOG3( BM_LogUtil, BString("RefManager: reference to <") << ptr->RefName() << ":"<<Hex(ptr)<<"> removed, ref-count is "<<numRefs);
#endif
		if (numRefs == 0) {
			// removed last reference, so we delete the object:
			mRefMap.erase( ptr);
#ifdef BM_LOGGING
			BM_LOG3( BM_LogUtil, BString("RefManager: ... object will be deleted"));
#endif
			delete ptr;
		} else if (numRefs < 0) {
			// was not referenced at all, we remove ref-item:
			mRefMap.erase( ptr);
			// M.B.: we refrain from deleting the object, possibly leaking memory, but
			//			this seems a safer bet then to do a delete on an object which is
			//			not referenced at all.
		}
	}
	
	void PrintStatistics() {
#ifdef BM_LOGGING
		BmAutolock lock( mLocker);
		lock.IsLocked()	 					|| BM_THROW_RUNTIME( "RemoveRef(): Unable to get lock");
		BmRefMap::const_iterator iter;
		BString s = BString(mID) << " statistics:\n";
		for( iter = mRefMap.begin(); iter != mRefMap.end(); ++iter) {
			T* ptr = iter->first;
			int32 numRefs = iter->second;
			s << "RefManager: reference to <" << ptr->RefName() << ":"<<Hex(ptr)<<"> ref-count is "<<numRefs<<"\n";
		}
		BM_LOG( BM_LogUtil, s);
#endif
	}

private:
	BmRefMap mRefMap;
	BLocker mLocker;
	BString mID;
};



/*------------------------------------------------------------------------------*\
	BmRef
		-	smart-pointer class that implements reference-counting (via BmRefManager)
\*------------------------------------------------------------------------------*/
template <class T> class BmRef {
private:
	T* mPtr;

public:
	BmRef(T* p = 0) : mPtr( p) {
		AddRef( mPtr);
	}
	BmRef( const BmRef<T>& ref) : mPtr( ref.Get()) {
		AddRef( mPtr);
	}
	~BmRef() {
		RemoveRef( mPtr);
	}
	BmRef<T>& operator= ( const BmRef<T>& ref) {
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
	BmRef<T>& operator= ( T* p) {
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
	bool operator== ( const BmRef<T>& ref) {
		return mPtr == ref.Get();
	}
	void Clear() {
		if (mPtr) {
			// in order to behave correctly when being called recursively,
			// we set new value before deleting old, so that a recursive call
			// will skip this block (because of the condition above).
			T* p = mPtr;
			mPtr = NULL;
			RemoveRef( p);
		}
	}
	T* operator->() const   				{ return mPtr; }
	T* Get() const 							{ return mPtr; }
	operator bool() const 					{ return mPtr!=NULL; }
private:
	void AddRef(T* p) const   				{ if (p)	T::RefManager.AddRef( p); }
	void RemoveRef(T* p) const 			{ if (p)	T::RefManager.RemoveRef( p); }

};


#endif
