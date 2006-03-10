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

#include "Base64EncoderTest.h"
#include "TestBeam.h"

#include "BmEncoding.h"

/*
 *
 * Please note that any string-constants in this file are UTF-8, so the
 * encoded string should be in utf-8, too.
 *
 */

// setUp
void
Base64EncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
Base64EncoderTest::tearDown()
{
	inherited::tearDown();
}

static bool SingleLineMode = false;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeBase64AndCheck( BmString input, BmString result)
{
	BmString encodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmBase64Encoder encoder( 
		&srcBuf, blockSize,
		SingleLineMode
			? BmBase64Encoder::nTagOnSingleLine
			: BM_DEFAULT_STRING
		);
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
Base64EncoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	EncodeBase64AndCheck( "",
								 "");
	// check 1 byte:
	NextSubTest(); 
	EncodeBase64AndCheck( "a",
								 "YQ==");
	// check 2 bytes:
	NextSubTest(); 
	EncodeBase64AndCheck( "ab",
								 "YWI=");
	// check 3 bytes:
	NextSubTest(); 
	EncodeBase64AndCheck( "abc",
								 "YWJj");
	// check 4 bytes:
	NextSubTest(); 
	EncodeBase64AndCheck( "abcd",
								 "YWJjZA==");

	// check normal encoding:
	NextSubTest(); 
	EncodeBase64AndCheck( "Simple encoding äöüß",
								 "U2ltcGxlIGVuY29kaW5nIMOkw7bDvMOf");

	// complete alphabet, 255 characters (1-255):
	BmString alphabet;
	for( int i = 0; i<16; ++i)
		alphabet << AsciiAlphabet[i];
	NextSubTest(); 
	EncodeBase64AndCheck( 
		alphabet,
		"AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyAhIiMkJSYnKCkqKywtLi8wMTI"
		"zNDU2Nzg5\r\nOjs8PT4/QEFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaW1xdXl9gYW"
		"JjZGVmZ2hpamtsbW5vcHFy\r\nc3R1dnd4eXp7fH1+f4CBgoOEhYaHiImKi4yNjo+Qk"
		"ZKTlJWWl5iZmpucnZ6foKGio6Slpqeoqaqr\r\nrK2ur7CxsrO0tba3uLm6u7y9vr/A"
		"wcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk\r\n5ebn6Onq6+zt7u/"
		"w8fLz9PX29/j5+vv8/f7/"
	);

	{
		// complete alphabet, again, this time on single line:
		Activator activator(SingleLineMode);
		NextSubTest(); 
		EncodeBase64AndCheck( 
			alphabet,
			"AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyAhIiMkJSYnKCkqKywtLi8wMTI"
			"zNDU2Nzg5Ojs8PT4/QEFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaW1xdXl9gYW"
			"JjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXp7fH1+f4CBgoOEhYaHiImKi4yNjo+Qk"
			"ZKTlJWWl5iZmpucnZ6foKGio6SlpqeoqaqrrK2ur7CxsrO0tba3uLm6u7y9vr/A"
			"wcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk5ebn6Onq6+zt7u/"
			"w8fLz9PX29/j5+vv8/f7/"
		);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Base64EncoderTest::MultiLineTest()
{
	// just check one text with multiple lines:
	NextSubTest(); 
	EncodeBase64AndCheck( 
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf.\n"
		"Einer mehr an dem ich aufstehen muß\n"
		"um irgendwas zu tun gegen den Schmerz.\n",
		//
		"Vm9yIG1laW5lbiBGZW5zdGVyIGbDpG5ndCBlcyBhbiBzaWNoIHp1IGJld2VnZW4KZWlu"
		"IG5ldWVy\r\nIFRhZyBuaW1tdCBzZWluZW4gVGFnZXNsYXVmLgpFaW5lciBtZWhyIGFu"
		"IGRlbSBpY2ggYXVmc3Rl\r\naGVuIG11w58KdW0gaXJnZW5kd2FzIHp1IHR1biBnZWdl"
		"biBkZW4gU2NobWVyei4K"
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Base64EncoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.base64_decoded", input);
	BmString result;
	SlurpFile("testdata.base64_encoded", result);
	// check if decoding of file-contents yields intended result:
	NextSubTest(); 
	EncodeBase64AndCheck( input, result);
}
