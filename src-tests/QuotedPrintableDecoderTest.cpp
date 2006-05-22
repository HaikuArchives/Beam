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

#include "QuotedPrintableDecoderTest.h"
#include "TestBeam.h"

#include "BmEncoding.h"

static bool IsEncodedWord = false;


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
static void DecodeQpAndCheck( BmString input, BmString result)
{
	BmString decodedStr;
	int32 blockSize = 128;
	BmStringIBuf srcBuf( input);
	BmStringOBuf destBuf( blockSize);
	BmQuotedPrintableDecoder qpDecoder( 
		&srcBuf, blockSize, 
		IsEncodedWord 
			? BmQuotedPrintableDecoder::nTagIsEncodedWord 
			: ""
	);
	destBuf.Write( &qpDecoder, blockSize);
	decodedStr.Adopt( destBuf.TheString());
	try {
		CPPUNIT_ASSERT( decodedStr == result);
	} catch( ...) { \
		DumpResult( decodedStr);
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableDecoderTest::SimpleTest() {
	// empty run:
	NextSubTest(); 
	DecodeQpAndCheck( "",
							"");
	// check what happens if nothing is to be decoded:
	NextSubTest(); 
	DecodeQpAndCheck( "A sentence with no encoded chars.",
							"A sentence with no encoded chars.");
	// check normal decoding:
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding =C3=A4=C3=B6=C3=BC=C3=9F",
							"Simple decoding äöüß");
	// case-insensitivity:
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding =c3=A4=c3=B6=C3=bC=C3=9f",
							"Simple decoding äöüß");
	// underline in bodypart-mode should just be copied:
	NextSubTest(); 
	DecodeQpAndCheck( "Simple_decoding_",
							"Simple_decoding_");
	{
		// underline in encoded-word-mode should be converted to space:
		Activator activate(IsEncodedWord);
		NextSubTest(); 
		DecodeQpAndCheck( "Simple_decoding_",
								"Simple decoding ");
	}
	// unneccessary encoding:
	NextSubTest(); 
	DecodeQpAndCheck( "Simple=20decoding=20=C3=A4=C3=B6=C3=BC=C3=9F",
							"Simple decoding äöüß");
	// broken encoding, non-hex after '=' (should be copied):
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding =xy=C3=B6=C3=BC=C3=9F",
							"Simple decoding =xyöüß");
	// broken encoding, whitespace after '=' (should be copied):
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding =C3=B6= =C3=BC=C3=9F",
							"Simple decoding ö= üß");
	// equal-sign massacre (should be copied):
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding ==========C3=9F",
							"Simple decoding =========ß");
	// broken encoding, chars missing at end (they should be copied):
	NextSubTest(); 
	DecodeQpAndCheck( "Simple decoding =C3=B6=C3=BC=C",
							"Simple decoding öü=C");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableDecoderTest::MultiLineTest() {
	// check two lines, qp should convert "\r\n" to "\n":
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Vor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// empty lines at start:
	NextSubTest(); 
	DecodeQpAndCheck( 
		"\n\r\n\r\nVor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"\n\n\nVor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// lone \r should be dropped:
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Vor meinen Fenster f=C3=A4ngt es \ran sich zu bewegen\r\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// spaces at end of line should be discarded...
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Keine Liebe keine Arbeit kein Leben    \r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben\n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// ...unless they are encoded:
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Keine Liebe keine Arbeit kein Leben=20=20=20\r\n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben   \n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// check folded lines, qp should unfold them (without inserting spaces):
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// unfolding should drop spaces after fold-char:
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, =      \r\n"
		"die den Tod bezwingt auszudenken",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken"
	);
	// faked folded line, no \r\n following:
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, =",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// faked folded line, no \r\n following:
	NextSubTest(); 
	DecodeQpAndCheck(
		"Ich war dabei mir eine Art von Verschwinden, =\r",
		//
		"Ich war dabei mir eine Art von Verschwinden, ="
	);
	// erraneous folded line (with empty second part):
	NextSubTest(); 
	DecodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, =\r\n"
		"",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableDecoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.qp_encoded", input);
	BmString result;
	SlurpFile("testdata.qp_decoded", result);
	// check if decoding of file-contents yields intended result:
	NextSubTest(); 
	DecodeQpAndCheck( input, result);
}
