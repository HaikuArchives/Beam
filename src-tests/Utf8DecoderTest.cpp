/*
	Utf8DecoderTest.cpp
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

#include "Utf8DecoderTest.h"
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
Utf8DecoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
Utf8DecoderTest::tearDown()
{
	inherited::tearDown();
}

static void DecodeUtf8AndCheck( BmString input, BmString result, 
										  BmString destCharset=DefaultCharset,
										  int32 firstDiscardedPos=-1,
										  bool hasError=false);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void DecodeUtf8AndCheck( BmString input, BmString result, 
										  BmString destCharset, int32 firstDiscardedPos,
										  bool hasError) {
	BmString decodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmUtf8Decoder decoder( &srcBuf, destCharset, blockSize);
	destBuf.Write( &decoder, blockSize);
	decodedStr.Adopt( destBuf.TheString());
	try {
		CPPUNIT_ASSERT( decodedStr.Compare( result)==0);
		CPPUNIT_ASSERT( firstDiscardedPos!=-1 
								|| decoder.FirstDiscardedPos() == firstDiscardedPos);
		CPPUNIT_ASSERT( !hasError || decoder.HadError());
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
Utf8DecoderTest::SimpleTest()
{
	// empty run:
	NextSubTest(); 
	DecodeUtf8AndCheck( "",
							  "");
	// check us-ascii compatible text:
	NextSubTest(); 
	DecodeUtf8AndCheck( "A simple text (which contains only us-ascii chars)",
							  "A simple text (which contains only us-ascii chars)");
	// check decoding of special characters:
	NextSubTest(); 
	DecodeUtf8AndCheck( "äöüß",
							  "\xe4\xf6\xfc\xdf");
	// check decoding of special characters which can't be found in dest-charset:
	NextSubTest(); 
	DecodeUtf8AndCheck( 
		"äöü - The €-sign is only contained in iso-8859-15",
		"\xe4\xf6\xfc - The -sign is only contained in iso-8859-15",
		"iso-8859-1", 14
	);
	// broken utf-8, bytes missing at end (should be dropped):
	NextSubTest(); 
	DecodeUtf8AndCheck( "text is broken \xFC",
							  "text is broken ", DefaultCharset, -1, true);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
Utf8DecoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.utf8_encoded", input);
	BmString result;
	SlurpFile("testdata.utf8_decoded", result);
	// check if decoding of file-contents yields intended result:
	NextSubTest(); 
	DecodeUtf8AndCheck( input, result);
}
