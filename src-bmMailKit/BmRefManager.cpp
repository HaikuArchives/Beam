/*
	BmRefManager.cpp

		$Id$
*/

#include "BmRefManager.h"

/*------------------------------------------------------------------------------*\
	RefPrintHex()
		-	helper-func that prints this-pointer as hex-number
\*------------------------------------------------------------------------------*/
BString BmRefObj::RefPrintHex() { 
	char buf[20]; sprintf( buf, "%p", this);	return buf;	
}

/*------------------------------------------------------------------------------*\
	AddRef()
		-	add one reference to object
\*------------------------------------------------------------------------------*/
void BmRefObj::AddRef() {
	++mRefCount;
	BM_LOG3( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> added, ref-count is "<<mRefCount);
}

/*------------------------------------------------------------------------------*\
	RemoveRef()
		-	removes one reference from object and deletes the object
			if the new reference count is zero
\*------------------------------------------------------------------------------*/
void BmRefObj::RemoveRef() {
	int32 currRefCount = --mRefCount;
	BM_LOG3( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> removed, ref-count is "<<currRefCount);
	if (currRefCount == 0) {
		// removed last reference, so we delete the object:
		BM_LOG3( BM_LogUtil, BString("RefManager: ... object will be deleted"));
		delete this;
	}
}


