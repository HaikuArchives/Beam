/*
	StringTest.cpp
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

#include <UTF8.h>

#include "StringTest.h"
#include "TestBeam.h"

#include "BmString.h"

// setUp
void
StringTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
StringTest::tearDown()
{
	inherited::tearDown();
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringBeamExtensionsTest(void)
{
	NextSubTest();
	BmString int64test;
	int64 ll = static_cast<int64>(4)*1000*1000*1000*1000*1000;
	int64test << ll;
	CPPUNIT_ASSERT( 
		strcmp( 
			int64test.String(), 
		 	"4000000000000000"
		) == 0
	);

	NextSubTest();
	BmString crlf( "this\r\n is a small\r test \nof linebreak-conversion\r\n");
	crlf.ConvertLinebreaksToLF();
	CPPUNIT_ASSERT( 
		strcmp( 
			crlf.String(), 
		 	"this\n is a small\r test \nof linebreak-conversion\n"
		) == 0
	);

	NextSubTest();
	BmString lf( "this\n is a small\r test\r\n of linebreak-conversion\n");
	lf.ConvertLinebreaksToCRLF();
	CPPUNIT_ASSERT( 
		strcmp( 
			lf.String(), 
		 	"this\r\n is a small\r test\r\n of linebreak-conversion\r\n"
		) == 0
	);

	NextSubTest();
	BmString tabs( "this\t is a small\r test of\t\ttabs-conversion\r\n");
	tabs.ConvertTabsToSpaces( 4);
	CPPUNIT_ASSERT( 
		strcmp( 
			tabs.String(), 
		 	"this     is a small\r test of        tabs-conversion\r\n"
		) == 0
	);

	NextSubTest();
	BmString url( "http://www.test.org?foo=%20%%%20ximul&bar=%Fa%xxdigit");
	url.DeUrlify();
	CPPUNIT_ASSERT( 
		strcmp( 
			url.String(), 
		 	"http://www.test.org?foo= % ximul&bar=\xfa%xxdigit"
		) == 0
	);

	NextSubTest();
	BmString trim( "");
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "") == 0);

	NextSubTest();
	trim = "x";
	trim.Trim(true, true);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x") == 0);

	NextSubTest();
	trim = "x";
	trim.Trim(true, false);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x") == 0);

	NextSubTest();
	trim = "x";
	trim.Trim(false, true);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x") == 0);

	NextSubTest();
	trim = "x";
	trim.Trim(false, false);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x") == 0);

	NextSubTest();
	trim = " x ";
	trim.Trim(true, true);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x") == 0);

	NextSubTest();
	trim = " x ";
	trim.Trim(true, false);
	CPPUNIT_ASSERT( strcmp( trim.String(), "x ") == 0);

	NextSubTest();
	trim = " x ";
	trim.Trim(false, true);
	CPPUNIT_ASSERT( strcmp( trim.String(), " x") == 0);

	NextSubTest();
	trim = " x ";
	trim.Trim(false, false);
	CPPUNIT_ASSERT( strcmp( trim.String(), " x ") == 0);

	NextSubTest();
	trim = "xxx";
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "xxx") == 0);

	NextSubTest();
	trim = " xxx";
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "xxx") == 0);

	NextSubTest();
	trim = "xxx ";
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "xxx") == 0);

	NextSubTest();
	trim = " xxx ";
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "xxx") == 0);

	NextSubTest();
	trim = "          x x x         ";
	trim.Trim();
	CPPUNIT_ASSERT( strcmp( trim.String(), "x x x") == 0);

	NextSubTest();
	trim = "          x x x         ";
	trim.Trim( false, false);
	CPPUNIT_ASSERT( strcmp( trim.String(), "          x x x         ") == 0);
}
