/*
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
/*
 * Beam's test-application is based on the OpenBeOS testing framework
 * (which in turn is based on cppunit). Big thanks to everyone involved!
 *
 */

#include "MultiLockerTest.h"
#include <ThreadedTestCaller.h>
#include <cppunit/Test.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>

MultiLockerTest::MultiLockerTest(std::string name)
	: BThreadedTestCase(name)
//	, mLocker( "lock")
	, mVal( 0)
{
}

CppUnit::Test*
MultiLockerTest::suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("MultiLockerSuite");
	BThreadedTestCaller<MultiLockerTest> *caller;
	MultiLockerTest *test;
	
	// massively parallel read-lock test:
	test = new MultiLockerTest;
	caller = new BThreadedTestCaller<MultiLockerTest>(
		"MultiLockerTests::MassiveReadLockTest", test
	);
	caller->addThread("t1", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t2", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t3", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t4", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t5", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t6", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t7", &MultiLockerTest::MassiveReadLockTest);
	caller->addThread("t8", &MultiLockerTest::MassiveReadLockTest);
	suite->addTest(caller);
	
#if 0
	// simple test for read-locking:
	suite->addTest(new CppUnit::TestCaller<MultiLockerTest>(
		"MultiLockerTests::BasicReadLockTest", 
		&MultiLockerTest::BasicReadLockTest
	));

	// simple test for write-locking:
	suite->addTest(new CppUnit::TestCaller<MultiLockerTest>(
		"MultiLockerTests::BasicWriteLockTest", 
		&MultiLockerTest::BasicWriteLockTest
	));

	// test for read-lock nesting:
	suite->addTest(new CppUnit::TestCaller<MultiLockerTest>(
		"MultiLockerTests::ReadLockNestTest", 
		&MultiLockerTest::ReadLockNestTest
	));

	// test for write-lock nesting:
	suite->addTest(new CppUnit::TestCaller<MultiLockerTest>(
		"MultiLockerTests::WriteLockNestTest", 
		&MultiLockerTest::WriteLockNestTest
	));

	// read-locks can pass one another:
	test = new MultiLockerTest;
	caller = new BThreadedTestCaller<MultiLockerTest>(
		"MultiLockerTests::ReadLockPassTest", test
	);
	caller->addThread("t1", &MultiLockerTest::ReadLockPassTest1);
	caller->addThread("t2", &MultiLockerTest::ReadLockPassTest2);
	suite->addTest(caller);
	
	// write-locks must block each other:
	test = new MultiLockerTest;
	caller = new BThreadedTestCaller<MultiLockerTest>(
		"MultiLockerTests::WriteLockBlockTest", test
	);
	caller->addThread("t1", &MultiLockerTest::WriteLockBlockTest1);
	caller->addThread("t2", &MultiLockerTest::WriteLockBlockTest1);
	caller->addThread("t3", &MultiLockerTest::WriteLockBlockTest1);
	caller->addThread("t4", &MultiLockerTest::WriteLockBlockTest1);
	caller->addThread("t5", &MultiLockerTest::WriteLockBlockTest1);
	suite->addTest(caller);
	
	// read- and write-locks must cross-block each other:
	test = new MultiLockerTest;
	caller = new BThreadedTestCaller<MultiLockerTest>(
		"MultiLockerTests::ReadWriteLockBlockTest", test
	);
	caller->addThread("t1", &MultiLockerTest::ReadWriteLockBlockTest1);
	caller->addThread("t2", &MultiLockerTest::ReadWriteLockBlockTest2);
	caller->addThread("t3", &MultiLockerTest::ReadWriteLockBlockTest3);
	caller->addThread("t4", &MultiLockerTest::ReadWriteLockBlockTest3);
	caller->addThread("t5", &MultiLockerTest::ReadWriteLockBlockTest3);
	suite->addTest(caller);
	
	// expanding a read-lock to a write-locks must cross-block all readers:
	test = new MultiLockerTest;
	caller = new BThreadedTestCaller<MultiLockerTest>(
		"MultiLockerTests::ExpandReadToWriteLockTest", test
	);
	caller->addThread("t1", &MultiLockerTest::ExpandReadToWriteLockTest1);
	caller->addThread("t2", &MultiLockerTest::ExpandReadToWriteLockTest2);
	caller->addThread("t3", &MultiLockerTest::ExpandReadToWriteLockTest3);
	caller->addThread("t4", &MultiLockerTest::ExpandReadToWriteLockTest4);
	suite->addTest(caller);
#endif	
	return suite;
}

void
MultiLockerTest::MassiveReadLockTest() {
	NextSubTest();
	CPPUNIT_ASSERT( true);
//	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	for( int i=0; i<10000; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( true);
//		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		CPPUNIT_ASSERT( true);
//		CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
	}
	for( int i=0; i<10000; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( true);
//		CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
//		mLocker.ReadUnlock();
	}
	NextSubTest();
	CPPUNIT_ASSERT( true);
//	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
}	

