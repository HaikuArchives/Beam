/*
	BmRefManager.cpp

		$Id$
*/

#include "BmRefManager.h"

#ifdef BM_REF_DEBUGGING
set<BmRefObj*> BM_REF_LIST;

void BM_PrintRefsLeft() {
	uint32 count = BM_REF_LIST.size();
	BM_LOG2( BM_LogUtil, BString("RefManager: active list (")<<count<<" refs)\n--------------------");
	set<BmRefObj*>::const_iterator iter;
	for( iter=BM_REF_LIST.begin(); iter != BM_REF_LIST.end(); ++iter) {
		BmRefObj* ref = *iter;
		BM_LOG2( BM_LogUtil, BString("\t") << ref->RefName() << ":"<<ref->RefPrintHex()<<"> added, ref-count is "<<ref->mRefCount);
	}
	BM_LOG2( BM_LogUtil, BString("--------------------"));
}
#endif

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
#ifdef BM_REF_DEBUGGING
	if (mRefCount == 1)
		BM_REF_LIST.insert( this);
#endif
	BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> added, ref-count is "<<mRefCount);
}

/*------------------------------------------------------------------------------*\
	RemoveRef()
		-	removes one reference from object and deletes the object
			if the new reference count is zero
\*------------------------------------------------------------------------------*/
void BmRefObj::RemoveRef() {
	int32 currRefCount = --mRefCount;
	BM_LOG2( BM_LogUtil, BString("RefManager: reference to <") << RefName() << ":"<<RefPrintHex()<<"> removed, ref-count is "<<currRefCount);
	if (currRefCount == 0) {
#ifdef BM_REF_DEBUGGING
		BM_REF_LIST.erase( this);
#endif
		// removed last reference, so we delete the object:
		BM_LOG( BM_LogUtil, BString("RefManager: ... object <") << RefName() << ":"<<RefPrintHex()<<"> will be deleted");
		delete this;
	}
}


