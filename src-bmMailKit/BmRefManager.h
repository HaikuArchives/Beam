/*
	BmRefManager.h
		$Id$
*/

#ifndef _BmRefManager_h
#define _BmRefManager_h

#include <stdio.h>

//#include <typeinfo>

#include <String.h>

#include "BmBasics.h"
#include "BmLogHandler.h"


/*------------------------------------------------------------------------------*\
	BmRefObj
		-	an object that can be reference-managed
\*------------------------------------------------------------------------------*/
class BmRefObj {

public:
	BmRefObj() : mRefCount(0) 				{}
	virtual ~BmRefObj() 						{}

	virtual const BString& RefName() const = 0;

	// native methods:
	void AddRef();
	void RemoveRef();
	BString RefPrintHex();

private:
	int32 mRefCount;
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
	void AddRef(T* p) const   				{ if (p)	p->AddRef(); }
	void RemoveRef(T* p) const 			{ if (p)	p->RemoveRef(); }

};


#endif
