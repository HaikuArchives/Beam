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


#ifndef _MailMonitorTest_h
#define _MailMonitorTest_h

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/extensions/HelperMacros.h>
#include <ThreadedTestCase.h>

class MailMonitorTest : public BThreadedTestCase
{
	typedef BThreadedTestCase inherited;
public:
	
	static CppUnit::Test* suite();

	MailMonitorTest();
	virtual ~MailMonitorTest();

	// This function called before *each* test added in Suite()
	void setUp();
	
	// This function called after *each* test added in Suite()
	void tearDown();

	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void BasicMailRefTest();
	void BasicMailFolderTest();
	void MassiveMailRefCreator();
	void MassiveMailRefRemover();
	void MassiveMailRefCheckerTest();
	void SpecialMailSetter();
	void SpecialMailClearer();
	void SpecialMailCheckerTest();
	void RefListAdder();
	void RefListRemover();
	void RefListStorageTest();
private:
	void SyncWithMailMonitor();
};


#endif
