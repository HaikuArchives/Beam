/*
	QuotedPrintableEncoderTest.cpp
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

#include "QuotedPrintableEncoderTest.h"

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
 * encoded string should be in utf-8, too.
 *
 */

// setUp
void
QuotedPrintableEncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
QuotedPrintableEncoderTest::tearDown()
{
	inherited::tearDown();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
// QuotedPrintableEncoder
#define ENCODE_QP_AND_CHECK( str, result) \
{ \
	BmString encodedStr; \
	int32 blockSize = 128; \
	NextSubTest(); \
	BmString src(str); \
	BmStringIBuf srcBuf( src); \
	BmStringOBuf destBuf( blockSize); \
	if (IsEncodedWord) { \
		BmQpEncodedWordEncoder qpEncoder( &srcBuf, blockSize, 0, "utf-8"); \
		destBuf.Write( &qpEncoder, blockSize); \
	} else { \
		BmQuotedPrintableEncoder qpEncoder( &srcBuf, blockSize); \
		destBuf.Write( &qpEncoder, blockSize); \
	} \
	encodedStr.Adopt( destBuf.TheString()); \
	try { \
		CPPUNIT_ASSERT( encodedStr.Compare( result)==0); \
	} catch( ...) { \
		DumpResult( encodedStr); \
		throw; \
	} \
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableEncoderTest::SimpleTest()
{
	// empty run:
	ENCODE_QP_AND_CHECK( "",
								"");
	// check what happens if nothing is to be encoded:
	ENCODE_QP_AND_CHECK( "A sentence with no encoded chars.",
								"A sentence with no encoded chars.");
	// check normal encoding:
	ENCODE_QP_AND_CHECK( "Simple encoding äöüß",
								"Simple encoding =C3=A4=C3=B6=C3=BC=C3=9F");
	// underlines should just be copied:
	ENCODE_QP_AND_CHECK( "Simple_encoding_",
								"Simple_encoding_");
	// equal-signs must be encoded:
	ENCODE_QP_AND_CHECK( "Simple encoding =",
								"Simple encoding =3D");
	// seemingly already qp, just the equal-signs should be encoded:
	ENCODE_QP_AND_CHECK( "Simple encoding =C3=9F",
								"Simple encoding =3DC3=3D9F");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableEncoderTest::EncodedWordTest()
{
	IsEncodedWord = true;
	// empty run, yields empty encoded-word:
	ENCODE_QP_AND_CHECK( "",
								"=?utf-8?q??=");
	// check what happens if nothing is to be encoded:
	ENCODE_QP_AND_CHECK( "goof",
								"=?utf-8?q?goof?=");
	// check normal encoding (note that spaces become underlines):
	ENCODE_QP_AND_CHECK( "Simple encoding äöüß",
								"=?utf-8?q?Simple_encoding_=C3=A4=C3=B6=C3=BC=C3=9F?=");
	// underlines should be encoded:
	ENCODE_QP_AND_CHECK( "Simple_encoding_",
								"Simple=3Fencoding=3F");
	// equal-signs must be encoded:
	ENCODE_QP_AND_CHECK( "Simple encoding =",
								"Simple_encoding_=3F");
	// seemingly already qp, just the equal-signs should be encoded:
	ENCODE_QP_AND_CHECK( "Simple_encoding_=C3=9F",
								"Simple_encoding_=3EC3=3E9F");
	IsEncodedWord = false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableEncoderTest::MultiLineTest()
{
/*
	// check two lines, qp should convert "\r\n" to "\n":
	ENCODE_QP_AND_CHECK( 
		"Vor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// empty lines at start:
	ENCODE_QP_AND_CHECK( 
		"\n\r\n\r\nVor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"\n\n\nVor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// lone \r should be dropped:
	ENCODE_QP_AND_CHECK( 
		"Vor meinen Fenster f=C3=A4ngt es \ran sich zu bewegen\r\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// spaces at end of line should be discarded...
	ENCODE_QP_AND_CHECK( 
		"Keine Liebe keine Arbeit kein Leben    \r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben\n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// ...unless they are encoded:
	ENCODE_QP_AND_CHECK( 
		"Keine Liebe keine Arbeit kein Leben=20=20=20\r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben   \n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// check folded lines, qp should unfold them (without inserting spaces):
	ENCODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// unfolding should drop spaces after fold-char:
	ENCODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =      \r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// faked folded line, no \r\n following:
	ENCODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// faked folded line, no \r\n following:
	ENCODE_QP_AND_CHECK(
		"Ich war dabei mir eine Art von Verschwinden, =\r",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// erraneous folded line (with empty second part):
	ENCODE_QP_AND_CHECK( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
	);
*/
}
