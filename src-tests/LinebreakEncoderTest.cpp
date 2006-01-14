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
