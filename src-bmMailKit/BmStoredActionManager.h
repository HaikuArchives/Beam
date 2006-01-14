/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmStoredActionManager_h
#define _BmStoredActionManager_h

#include "BmMailKit.h"

#include <set>
#include <vector>

#include "BmRefManager.h"

class BmListModel;
/*------------------------------------------------------------------------------*\
	BmStoredActionFlusher
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmStoredActionFlusher {

public:
	static BmStoredActionFlusher* CreateInstance();

	void Run();
	void Quit();
	//
	void AddList( BmRef<BmListModel> list);

	static BmStoredActionFlusher* theInstance;
private:
	//	native methods:
	BmStoredActionFlusher();
	void _Loop();
	void _FlushList( BmRef<BmListModel>& list);
	//
	static int32 _ThreadEntry(void* data);

	typedef set< BmRef< BmListModel> > ListSet;
	ListSet mListSet;

	BLocker mLocker;
	bool mShouldRun;
	thread_id mThreadId;

	// Hide copy-constructor and assignment:
	BmStoredActionFlusher( const BmStoredActionFlusher&);
	BmStoredActionFlusher operator=( const BmStoredActionFlusher&);
};

#define TheStoredActionFlusher BmStoredActionFlusher::theInstance

/*------------------------------------------------------------------------------*\
	BmStoredActionManager
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmStoredActionManager {
	typedef vector<BMessage*> ActionVect;
public:
	BmStoredActionManager(BmListModel* list);
	~BmStoredActionManager();
	//
	bool StoreAction(BMessage* action);
	bool Flush();
	//
	void MaxCacheSize(uint32 maxCacheSize)
													{ mMaxCacheSize = maxCacheSize; }
private:
	ActionVect mActionVect;
	BmListModel* mList;
	uint32 mMaxCacheSize;
};
	
#endif
