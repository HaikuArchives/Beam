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


#ifndef _SieveTest_h
#define _SieveTest_h

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>
#include <TestCase.h>

class SieveTest : public BTestCase
{
	typedef TestCase inherited;
	CPPUNIT_TEST_SUITE( SieveTest );
	CPPUNIT_TEST( BasicActionTest);
	CPPUNIT_TEST( ExtendedActionTest);
	CPPUNIT_TEST( BasicTestsTest);
	CPPUNIT_TEST( ExistsTestTest);
	CPPUNIT_TEST( HeaderTestTest);
	CPPUNIT_TEST( AddressTestTest);
	CPPUNIT_TEST( RelationalValueTestsTest);
	CPPUNIT_TEST( NumericRelationalValueTestsTest);
	CPPUNIT_TEST( NumericRelationalCountTestsTest);
	CPPUNIT_TEST_SUITE_END();
public:
//	static CppUnit::Test* Suite();
	
	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void BasicActionTest();
	void ExtendedActionTest();
	void BasicTestsTest();
	void AddressTestTest();
	void ExistsTestTest();
	void HeaderTestTest();
	void RelationalValueTestsTest();
	void NumericRelationalValueTestsTest();
	void NumericRelationalCountTestsTest();
};


#endif
