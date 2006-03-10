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

#include <OS.h>
#include <stdio.h>
#include <unistd.h>
#include <iostream>

#ifdef B_BEOS_VERSION_DANO
class BPath;
#endif

#include <SemaphoreSyncObject.h>
#include <TestShell.h>

#include "split.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"

#include "TestBeam.h"

#include "Base64DecoderTest.h"
#include "Base64EncoderTest.h"
#include "BinaryDecoderTest.h"
#include "BinaryEncoderTest.h"
#include "EncodedWordEncoderTest.h"
#include "FoldedLineEncoderTest.h"
#include "LinebreakDecoderTest.h"
#include "LinebreakEncoderTest.h"
#include "MailMonitorTest.h"
#include "MemIoTest.h"
#include "MultiLockerTest.h"
#include "QuotedPrintableDecoderTest.h"
#include "QuotedPrintableEncoderTest.h"
#include "SieveTest.h"
#include "StringTest.h"
#include "Utf8DecoderTest.h"
#include "Utf8EncoderTest.h"

//------------------------------------------------------------------------------
BmString AsciiAlphabet[16];
bool HaveTestdata = false;
bool LargeDataMode = false;

static BmApplication* testApp = NULL;

struct ArgsInfo {
	int argc;
	char** argv;
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void DumpResult( const BmString& str) {
	if (LargeDataMode) {
		cerr << str.String();
	} else {
		vector<BmString> lines = split( "\n", str);
		cerr << "Result:" << endl;
		for( uint32 i=0; i<lines.size(); ++i) {
			lines[i].RemoveSet( "\r");
			cerr << "|" << lines[i].String() << "|" << endl;
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void SlurpFile( const char* filename, BmString& str) {
	if (!FetchFile( filename, str))
		throw CppUnit::Exception( 
			(BmString("couldn't open file ")+filename).String()
		);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BTestSuite* CreateBmBaseTestSuite() {
	BTestSuite *suite = new BTestSuite("BmBase");

	// ##### Add test suites here #####
	suite->addTest("BmBase::MemIo", 
						MemIoTest::suite());
	suite->addTest("BmBase::MultiLocker", 
						MultiLockerTest::suite());
	suite->addTest("BmBase::String", 
						StringTest::suite());
	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BTestSuite* CreateMailTrackerTestSuite() {
	BTestSuite *suite = new BTestSuite("MailTracker");

	// ##### Add test suites here #####
	suite->addTest("MailTracker::MailMonitor", 
						MailMonitorTest::suite());
	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BTestSuite* CreateMailParserTestSuite() {
	BTestSuite *suite = new BTestSuite("MailParser");

	// ##### Add test suites here #####
	suite->addTest("Encoding::Base64Decoder", 
						Base64DecoderTest::suite());
	suite->addTest("Encoding::Base64Encoder", 
						Base64EncoderTest::suite());
	suite->addTest("Encoding::BinaryDecoder", 
						BinaryDecoderTest::suite());
	suite->addTest("Encoding::BinaryEncoder", 
						BinaryEncoderTest::suite());
	suite->addTest("Encoding::EncodedWordEncoder", 
						EncodedWordEncoderTest::suite());
	suite->addTest("Encoding::FoldedLineEncoder", 
						FoldedLineEncoderTest::suite());
	suite->addTest("Encoding::LinebreakDecoder", 
						LinebreakDecoderTest::suite());
	suite->addTest("Encoding::LinebreakEncoder", 
						LinebreakEncoderTest::suite());
	suite->addTest("Encoding::QuotedPrintableDecoder", 
						QuotedPrintableDecoderTest::suite());
	suite->addTest("Encoding::QuotedPrintableEncoder", 
						QuotedPrintableEncoderTest::suite());
	suite->addTest("Encoding::Utf8Decoder", 
						Utf8DecoderTest::suite());
	suite->addTest("Encoding::Utf8Encoder", 
						Utf8EncoderTest::suite());
	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BTestSuite* CreateFilterAddonsTestSuite() {
	BTestSuite *suite = new BTestSuite("FilterAddons");

	// ##### Add test suites here #####
	suite->addTest("FilterAddons::Sieve", 
						SieveTest::suite());
	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 StartTests( void* args) {
	try {
		// wait until app ist initialized:
		testApp->StartupLocker()->Lock();
	
		// create ascii-alphabet:
		for( uint8 h=0; h<16; ++h) {
			for( uint8 l=0; l<16; ++l) {
				if (h || l)
					AsciiAlphabet[h].Append( h*16 + l, 1);
			}
		}
		
		// change to src-test folder in order to be able to read
		// testdata:
		BmString testPath(testApp->AppPath());
		testPath.Truncate( testPath.FindLast( "/beam"));
		testPath << "/beam_testdata";
		if (!access( testPath.String(), R_OK)) {
			HaveTestdata = true;
			chdir( testPath.String());
			// now remove old test-mailbox and replace by freshly unzipped archive:
			static char buf[1024];
			BmString currPath = getcwd( buf, 1024);
			CPPUNIT_ASSERT( currPath.FindFirst( "beam_testdata") >= 0);
									// make sure we have correct path, we *don't* want
									// to make a rm -rf on a wrong path...
			system( "rm -rf >/dev/null mail");
			system( "unzip -o >/dev/null mail.zip");

			// use a different mailbox if in test-mode:
			ThePrefs->SetString( "MailboxPath", testPath+"/mail");
			ThePrefs->SetupMailboxVolume();
		} else {
			fprintf(stderr, "beam_testdata-folder wasn't found, skipping MailMonitor-tests\n");
		}
	
		// allow app to start running...
		testApp->StartupLocker()->Unlock();
		snooze( 200*1000);
		// ...and wait for Beam to be completely up and running:
		testApp->StartupLocker()->Lock();
	
		BTestShell shell("Beam Testing Framework", new SemaphoreSyncObject);
	
		// we use only statically linked tests since linking each test against
		// Beam_in_Parts.a would yield large binaries for each test, no good!
		shell.AddSuite( CreateBmBaseTestSuite() );
		if (HaveTestdata)
			shell.AddSuite( CreateMailTrackerTestSuite() );
		shell.AddSuite( CreateMailParserTestSuite() );
		shell.AddSuite( CreateFilterAddonsTestSuite() );
	
		BTestShell::SetGlobalShell(&shell);

		// set prefs as required by tests:	
		ThePrefs->SetBool( "MakeQPSafeForEBCDIC", false);

		// run the tests
		ArgsInfo* argsInfo = static_cast<ArgsInfo*>( args);
		shell.Run( argsInfo->argc, argsInfo->argv);
		
		// done with the tests, now quit the app:
		testApp->StartupLocker()->Unlock();
		be_app->PostMessage( B_QUIT_REQUESTED);
	} catch( BM_error& e) {
		ShowAlert( e.what());
	}
	return 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int main(int argc, char **argv) {

	ArgsInfo argsInfo;
	argsInfo.argc = argc;
	argsInfo.argv = argv;

	const char* BM_TEST_APP_SIG = "application/x-vnd.zooey-testbeam";

	// first remove any old test-settings, since we want to start afresh:
	system( "rm -rf >/dev/null /boot/home/config/settings/Beam_Test");

	testApp = new BmApplication( BM_TEST_APP_SIG, true);
	
	thread_id tid = spawn_thread( StartTests, "Beam_Test", 
											B_NORMAL_PRIORITY, &argsInfo);
	resume_thread( tid);

	testApp->Run();

	delete testApp;
}

