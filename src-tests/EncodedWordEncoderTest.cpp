/*
	EncodedWordEncoderTest.cpp
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

#include "EncodedWordEncoderTest.h"
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
EncodedWordEncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
EncodedWordEncoderTest::tearDown()
{
	inherited::tearDown();
}

static void EncodeQpAndCheck( BmString input, BmString result, 
										BmString srcCharset="utf-8",
										int32 firstDiscardedPos=-1,
										bool hasError=false);
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeQpAndCheck( BmString input, BmString result, 
										BmString srcCharset, int32 firstDiscardedPos,
										bool hasError) {
	BmString encodedStr; 
	int32 blockSize = 128; 
	BmStringIBuf srcBuf( input); 
	BmStringOBuf destBuf( blockSize); 
	BmQpEncodedWordEncoder qpEncoder( &srcBuf, blockSize, 0, srcCharset); 
	destBuf.Write( &qpEncoder, blockSize); 
	encodedStr.Adopt( destBuf.TheString()); 
	try { 
		CPPUNIT_ASSERT( encodedStr.Compare( result)==0); 
		CPPUNIT_ASSERT( firstDiscardedPos!=-1 
								|| qpEncoder.FirstDiscardedPos() == firstDiscardedPos);
		CPPUNIT_ASSERT( !hasError || qpEncoder.HadError());
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
EncodedWordEncoderTest::SimpleTest() {
	// empty run, yields empty encoded-word:
	NextSubTest(); 
	EncodeQpAndCheck( "",
							"=?utf-8?q??=");
	// check what happens if nothing is to be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "goof",
							"=?utf-8?q?goof?=");
	// check normal encoding (note that spaces become underlines):
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding äöüß",
							"=?utf-8?q?Simple_encoding_=C3=A4=C3=B6=C3=BC=C3=9F?=");
	// underlines should be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple_encoding_",
							"=?utf-8?q?Simple=5Fencoding=5F?=");
	// equal-signs must be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding =",
							"=?utf-8?q?Simple_encoding_=3D?=");
	// seemingly already qp, just the equal-signs should be encoded:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding =C3=9F",
							"=?utf-8?q?Simple_encoding_=3DC3=3D9F?=");
	// check that chars unknown in src-charset are discarded properly:
	NextSubTest(); 
	EncodeQpAndCheck( "Simple encoding äöüß xxx",
							"=?us-ascii?q?Simple_encoding__xxx?=",
							"us-ascii", 17);
	// broken utf-8, bytes missing at end (whole string will be dropped):
	NextSubTest(); 
	EncodeQpAndCheck( "text is broken \xFC",
							"", "utf-8", -1, true);
	// lower half alphabet test, 8 lines with 16 chars (minus 1):
	// (upper half left out because it would yield illegal utf-8 characters)
	BmString result[8] = {
		"=?utf-8?q?=01=02=03=04=05=06=07=08=09=0A=0B=0C=0E=0F?=",
							// yes, =0D (\r) is dumped!
		"=?utf-8?q?=10=11=12=13=14=15=16=17=18=19=1A=1B=1C=1D=1E=1F?=",
		"=?utf-8?q?_!=22#$%&=27=28=29*+=2C-./?=",
		"=?utf-8?q?0123456789=3A=3B=3C=3D=3E=3F?=",
		"=?utf-8?q?@ABCDEFGHIJKLMNO?=",
		"=?utf-8?q?PQRSTUVWXYZ=5B=5C=5D^=5F?=",
		"=?utf-8?q?=60abcdefghijklmno?=",
		"=?utf-8?q?pqrstuvwxyz{|}=7E=7F?=",
	};
	for( int i = 0; i<8; ++i) {
		NextSubTest(); 
		EncodeQpAndCheck( AsciiAlphabet[i], result[i]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
EncodedWordEncoderTest::MultiLineTest() {
	// check long lines, qp-encoding should fold them:
	NextSubTest(); 
	EncodeQpAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"=?utf-8?q?Ich_war_dabei_mir_eine_Art_von_Verschwinden=2C_"
		"die_den_Tod_bezwi?=\r\n"
		" =?utf-8?q?ngt_auszudenken_und_lie=C3=9F_mich_nieder_"
		"wo_ich_mich_beherrsch?=\r\n"
		" =?utf-8?q?e?="
	);
	// check wrapping border at 76 characters (should not be folded):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234",
		//
		"=?utf-8?q?123456789012345678901234567890123456789012345678901234567890"
		"1234?="
	);
	// check wrapping border at 76 characters (should be folded):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"12345678901234567890123456789012345678901234567890123456789012345",
		//
		"=?utf-8?q?123456789012345678901234567890123456789012345678901234567890"
		"1234?=\r\n"
		" =?utf-8?q?5?="
	);

	// check encoding near wrapping border (fits on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678ä012345678",
		//
		"=?utf-8?q?1234567890123456789012345678901234567890123456789012345678"
		"=C3=A4?=\r\n"
		" =?utf-8?q?012345678?="
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"12345678901234567890123456789012345678901234567890123456789ä12345678",
		//
		"=?utf-8?q?1234567890123456789012345678901234567890123456789012345678"
		"9?=\r\n"
		" =?utf-8?q?=C3=A412345678?="
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"123456789012345678901234567890123456789012345678901234567890ä2345678",
		//
		"=?utf-8?q?1234567890123456789012345678901234567890123456789012345678"
		"90?=\r\n"
		" =?utf-8?q?=C3=A42345678?="
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901ä345678",
		//
		"=?utf-8?q?1234567890123456789012345678901234567890123456789012345678"
		"901?=\r\n"
		" =?utf-8?q?=C3=A4345678?="
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"12345678901234567890123456789012345678901234567890123456789012ä45678",
		//
		"=?utf-8?q?1234567890123456789012345678901234567890123456789012345678"
		"9012?=\r\n"
		" =?utf-8?q?=C3=A445678?="
	);

	// now same thing but different charset:
	// check encoding near wrapping border (fits on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"12345678901234567890123456789012345678901234567890123456ä89012345678",
		//
		"=?iso-8859-1?q?12345678901234567890123456789012345678901234567890123"
		"456=E4?=\r\n"
		" =?iso-8859-1?q?89012345678?=",
		"iso-8859-1"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"123456789012345678901234567890123456789012345678901234567ä9012345678",
		//
		"=?iso-8859-1?q?12345678901234567890123456789012345678901234567890123"
		"4567?=\r\n"
		" =?iso-8859-1?q?=E49012345678?=",
		"iso-8859-1"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678ä012345678",
		//
		"=?iso-8859-1?q?12345678901234567890123456789012345678901234567890123"
		"45678?=\r\n"
		" =?iso-8859-1?q?=E4012345678?=",
		"iso-8859-1"
	);
	// check encoding near wrapping border (doesn't fit on current line):
	NextSubTest(); 
	EncodeQpAndCheck( 
		"12345678901234567890123456789012345678901234567890123456789ä12345678",
		//
		"=?iso-8859-1?q?12345678901234567890123456789012345678901234567890123"
		"456789?=\r\n"
		" =?iso-8859-1?q?=E412345678?=",
		"iso-8859-1"
	);

	// the following is a japanese-textphrase which had exposed a hanging-error
	// in QpEncodedWordEncoder:
	NextSubTest(); 
	EncodeQpAndCheck( "Re: ありがとうございます。 (and more...)",
							"=?utf-8?q?Re=3A_=E3=81=82=E3=82=8A=E3=81=8C=E3=81=A8=E3"
								"=81=86=E3=81=94?=\r\n"
							" =?utf-8?q?=E3=81=96=E3=81=84=E3=81=BE=E3=81=99=E3=80"
								"=82_=28and_more...=29?=");
}
