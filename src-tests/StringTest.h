/*
	StringTest.h
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


#ifndef _StringTest_h
#define _StringTest_h

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>
#include <TestCase.h>

class StringTest : public BTestCase
{
	typedef TestCase inherited;
	CPPUNIT_TEST_SUITE( StringTest );
	CPPUNIT_TEST( StringSubCopyTest);
	CPPUNIT_TEST( StringSearchTest);
	CPPUNIT_TEST( StringReplaceTest);
	CPPUNIT_TEST( StringRemoveTest);
	CPPUNIT_TEST( StringPrependTest);
	CPPUNIT_TEST( StringInsertTest);
	CPPUNIT_TEST( StringFormatAppendTest);
	CPPUNIT_TEST( StringEscapeTest);
	CPPUNIT_TEST( StringConstructionTest);
	CPPUNIT_TEST( StringCompareTest);
	CPPUNIT_TEST( StringCharAccessTest);
	CPPUNIT_TEST( StringCaseTest);
	CPPUNIT_TEST( StringAssignTest);
	CPPUNIT_TEST( StringAccessTest);
	CPPUNIT_TEST( StringAppendTest);
	CPPUNIT_TEST( StringBeamExtensionsTest);
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
	void StringSubCopyTest();
	void StringSearchTest();
	void StringReplaceTest();
	void StringRemoveTest();
	void StringPrependTest();
	void StringInsertTest();
	void StringFormatAppendTest();
	void StringEscapeTest();
	void StringConstructionTest();
	void StringCompareTest();
	void StringCharAccessTest();
	void StringCaseTest();
	void StringAssignTest();
	void StringAccessTest();
	void StringAppendTest();
	void StringBeamExtensionsTest();
};


#endif
