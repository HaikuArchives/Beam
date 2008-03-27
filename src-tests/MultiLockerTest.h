/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
/*
 * Beam's test-application is based on the OpenBeOS testing framework
 * (which in turn is based on cppunit). Big thanks to everyone involved!
 *
 */


#ifndef _MultiLockerTest_h
#define _MultiLockerTest_h


#include <ThreadedTestCase.h>
#include "BmMultiLocker.h"

class MultiLockerTest : public BThreadedTestCase {
public:
	MultiLockerTest(string name = "");

	static CppUnit::Test* suite();
	
	void MassiveReadLockTest();

	void BasicReadLockTest();
	void BasicWriteLockTest();

	void ReadLockNestTest();
	void WriteLockNestTest();

	void ReadLockPassTest1();
	void ReadLockPassTest2();

	void WriteLockBlockTest1();

	void ReadWriteLockBlockTest1();
	void ReadWriteLockBlockTest2();
	void ReadWriteLockBlockTest3();

	void ExpandReadToWriteLockTest1();
	void ExpandReadToWriteLockTest2();
	void ExpandReadToWriteLockTest3();
	void ExpandReadToWriteLockTest4();

protected:
	bool WaitForVal( int32 val);
	BmMultiLocker mLocker;
	int32 mVal;
};

#endif
