/*
	TestBeam.cpp
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

#include <OS.h>
#include <stdio.h>
#include <unistd.h>

#include <SemaphoreSyncObject.h>
#include <TestShell.h>

#include "split.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmStorageUtil.h"

#include "TestBeam.h"

#include "Base64DecoderTest.h"
#include "Base64EncoderTest.h"
#include "FoldedLineEncoderTest.h"
#include "MemIoTest.h"
#include "QuotedPrintableDecoderTest.h"
#include "QuotedPrintableEncoderTest.h"
#include "StringTest.h"

//------------------------------------------------------------------------------
BmString AsciiAlphabet[16];
bool HaveTestdata = false;
bool LargeDataMode = false;

static BmApplication* testApp = NULL;

struct ArgsInfo {
	int argc;
	char** argv;
};

//------------------------------------------------------------------------------
BTestShell shell("Beam Testing Framework", new SemaphoreSyncObject);

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
	suite->addTest("BmBase::String", 
						StringTest::suite());
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
	suite->addTest("Encoding::FoldedLineEncoder", 
						FoldedLineEncoderTest::suite());
	suite->addTest("Encoding::QuotedPrintableDecoder", 
						QuotedPrintableDecoderTest::suite());
	suite->addTest("Encoding::QuotedPrintableEncoder", 
						QuotedPrintableEncoderTest::suite());
	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 StartTests( void* args) {
	// create ascii-alphabet:
	for( uint8 h=0; h<16; ++h) {
		for( uint8 l=0; l<16; ++l) {
			if (h || l)
				AsciiAlphabet[h].Append( h*16 + l, 1);
		}
	}
	
	// wait for Beam to be completely up and running:
	while( !testApp || !testApp->IsRunning())
		snooze( 200*1000);

	// change to src-test folder in order to be able to read
	// testdata:
	BmString testPath(testApp->AppPath());
	testPath.Truncate( testPath.FindLast( "/beam/"));
	testPath << "/beam_testdata";
	if (!access( testPath.String(), R_OK)) {
		HaveTestdata = true;
		chdir( testPath.String());
	}

	// we use only statically linked tests since linking each test against
	// Beam_in_Parts.a would yield large binaries for each test, no good!
	shell.AddSuite( CreateBmBaseTestSuite() );
	shell.AddSuite( CreateMailParserTestSuite() );

	BTestShell::SetGlobalShell(&shell);

	// run the tests
	ArgsInfo* argsInfo = static_cast<ArgsInfo*>( args);
	shell.Run( argsInfo->argc, argsInfo->argv);
	
	// done with the tests, now quit the app:
	be_app->PostMessage( B_QUIT_REQUESTED);

	return 0;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int main(int argc, char *argv[]) {

	ArgsInfo argsInfo;
	argsInfo.argc = argc;
	argsInfo.argv = argv;

	testApp = new BmApplication( BM_TEST_APP_SIG);
	
	thread_id tid = spawn_thread( StartTests, "Beam_Test", 
											B_NORMAL_PRIORITY, &argsInfo);
	resume_thread( tid);

	testApp->Run();

	delete testApp;
}

