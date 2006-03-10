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

#include "Utf8EncoderTest.h"
#include "TestBeam.h"

#include "BmEncoding.h"

/*
 *
 * Please note that any string-constants in this file are UTF-8, so the
 * decoded string should be in utf-8, too.
 *
 */

static BmString DefaultCharset = "iso-8859-15";

// setUp
void
Utf8EncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
Utf8EncoderTest::tearDown()
{
	inherited::tearDown();
}

static void EncodeUtf8AndCheck( BmString input, BmString result, 
										  BmString srcCharset=DefaultCharset,
										  int32 firstDiscardedPos=-1,
										  bool hasError=false);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeUtf8AndCheck( BmString input, BmString result, 
										  BmString srcCharset, int32 firstDiscardedPos,
										  bool hasError) {
	BmString encodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmUtf8Encoder encoder( &srcBuf, srcCharset, blockSize);
	destBuf.Write( &encoder, blockSize);
	encodedStr.Adopt( destBuf.TheString());
	try {
		CPPUNIT_ASSERT( encodedStr.Compare( result)==0);
		CPPUNIT_ASSERT( firstDiscardedPos!=-1 
								|| encoder.FirstDiscardedPos() == firstDiscardedPos);
		CPPUNIT_ASSERT( !hasError || encoder.HadError());
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
Utf8EncoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	EncodeUtf8AndCheck( "",
							  "");
	// check us-ascii compatible text:
	NextSubTest(); 
	EncodeUtf8AndCheck( "A simple text (which contains only us-ascii chars)",
							  "A simple text (which contains only us-ascii chars)");
	// check encoding of special characters:
	NextSubTest(); 
	EncodeUtf8AndCheck( "\xe4\xf6\xfc\xdf",
							  "äöüß");
	// check encoding of special characters which can't be found in src-charset:
	NextSubTest(); 
	EncodeUtf8AndCheck( 
		"The \xa4-sign is only contained in iso-8859-15",
		"The -sign is only contained in iso-8859-15",
		"us-ascii", 5
	);
	// broken utf-8, bytes missing at end (should be dropped):
	NextSubTest(); 
	EncodeUtf8AndCheck( "text is broken \xFC",
							  "text is broken ", "utf-8", -1, true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Utf8EncoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.utf8_decoded", input);
	BmString result;
	SlurpFile("testdata.utf8_encoded", result);
	// check if encoding of file-contents yields intended result:
	NextSubTest(); 
	EncodeUtf8AndCheck( input, result);
}
