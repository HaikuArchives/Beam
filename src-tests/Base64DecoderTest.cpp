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

#include "Base64DecoderTest.h"
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
Base64DecoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
Base64DecoderTest::tearDown()
{
	inherited::tearDown();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void DecodeBase64AndCheck( BmString input, BmString result)
{
	BmString decodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmBase64Decoder decoder( &srcBuf, blockSize);
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
Base64DecoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	DecodeBase64AndCheck( "",
								 "");
	// check 1 byte:
	NextSubTest(); 
	DecodeBase64AndCheck( "YQ==",
								 "a");
	// check 2 bytes:
	NextSubTest(); 
	DecodeBase64AndCheck( "YWI=",
								 "ab");
	// check 3 bytes:
	NextSubTest(); 
	DecodeBase64AndCheck( "YWJj",
								 "abc");
	// check 4 bytes:
	NextSubTest(); 
	DecodeBase64AndCheck( "YWJjZA==",
								 "abcd");

	// check what happens with an extra '=' at end, should be ignored:
	NextSubTest(); 
	DecodeBase64AndCheck( "YWJjZA===",
								 "abcd");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( "=",
								 "");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( "++=",
								 "\xfb");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( "++==",
								 "\xfb");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( "+===",
								 "\xf8");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( "++",
								 "\xfb");

	// some more robustness checks:
	NextSubTest(); 
	DecodeBase64AndCheck( " =\r\t!",
								 "");

	// check normal decoding:
	NextSubTest(); 
	DecodeBase64AndCheck( "U2ltcGxlIGVuY29kaW5nIMOkw7bDvMOf",
								 "Simple encoding äöüß");

	// check that decoding skips whitespace:
	NextSubTest(); 
	DecodeBase64AndCheck( "U2 l tcG xl IGVuY2\t\n\r9ka W5	nIMOkw7bDvMOf",
								 "Simple encoding äöüß");

	// check that decoding skips illegal chars (not in base64-alphabet):
	NextSubTest(); 
	DecodeBase64AndCheck( "U2ltcGxlIGVu!§$%&()Y29kaW5nIMOkw7bDvMOf",
								 "Simple encoding äöüß");

	// check that decoding correctly handles embedded padding chars
	// by resetting the state (the result in this test is iso-2022-jp text):
	NextSubTest(); 
	DecodeBase64AndCheck( "W1lhaG9vIRskQiVHJWolUCE8GyhCXQ==IBskQiJjMEIbKEI="
								 "GyRCPzQiZDpHQzskR0IoRnwlLSVjJUMlNyVzGyhC"
								 "GyRCJTAhKiUtJWMlQyU3JWUlbyVzGyhC",
								 "[Yahoo!\x1b$B%G%j%P!<\x1b(B] \x1b$B\"c0B\x1b(B"
								 "\x1b$B?4\"d:GC;$GB(F|%-%c%C%7%s\x1b(B\x1b$B%0!"
								 "*%-%c%C%7%e%o%s\x1b(B");

	// complete alphabet, 255 characters (1-255):
	BmString alphabet;
	for( int i = 0; i<16; ++i)
		alphabet << AsciiAlphabet[i];
	NextSubTest(); 
	DecodeBase64AndCheck( 
		"AQIDBAUGBwgJCgsMDQ4PEBESExQVFhcYGRobHB0eHyAhIiMkJSYnKCkqKywtLi8wMTI"
		"zNDU2Nzg5\r\nOjs8PT4/QEFCQ0RFRkdISUpLTE1OT1BRUlNUVVZXWFlaW1xdXl9gYW"
		"JjZGVmZ2hpamtsbW5vcHFy\r\nc3R1dnd4eXp7fH1+f4CBgoOEhYaHiImKi4yNjo+Qk"
		"ZKTlJWWl5iZmpucnZ6foKGio6Slpqeoqaqr\r\nrK2ur7CxsrO0tba3uLm6u7y9vr/A"
		"wcLDxMXGx8jJysvMzc7P0NHS09TV1tfY2drb3N3e3+Dh4uPk\r\n5ebn6Onq6+zt7u/"
		"w8fLz9PX29/j5+vv8/f7/",
		alphabet
	);
	
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Base64DecoderTest::MultiLineTest()
{
	// just check one text with multiple lines:
	NextSubTest(); 
	DecodeBase64AndCheck( 
		"Vm9yIG1laW5lbiBGZW5zdGVyIGbDpG5ndCBlcyBhbiBzaWNoIHp1IGJld2VnZW4KZWlu"
		"IG5ldWVy\r\nIFRhZyBuaW1tdCBzZWluZW4gVGFnZXNsYXVmLgpFaW5lciBtZWhyIGFu"
		"IGRlbSBpY2ggYXVmc3Rl\r\naGVuIG11w58KdW0gaXJnZW5kd2FzIHp1IHR1biBnZWdl"
		"biBkZW4gU2NobWVyei4K",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf.\n"
		"Einer mehr an dem ich aufstehen muß\n"
		"um irgendwas zu tun gegen den Schmerz.\n"
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Base64DecoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.base64_encoded", input);
	BmString result;
	SlurpFile("testdata.base64_decoded", result);
	// check if decoding of file-contents yields intended result:
	NextSubTest(); 
	DecodeBase64AndCheck( input, result);
}
