/*
	BmRefManager.cpp
		$Id$
*/

#include "BmLogHandler.h"
#include "BmUtil.h"


BmRefManager* BmRefManager::theInstance = NULL;

/********************************************************************************\
	BmRefManager
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmRefManager* BmRefManager::CreateInstance() {
	if (theInstance)
		return theInstance;
	else
		return theInstance = new BmRefManager();
}

/*------------------------------------------------------------------------------*\
	BmRefManager()
		-	c'tor
\*------------------------------------------------------------------------------*/
BmRefManager::BmRefManager()
	:	mLocker( "RefManager", false)
{
}

/*------------------------------------------------------------------------------*\
	~BmRefManager()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmRefManager::~BmRefManager() {
}


/*------------------------------------------------------------------------------*\
	AddRef( model)
		-	adds a reference to the given model
\*------------------------------------------------------------------------------*/
void BmRefManager::AddRef( BmRefManager* model) {
	assert( model);
	BAutolock lock( mLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( "AddRef(): Unable to get lock");
	int32 numRefs = ++mRefMap[model];
	BM_LOG2( BM_LogUtil, BString("ModelManager: reference to <") << model->ModelName() << "> added, ref-count is "<<numRefs);
}

/*------------------------------------------------------------------------------*\
	AddRef( model)
		-	adds a reference to the given model
\*------------------------------------------------------------------------------*/
void BmRefManager::RemoveRef( BmRefManager* model) {
	assert( model);
	BAutolock lock( mLocker);
	lock.IsLocked()	 					|| BM_THROW_RUNTIME( "RemoveRef(): Unable to get lock");
	int32 numRefs = --mRefMap[model];
	BM_LOG2( BM_LogUtil, BString("ModelManager: reference to <") << model->ModelName() << "> removed, ref-count is " <<numRefs);
	if (numRefs == 0) {
		// removed last reference, so we delete the model:
		mRefMap.erase( model);
		BM_LOG2( BM_LogUtil, BString("ModelManager: ... model will be deleted"));
		delete model;
	} else if (numRefs < 0) {
		// was not referenced at all, we remove ref-item:
		mRefMap.erase( model);
	}
}