/*
void
MultiLockerTest::BasicReadLockTest() {
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
	
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.ReadLock() == true);
	
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);

	mLocker.ReadUnlock();
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
}

void
MultiLockerTest::BasicWriteLockTest() {
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
	
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.WriteLock() == true);
	
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == true);

	mLocker.WriteUnlock();
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
}

void
MultiLockerTest::ReadLockNestTest() {
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
	}
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
		mLocker.ReadUnlock();
	}
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
}	

void
MultiLockerTest::WriteLockNestTest() {
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.WriteLock() == true);
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsWriteLocked() == true);
	}
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsWriteLocked() == true);
		mLocker.WriteUnlock();
	}
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
}	

bool
MultiLockerTest::WaitForVal( int32 val) {
	for( int32 c = 10000; mVal != val && c>0; --c)
		snooze(1000);
	return mVal == val;
}

void
MultiLockerTest::ReadLockPassTest1() {
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.ReadLock() == true);
	mVal++;
	NextSubTest();
	CPPUNIT_ASSERT( WaitForVal(2));
	mVal--;
	mLocker.ReadUnlock();
	NextSubTest();
	CPPUNIT_ASSERT( !mLocker.IsReadLocked());
}

void
MultiLockerTest::ReadLockPassTest2() {
	NextSubTest();
	CPPUNIT_ASSERT( WaitForVal(1));
	NextSubTest();
	CPPUNIT_ASSERT( !mLocker.IsReadLocked());
	NextSubTest();
	CPPUNIT_ASSERT( mVal == 1);
	NextSubTest();
	CPPUNIT_ASSERT( mLocker.ReadLock() == true);
	mVal++;
	NextSubTest();
	CPPUNIT_ASSERT( mVal > 1);
	NextSubTest();
	CPPUNIT_ASSERT( WaitForVal(1));
	mLocker.ReadUnlock();
	NextSubTest();
	CPPUNIT_ASSERT( !mLocker.IsReadLocked());
}

void
MultiLockerTest::WriteLockBlockTest1() {
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.WriteLock() == true);
		CPPUNIT_ASSERT( mVal == 0);
		mVal++;
		snooze(1000);
		mVal--;
		mLocker.WriteUnlock();
	}
}

void
MultiLockerTest::ReadWriteLockBlockTest1() {
	mVal = 1;
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		snooze(5000);
		mVal = 2;
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
		mLocker.ReadUnlock();
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsWriteLocked() == false);
		NextSubTest();
		CPPUNIT_ASSERT( WaitForVal(3));
	}
	mVal = 0;
}

void
MultiLockerTest::ReadWriteLockBlockTest2() {
	NextSubTest();
	CPPUNIT_ASSERT( WaitForVal(1));
	for( int i=0; i<10; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.WriteLock() == true);
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsReadLocked() == false);
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.IsWriteLocked() == true);
		mVal = 3;
		snooze(5000);
		mVal = 2;
		mLocker.WriteUnlock();
	}
	mVal = 0;
}

void
MultiLockerTest::ReadWriteLockBlockTest3() {
	CPPUNIT_ASSERT( WaitForVal(1));
	while( mVal != 0) {
		NextSubTest();
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		CPPUNIT_ASSERT( mVal == 1 || mVal == 2 || mVal == 0);
		mLocker.ReadUnlock();
	}
}

void
MultiLockerTest::ExpandReadToWriteLockTest1() {
	NextSubTest();
//	CPPUNIT_ASSERT( mLocker.ReadLock() == true);
	mVal = 1;
	NextSubTest();
//	CPPUNIT_ASSERT( WaitForVal(4));
	try {
		for( int i=0; i<1000; ++i) {
//			CPPUNIT_ASSERT( mLocker.WriteLock() == true);
			NextSubTest();
//			CPPUNIT_ASSERT( mLocker.IsReadLocked() == true);
//			CPPUNIT_ASSERT( mLocker.IsWriteLocked() == true);
//			CPPUNIT_ASSERT( mVal == 4);
//			mLocker.WriteUnlock();
			snooze(1000);
		}
		mVal = -100;
	}
	catch(...) {
		mVal = -100;
		throw;
	}
}

void
MultiLockerTest::ExpandReadToWriteLockTest2() {
	CPPUNIT_ASSERT( WaitForVal(1));
	mVal++;
	snooze(5000);
	while( mVal > 0) {
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		atomic_add( &mVal, 1);
		snooze(5000);
		atomic_add( &mVal, -1);
		mLocker.ReadUnlock();
	}
}

void
MultiLockerTest::ExpandReadToWriteLockTest3() {
	CPPUNIT_ASSERT( WaitForVal(2));
	mVal++;
	snooze(5000);
	while( mVal > 0) {
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		atomic_add( &mVal, 1);
		snooze(500);
		atomic_add( &mVal, -1);
		mLocker.ReadUnlock();
	}
}

void
MultiLockerTest::ExpandReadToWriteLockTest4() {
	CPPUNIT_ASSERT( WaitForVal(3));
	mVal++;
	snooze(5000);
	while( mVal > 0) {
		CPPUNIT_ASSERT( mLocker.ReadLock() == true);
		NextSubTest();
		atomic_add( &mVal, 1);
		atomic_add( &mVal, -1);
		mLocker.ReadUnlock();
	}
}
*/
