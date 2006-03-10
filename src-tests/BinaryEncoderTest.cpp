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

#include "BinaryEncoderTest.h"
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
BinaryEncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
BinaryEncoderTest::tearDown()
{
	inherited::tearDown();
}

static void EncodeAndCheck( BmString input, BmString result);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeAndCheck( BmString input, BmString result) {
	BmString encodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmBinaryEncoder encoder( &srcBuf, blockSize);
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
BinaryEncoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	EncodeAndCheck( "",
						 "");
	// binary decoding is just a copy, so there's really nothing to do...
	NextSubTest(); 
	EncodeAndCheck( "A simple text",
						 "A simple text");
	// check handling of embedded 0-bytes:
	NextSubTest(); 
	BmString input("abcdefghijklmnopqrstuvwxyz");
	int32 len = input.Length();
	char* p = input.LockBuffer( -1);
	p[10] = '\0';
	p[20] = '\0';
	input.UnlockBuffer( len);
	BmString output( input);
	EncodeAndCheck( input,
						 output);
}
