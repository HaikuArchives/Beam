/*
	QuotedPrintableDecoderTest.cpp
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

#include "split.hh"
using namespace regexx;

#include "QuotedPrintableDecoderTest.h"

#include "BmEncoding.h"

static bool IsEncodedWord = false;


static void DumpResult( const BmString& str) {
	vector<BmString> lines = split( "\n", str);
	cerr << "Result:" << endl;
	for( uint32 i=0; i<lines.size(); ++i) 
		cerr << "|" << lines[i].String() << "|" << endl;
}

/*
 *
 * Please note that any string-constants in this file are UTF-8, so the
 * qp-encoded string should be in utf-8, too.
 *
 */

// setUp
void
QuotedPrintableDecoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
QuotedPrintableDecoderTest::tearDown()
{
	inherited::tearDown();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
// QuotedPrintableDecoder
#define DECODE_QP_AND_CHECK( qp, result) \
{ \
	BmString decodedStr; \
	int32 blockSize = 128; \
	NextSubTest(); \
	BmString src(qp); \
	BmStringIBuf srcBuf( src); \
	BmStringOBuf destBuf( blockSize); \
	BmQuotedPrintableDecoder qpDecoder( &srcBuf, IsEncodedWord, blockSize); \
	destBuf.Write( &qpDecoder, blockSize); \
	decodedStr.Adopt( destBuf.TheString()); \
	try { \
		CPPUNIT_ASSERT( decodedStr == result); \
	} catch( ...) { \
		DumpResult( decodedStr); \
		throw; \
	} \
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableDecoderTest::SimpleTest()
{
	// empty run:
	DECODE_QP_AND_CHECK( "",
								"");
	// check what happens if nothing is to be decoded:
	DECODE_QP_AND_CHECK( "A sentence with no encoded chars.",
								"A sentence with no encoded chars.");
	// check normal decoding:
	DECODE_QP_AND_CHECK( "Simple decoding =C3=A4=C3=B6=C3=BC=C3=9F",
								"Simple decoding äöüß");
	// case-insensitivity:
	DECODE_QP_AND_CHECK( "Simple decoding =c3=A4=c3=B6=C3=bC=C3=9f",
								"Simple decoding äöüß");
	// underline in bodypart-mode should just be copied:
	DECODE_QP_AND_CHECK( "Simple_decoding_",
								"Simple_decoding_");
	// underline in encoded-word-mode should be converted to space:
	IsEncodedWord = true;
	DECODE_QP_AND_CHECK( "Simple_decoding_",
								"Simple decoding ");
	IsEncodedWord = false;
	// unneccessary encoding:
	DECODE_QP_AND_CHECK( "Simple=20decoding=20=C3=A4=C3=B6=C3=BC=C3=9F",
								"Simple decoding äöüß");
	// broken encoding, non-hex after '=' (should be copied):
	DECODE_QP_AND_CHECK( "Simple decoding =xy=C3=B6=C3=BC=C3=9F",
								"Simple decoding =xyöüß");
	// broken encoding, whitespace after '=' (should be copied):
	DECODE_QP_AND_CHECK( "Simple decoding =C3=B6= =C3=BC=C3=9F",
								"Simple decoding ö= üß");
	// equal-sign massacre (should be copied):
	DECODE_QP_AND_CHECK( "Simple decoding ==========C3=9F",
								"Simple decoding =========ß");
	// broken encoding, chars missing at end (they should be copied):
	DECODE_QP_AND_CHECK( "Simple decoding =C3=B6=C3=BC=C",
								"Simple decoding öü=C");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableDecoderTest::MultiLineTest()
{
	// check two lines, qp should convert "\r\n" to "\n":
	DECODE_QP_AND_CHECK( 
		"Vor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// empty lines at start:
	DECODE_QP_AND_CHECK( 
		"\n\r\n\r\nVor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"\n\n\nVor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// lone \r should be dropped:
	DECODE_QP_AND_CHECK( 
		"Vor meinen Fenster f=C3=A4ngt es \ran sich zu bewegen\r\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// spaces at end of line should be discarded...
	DECODE_QP_AND_CHECK( 
		"Keine Liebe keine Arbeit kein Leben    \r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben\n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// ...unless they are encoded:
	DECODE_QP_AND_CHECK( 
		"Keine Liebe keine Arbeit kein Leben=20=20=20\r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben   \n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// check folded lines, qp should unfold them (without inserting spaces):
	DECODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// unfolding should drop spaces after fold-char:
	DECODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =      \r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// faked folded line, no \r\n following:
	DECODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// faked folded line, no \r\n following:
	DECODE_QP_AND_CHECK(
		"Ich war dabei mir eine Art von Verschwinden, =\r",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// erraneous folded line (with empty second part):
	DECODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
	);
}
