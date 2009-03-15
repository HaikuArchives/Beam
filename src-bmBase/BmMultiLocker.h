/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
/* 
 * multiple-reader single-writer locking class,
 * inspired by BMultiLocker, which is
 *    Copyright 1999, Be Incorporated.   All Rights Reserved.
 */

#ifndef BM_MULTI_LOCKER_H
#define BM_MULTI_LOCKER_H

#include <OS.h>

#include <map>

#include <Locker.h>

#include "BmBase.h"
#include "BmString.h"

using std::map;

class IMPEXPBMBASE BmMultiLocker
{
	typedef map<thread_id,int32> ThreadMap;

public:
	BmMultiLocker( const BmString& name);
	virtual ~BmMultiLocker();
		
	// locking for reading or writing
	bool ReadLock();
	bool WriteLock();

	// unlocking after reading or writing
	void ReadUnlock();
	void WriteUnlock();

	// does the current thread hold a write lock ?
	bool IsWriteLocked() const;

	// does the current thread hold a read lock ?
	bool IsReadLocked() const;

private:
	int32 AddReader();
	int32 RemoveReader();
	int32 NestCount();

	// we keep a map of reading threads and their nesting count
	mutable int32 mThreadMapGuard;
	ThreadMap mThreadMap;

	// number of readers
	int32 mReaderCount;
	// readers block on mReaderSem when a writer holds the lock
	sem_id mReaderSem;

	// writers block on mWriteLocker when readers hold the lock
	BLocker mWriteLocker;
};

#endif
