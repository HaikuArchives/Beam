/*
	BmMailRefList.h
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
