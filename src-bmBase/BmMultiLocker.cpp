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

#include <cstdio>

#include "BmMultiLocker.h"

static const int32 MaxReaders = 1000000;

struct Spinlock
{
	Spinlock(int32& guardVar);
	~Spinlock();
	int32& mGuardVar;
	static const int32 SpinlockTime = 1000;
};

Spinlock::Spinlock(int32& guardVar) 
	:	mGuardVar(guardVar)
{
	while(atomic_or(&mGuardVar, 1) > 0) {
		snooze( SpinlockTime);
	}
}

Spinlock::~Spinlock() 
{
	atomic_and(&mGuardVar, 0xffffffff);
}



BmMultiLocker::BmMultiLocker( const BmString& name)
	:	mThreadMapGuard( 0)
	,	mReaderCount( 0)
	,	mReaderSem(create_sem(0, (name+"_R").String()))
	,	mWriteLocker((name+"_W").String(), true)
{
}


BmMultiLocker::~BmMultiLocker()
{
	delete_sem(mReaderSem);
}

bool 
BmMultiLocker::ReadLock()
{
	bool locked = true;

	// increment and retrieve the current count of readers
	int32 current_count = AddReader();
	if (current_count < 0) {
		// a writer holds the lock, maybe it's our own thread?
		if (!mWriteLocker.IsLocked()) {
			// no, it's someone else, so we wait for mReadSem to be released
			status_t status;
			do {
				status = acquire_sem_etc( mReaderSem, 1, 0, B_INFINITE_TIMEOUT);
			} while( status == B_INTERRUPTED);
			locked = (status == B_OK);
		}
	}

	return locked;	
}

bool 
BmMultiLocker::WriteLock()
{
	bool locked = true;
	
	// wait for other writers to yield...
	mWriteLocker.Lock();
	// ok, now we are the next writer

	// decrement mReaderCount by a very large number
	// this will cause new readers to block on mReadSem
	int32 readers = atomic_add(&mReaderCount, -MaxReaders);
	if (readers > 0)
		// take our own thread out, as we don't want to wait on
		// ourselves (->deadlock!).
		readers -= NestCount();
	if (readers > 0) {
		// foreign readers hold the lock, so we wait for mReadSem to be 
		// released until no more readers are left
		status_t status;
		do {
			status = acquire_sem_etc( mReaderSem, readers, 0, B_INFINITE_TIMEOUT);
		} while( status == B_INTERRUPTED);
		locked = (status == B_OK);
	}

	return locked;
}

void 
BmMultiLocker::ReadUnlock()
{
	// decrement and retrieve the read counter
	int32 current_count = RemoveReader();
	if (current_count < 0) {
		// a writer is waiting for the lock so release mReaderSem
		release_sem_etc(mReaderSem, 1, B_DO_NOT_RESCHEDULE);
	}
}

void 
BmMultiLocker::WriteUnlock()
{
	if (mWriteLocker.IsLocked()) {
		// increment mReadCount by a large number
		// this will let new readers acquire the read lock
		// retrieve the number of current waiters
		int32 readersWaiting 
			= atomic_add(&mReaderCount, MaxReaders) + MaxReaders;

		if (readersWaiting)
			// take our own thread out, as we don't want to release for
			// ourselves yet (->lockleak!).
			readersWaiting -= NestCount();
			
		if (readersWaiting > 0) {
			// readers are waiting for the lock - release mReaderSem
			release_sem_etc( mReaderSem, readersWaiting, B_DO_NOT_RESCHEDULE);
		}
			
		mWriteLocker.Unlock();
	} else 
		debugger("Non-writer attempting to WriteUnlock()\n");
}

bool 
BmMultiLocker::IsWriteLocked() const
{
	return mWriteLocker.IsLocked();
}

bool 
BmMultiLocker::IsReadLocked() const
{
	Spinlock splock(mThreadMapGuard);
	return mThreadMap.find(find_thread(NULL)) != mThreadMap.end();
}

int32 
BmMultiLocker::AddReader()
{
	Spinlock splock(mThreadMapGuard);
	int32 count = atomic_add(&mReaderCount, 1);
	mThreadMap[find_thread(NULL)]++;
	return count;
}

int32 
BmMultiLocker::RemoveReader()
{
	Spinlock splock(mThreadMapGuard);
	int32 count = atomic_add(&mReaderCount, -1);
	thread_id thisThread = find_thread(NULL);
	ThreadMap::iterator pos = mThreadMap.find(thisThread);
	if (pos == mThreadMap.end())
		debugger("RemoveReader() called for thread that has no lock!");
	if (pos->second-- == 1)
		mThreadMap.erase(thisThread);
	return count;
}

int32 
BmMultiLocker::NestCount()
{
	Spinlock splock(mThreadMapGuard);
	ThreadMap::iterator pos = mThreadMap.find(find_thread(NULL));
	int32 result = (pos == mThreadMap.end()) ? 0 : pos->second;
	return result;
}

