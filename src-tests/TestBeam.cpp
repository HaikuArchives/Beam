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

#include <SemaphoreSyncObject.h>
#include <TestShell.h>

#include <OS.h>

#include "BmApp.h"

static BmApplication* testApp = NULL;

struct ArgsInfo {
	int argc;
	char** argv;
};

#include "QuotedPrintableDecoderTest.h"
#include "QuotedPrintableEncoderTest.h"

//------------------------------------------------------------------------------
BTestShell shell("Beam Testing Framework", new SemaphoreSyncObject);

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BTestSuite* CreateMailParserTestSuite() {
	BTestSuite *suite = new BTestSuite("MailParser");

	// ##### Add test suites here #####
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
	// wait for Beam to be completely up and running:
	while( !testApp || !testApp->IsRunning())
		snooze( 200*1000);

	// we use only statically linked tests since linking each test against
	// Beam_in_Parts.a would yield large binaries for each test, no good!
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

