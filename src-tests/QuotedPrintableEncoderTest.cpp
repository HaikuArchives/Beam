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

#include "QuotedPrintableEncoderTest.h"
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

static void EncodeQpAndCheck( BmString input, BmString result);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeQpAndCheck( BmString input, BmString result) { 
	BmString encodedStr; 
	int32 blockSize = 128; 
	BmStringIBuf srcBuf( input); 
	BmStringOBuf destBuf( blockSize); 
	BmQuotedPrintableEncoder qpEncoder( &srcBuf, blockSize); 
	destBuf.Write( &qpEncoder, blockSize); 
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
QuotedPrintableEncoderTest::SimpleTest() {
	// empty run:
	NextSubTest(); 
	EncodeQpAndCheck( "",
							"");
	// check what happens if nothing is to be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "A sentence with no encoded chars.",
							"A sentence with no encoded chars.");
	// check normal encoding:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding äöüß",
							"Simple encoding =C3=A4=C3=B6=C3=BC=C3=9F");
	// underlines should just be copied:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple_encoding_",
							"Simple_encoding_");
	// equal-signs must be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding =",
							"Simple encoding =3D");
	// seemingly already qp, just the equal-signs should be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding =C3=9F",
							"Simple encoding =3DC3=3D9F");

	// complete alphabet test, 16 lines with 16 chars (minus 1):
	BmString result[16] = {
		"=01=02=03=04=05=06=07=08=09\r\n=0B=0C=0E=0F",
							// yes, =0D (\r) is dumped!
		"=10=11=12=13=14=15=16=17=18=19=1A=1B=1C=1D=1E=1F",
		" !\"#$%&'()*+,-./",
		"0123456789:;<=3D>?",
		"@ABCDEFGHIJKLMNO",
		"PQRSTUVWXYZ[\\]^_",
		"=60abcdefghijklmno",
		"pqrstuvwxyz{|}~=7F",
		"=80=81=82=83=84=85=86=87=88=89=8A=8B=8C=8D=8E=8F",
		"=90=91=92=93=94=95=96=97=98=99=9A=9B=9C=9D=9E=9F",
		"=A0=A1=A2=A3=A4=A5=A6=A7=A8=A9=AA=AB=AC=AD=AE=AF",
		"=B0=B1=B2=B3=B4=B5=B6=B7=B8=B9=BA=BB=BC=BD=BE=BF",
		"=C0=C1=C2=C3=C4=C5=C6=C7=C8=C9=CA=CB=CC=CD=CE=CF",
		"=D0=D1=D2=D3=D4=D5=D6=D7=D8=D9=DA=DB=DC=DD=DE=DF",
		"=E0=E1=E2=E3=E4=E5=E6=E7=E8=E9=EA=EB=EC=ED=EE=EF",
		"=F0=F1=F2=F3=F4=F5=F6=F7=F8=F9=FA=FB=FC=FD=FE=FF"
	};
	for( int i = 0; i<16; ++i) {
		NextSubTest(); 
		EncodeQpAndCheck( AsciiAlphabet[i], result[i]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableEncoderTest::MultiLineTest() {
	// check two lines, encoding should convert "\n" to "\r\n":
	NextSubTest(); 
	EncodeQpAndCheck( 
		"Vor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// empty lines at start:
	NextSubTest(); 
	EncodeQpAndCheck( 
		"\n\n\nVor meinen Fenster fängt es an sich zu bewegen\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"\r\n\r\n\r\nVor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// lone \r should be dropped:
	NextSubTest(); 
	EncodeQpAndCheck( 
		"Vor meinen Fenster fängt es \ran sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf.",
		//
		"Vor meinen Fenster f=C3=A4ngt es an sich zu bewegen\r\n"
		"ein neuer Tag nimmt seinen Tageslauf."
	);
	// spaces at end of line should be encoded...
	NextSubTest(); 
	EncodeQpAndCheck( 
		"Keine Liebe keine Arbeit kein Leben   \n"
		"an meinem Kissen schlag ich mir den Kopf auf",
		//
		"Keine Liebe keine Arbeit kein Leben=20=20=20\r\n"
		"an meinem Kissen schlag ich mir den Kopf auf"
	);
	// check long lines, qp-encoding should fold them:
	NextSubTest(); 
	EncodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenk=\r\n"
		"en und lie=C3=9F mich nieder wo ich mich beherrsche"
	);
	// check wrapping border at 76 characters (should not be folded):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123456",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123456"
	);
	// check wrapping border at 76 characters (should be folded):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1234567",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"12345=\r\n67"
	);
	// check wrapping border at 76 characters (should not be folded, 
	// since it contains a hard linebreak at pos 77):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123456\n"
		"78901234567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123456\r\n"
		"78901234567890"
	);
	// check encoding near wrapping border (fits on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"123456789012345678901234567890123456789012345678901234567890123456789ä"
		"1234567890",
		//
		"123456789012345678901234567890123456789012345678901234567890123456789"
		"=C3=A4=\r\n"
		"1234567890"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"ä234567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"=C3=\r\n"
		"=A4234567890"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1ä34567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1=C3=\r\n"
		"=A434567890"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"12ä4567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"12=C3=\r\n"
		"=A44567890"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123ä567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"123=\r\n"
		"=C3=A4567890"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1234ä67890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1234=\r\n"
		"=C3=A467890"
	);

	// another check of folding code:
	NextSubTest(); 
	EncodeQpAndCheck( 
		"<  9496|000000043691>: Testhänger has started\n"
		"<  9496|000000148730>: RefManager: reference to "
			"<MailFolderList:0x80207cd0> added, ref-count is 1\n"
		"<  9496|000000153476>: RefManager: reference to "
			"<SmtpAccountList:0x80208098> added, ref-count is 1\n"
		"<  9496|000000153802>: RefManager: reference to "
			"<PopAccountList:0x80207e48> added, ref-count is 1",
		//
		"<  9496|000000043691>: Testh=C3=A4nger has started\r\n"
		"<  9496|000000148730>: RefManager: reference "
			"to <MailFolderList:0x80207cd0>=\r\n"
		" added, ref-count is 1\r\n"
		"<  9496|000000153476>: RefManager: reference to "
			"<SmtpAccountList:0x80208098=\r\n"
		"> added, ref-count is 1\r\n"
		"<  9496|000000153802>: RefManager: reference to "
			"<PopAccountList:0x80207e48>=\r\n"
		" added, ref-count is 1"
	);

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
QuotedPrintableEncoderTest::LargeDataTest() {
	if (!HaveTestdata)
		return;
	Activator activate(LargeDataMode);
	BmString input;
	SlurpFile("testdata.qp_decoded", input);
	BmString result;
	SlurpFile("testdata.qp_encoded", result);
	// check if decoding of file-contents yields intended result:
	NextSubTest();
	EncodeQpAndCheck( input, result);
}
