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
