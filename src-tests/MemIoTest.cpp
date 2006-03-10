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

#include "MemIoTest.h"
#include "TestBeam.h"

#include "BmMemIO.h"

// setUp
void
MemIoTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
MemIoTest::tearDown()
{
	inherited::tearDown();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
MemIoTest::StringIBufTest()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
MemIoTest::StringOBufTest()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void CheckRingBuf( BmRingBuf& ringBuf, int32 l, char pf, char pt,
								  char g, int32 l2) {
	CPPUNIT_ASSERT( ringBuf.Length() == l);
	CPPUNIT_ASSERT( ringBuf.PeekFront() == pf);
	CPPUNIT_ASSERT( ringBuf.PeekTail() == pt);
	CPPUNIT_ASSERT( ringBuf.Get() == g);
	CPPUNIT_ASSERT( ringBuf.Length() == l2);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void MemIoTest::RingBufTest() {
	BmRingBuf ringBuf(1);
	// empty run, should yield '\0' / 0:
	NextSubTest();
	CheckRingBuf( ringBuf, 0, '\0', '\0', '\0', 0);

	// add nothing, should still yield '\0' / 0:
	NextSubTest();
	ringBuf << "";
	CheckRingBuf( ringBuf, 0, '\0', '\0', '\0', 0);

	// add a single char:
	NextSubTest();
	ringBuf << 'x';
	CheckRingBuf( ringBuf, 1, 'x', 'x', 'x', 0);

	// add a string:
	NextSubTest();
	ringBuf << "yx";
	CheckRingBuf( ringBuf, 2, 'y', 'x', 'y', 1);
	CheckRingBuf( ringBuf, 1, 'x', 'x', 'x', 0);
	CheckRingBuf( ringBuf, 0, '\0', '\0', '\0', 0);

	// add a BmString:
	NextSubTest();
	ringBuf << BmString("xyz");
	CheckRingBuf( ringBuf, 3, 'x', 'z', 'x', 2);
	ringBuf << BmString("1234");
	CheckRingBuf( ringBuf, 6, 'y', '4', 'y', 5);
	CheckRingBuf( ringBuf, 5, 'z', '4', 'z', 4);
	CheckRingBuf( ringBuf, 4, '1', '4', '1', 3);
	CheckRingBuf( ringBuf, 3, '2', '4', '2', 2);
	CheckRingBuf( ringBuf, 2, '3', '4', '3', 1);
	CheckRingBuf( ringBuf, 1, '4', '4', '4', 0);
	CheckRingBuf( ringBuf, 0, '\0', '\0', '\0', 0);
}
