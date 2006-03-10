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

#include "LinebreakDecoderTest.h"
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
LinebreakDecoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
LinebreakDecoderTest::tearDown()
{
	inherited::tearDown();
}

static void DecodeLinebreaksAndCheck( BmString input, BmString result);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void DecodeLinebreaksAndCheck( BmString input, BmString result) {
	BmString decodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmLinebreakDecoder decoder( &srcBuf, blockSize);
	destBuf.Write( &decoder, blockSize);
	decodedStr.Adopt( destBuf.TheString());
	try {
		CPPUNIT_ASSERT( decodedStr.Compare( result)==0);
	} catch( ...) {
		DumpResult( decodedStr);
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
LinebreakDecoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	DecodeLinebreaksAndCheck( "",
									  "");
	// run with nothing to do:
	NextSubTest(); 
	DecodeLinebreaksAndCheck( "A simple text",
									  "A simple text");
	// check everything in one go:
	NextSubTest(); 
	DecodeLinebreaksAndCheck( 
		"\nA simple text\r\n (which contains \rsome linebreaks)\r\n\r\n",
		"\nA simple text\n (which contains some linebreaks)\n\n"
	);
}
