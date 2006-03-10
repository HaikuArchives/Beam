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

#include "LinebreakEncoderTest.h"
#include "TestBeam.h"

#include "BmEncoding.h"

/*
 *
 * Please note that any string-constants in this file are UTF-8, so the
 * decoded string should be in utf-8, too.
 *
 */

// setUp
void
LinebreakEncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
LinebreakEncoderTest::tearDown()
{
	inherited::tearDown();
}

static void EncodeLinebreaksAndCheck( BmString input, BmString result);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeLinebreaksAndCheck( BmString input, BmString result) {
	BmString encodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmLinebreakEncoder encoder( &srcBuf, blockSize);
	destBuf.Write( &encoder, blockSize);
	encodedStr.Adopt( destBuf.TheString());
	try {
		CPPUNIT_ASSERT( encodedStr.Compare( result)==0);
	} catch( ...) {
		DumpResult( encodedStr);
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
LinebreakEncoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	EncodeLinebreaksAndCheck( "",
									  "");
	// run with nothing to do:
	NextSubTest(); 
	EncodeLinebreaksAndCheck( "A simple text",
									  "A simple text");
	// check everything in one go:
	NextSubTest(); 
	EncodeLinebreaksAndCheck( 
		"\nA simple text\n (which contains \rsome linebreaks)\n\n",
		"\r\nA simple text\r\n (which contains some linebreaks)\r\n\r\n"
	);
}
