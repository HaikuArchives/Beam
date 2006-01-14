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

#include "FoldedLineEncoderTest.h"
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
FoldedLineEncoderTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
FoldedLineEncoderTest::tearDown()
{
	inherited::tearDown();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void EncodeFlAndCheck( BmString input, BmString result) { 
	BmString encodedStr; 
	int32 blockSize = 128; 
	BmStringIBuf srcBuf( input); 
	BmStringOBuf destBuf( blockSize); 
	BmFoldedLineEncoder flEncoder( &srcBuf, 76, blockSize, 6); 
							// N.B.: we assume the name of the folded header-field to
							//       be 'From', so it's 6 chars offset ('From: ').
	destBuf.Write( &flEncoder, blockSize); 
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
FoldedLineEncoderTest::SimpleTest() {
	// empty run:
	NextSubTest(); 
	EncodeFlAndCheck( "", "");
	// check what happens if nothing is to be folded:
	NextSubTest(); 
	EncodeFlAndCheck( "A sentence with no encoded chars.",
							"A sentence with no encoded chars.");
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void
FoldedLineEncoderTest::MultiLineTest() {
	// lone \r should be dropped:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Vor meinen Fenster fängt es \ran sich zu bewegen",
		//
		"Vor meinen Fenster fängt es an sich zu bewegen"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod1 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod1 bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod12 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod12 bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod123 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod123 bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod1234 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod1234 bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod12345 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod12345 bezwingt\r\n"
		" auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check long lines, they should be folded (this one folds earlier):
	NextSubTest(); 
	EncodeFlAndCheck( 
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod123456 bezwingt auszudenken "
		"und ließ mich nieder wo ich mich beherrsche",
		//
		"Ich war dabei mir eine Art von Verschwinden, "
		"die den Tod123456\r\n"
		" bezwingt auszudenken und ließ mich nieder wo ich mich beherrsche"
	);
	// check wrapping of long words:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1234567890123456789012345678901234567890123456789012345678901234567890"
		"1234567890123456789012345678901234567890123456789012345678901234567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
			"\r\n"
		" 1234567890123456789012345678901234567890123456789012345678901234567890"
			"12345\r\n"
		" 67890123456789012345678901234567890123456789012345678901234567890"
	);
	// check wrapping near border at 76 characters:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
	);
	// check wrapping near border at 76 characters:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"12345678901234567890123456789012345678901234567890123456789012345678901",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
			"\r\n"
		" 1"
	);
	// check wrapping of space near border at 76 characters:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"123456789012345678901234567890123456789012345678901234567890123456789 ",
		//
		"123456789012345678901234567890123456789012345678901234567890123456789 "
	);
	// check wrapping of space near border at 76 characters:
	NextSubTest(); 
	EncodeFlAndCheck( 
		"1234567890123456789012345678901234567890123456789012345678901234567890 ",
		//
		"1234567890123456789012345678901234567890123456789012345678901234567890"
			"\r\n"
		" "
	);
}
