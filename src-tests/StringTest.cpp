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
 * Most tests in this file are copies from the open-beos tests, since Beam's
 * string-class, BmString, is just an extension of the openbeos-class BmString.
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
StringTest::StringSubCopyTest(void)
{
	BmString *string1, *string2;
	
	//CopyInto(BmString&, int32, int32)
	NextSubTest();
	string1 = new BmString;
	string2 = new BmString("Something");
	string2->CopyInto(*string1, 4, 30);
	CPPUNIT_ASSERT(strcmp(string1->String(), "thing") == 0);
	delete string1;
	delete string2;
	
	//CopyInto(const char*, int32, int32)
	NextSubTest();
	char tmp[10];
	memset(tmp, 0, 10);
	string1 = new BmString("ABC");
	string1->CopyInto(tmp, 0, 4);
	CPPUNIT_ASSERT(strcmp(tmp, "ABC") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "ABC") == 0);
	delete string1;			
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringSearchTest(void)
{
	BmString *string1, *string2;
	int32 i;

	//FindFirst(BmString&)
	NextSubTest();
	string1 = new BmString("last but not least");
	string2 = new BmString("st");
	i = string1->FindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString;
	string2 = new BmString("some text");
	i = string1->FindFirst(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindFirst(char*)
	NextSubTest();
	string1 = new BmString("last but not least");
	i = string1->FindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	
	NextSubTest();
	string1 = new BmString;
	i = string1->FindFirst("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
#ifndef TEST_R5
//	Commented, since crashes R5
	NextSubTest();
	string1 = new BmString("string");
	i = string1->FindFirst((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindFirst(BmString&, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->FindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->FindFirst(*string2, 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->FindFirst(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindFirst(const char*, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst("abc", 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst((char*)NULL, 3);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindFirst(char)
	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindFirst('c');
	CPPUNIT_ASSERT(i == 2);
	delete string1;

	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindFirst('e');
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//FindFirst(char, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst("b", 3);
	CPPUNIT_ASSERT(i == 5);
	delete string1;

	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindFirst('e', 3);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindFirst("a", 9);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//FindLast(BmString&)
	NextSubTest();
	string1 = new BmString("last but not least");
	string2 = new BmString("st");
	i = string1->FindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString;
	string2 = new BmString("some text");
	i = string1->FindLast(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindLast(char*)
	NextSubTest();
	string1 = new BmString("last but not least");
	i = string1->FindLast("st");
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	
	NextSubTest();
	string1 = new BmString;
	i = string1->FindLast("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BmString("string");
	i = string1->FindLast((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(BmString&, int32)
	NextSubTest();
	string1 = new BmString("abcabcabc");
	string2 = new BmString("abc");
	i = string1->FindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->FindLast(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//FindLast(const char*, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindLast("abc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindLast("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

#ifndef TEST_R5
	//Commented since crashes r5
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindLast((char*)NULL, 3);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(char)
	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindLast('c');
	CPPUNIT_ASSERT(i == 7);
	delete string1;

	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindLast('e');
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//FindLast(char, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindLast("b", 5);
	CPPUNIT_ASSERT(i == 1);
	delete string1;

	NextSubTest();
	string1 = new BmString("abcd abcd");
	i = string1->FindLast('e', 3);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->FindLast("a", 0);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	//IFindFirst(BmString&)
	NextSubTest();
	string1 = new BmString("last but not least");
	string2 = new BmString("st");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("last but not least");
	string2 = new BmString("ST");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString;
	string2 = new BmString("some text");
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("string");
	string2 = new BmString;
	i = string1->IFindFirst(*string2);
	CPPUNIT_ASSERT(i == 0);
	delete string1;
	delete string2;
	
	//IFindFirst(const char*)
	NextSubTest();
	string1 = new BmString("last but not least");
	i = string1->IFindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("LAST BUT NOT least");
	i = string1->IFindFirst("st");
	CPPUNIT_ASSERT(i == 2);
	delete string1;
	
	NextSubTest();
	string1 = new BmString;
	i = string1->IFindFirst("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
#ifndef TEST_R5
	//Commented, since crashes R5
	NextSubTest();
	string1 = new BmString("string");
	i = string1->IFindFirst((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//IFindFirst(BmString&, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->IFindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("AbC");
	i = string1->IFindFirst(*string2, 5);
	CPPUNIT_ASSERT(i == 8);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->IFindFirst(*string2, 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->IFindFirst(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindFirst(const char*, int32)
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->IFindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;

	NextSubTest();
	string1 = new BmString("AbC ABC abC");
	i = string1->IFindFirst("abc", 2);
	CPPUNIT_ASSERT(i == 4);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->IFindFirst("abc", 200);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;

	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->IFindFirst("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
#ifndef TEST_R5
	//IFindLast(BmString&)
	NextSubTest();
	string1 = new BmString("last but not least");
	string2 = new BmString("st");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("laSt but NOT leaSt");
	string2 = new BmString("sT");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == 16);
	delete string1;
	delete string2;
#endif

	NextSubTest();
	string1 = new BmString;
	string2 = new BmString("some text");
	i = string1->IFindLast(*string2);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindLast(const char*)
	NextSubTest();
	string1 = new BmString("last but not least");
	i = string1->IFindLast("st");
	CPPUNIT_ASSERT(i == 16);
	delete string1;

#ifndef TEST_R5	
	NextSubTest();
	string1 = new BmString("laSt but NOT leaSt");
	i = string1->IFindLast("ST");
	CPPUNIT_ASSERT(i == 16);
	delete string1;
#endif
	
	NextSubTest();
	string1 = new BmString;
	i = string1->IFindLast("some text");
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
#ifndef TEST_R5
	//Commented since crashes R5
	NextSubTest();
	string1 = new BmString("string");
	i = string1->IFindLast((char*)NULL);
	CPPUNIT_ASSERT(i == B_BAD_VALUE);
	delete string1;
#endif

	//FindLast(BmString&, int32)
	NextSubTest();
	string1 = new BmString("abcabcabc");
	string2 = new BmString("abc");
	i = string1->IFindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("abcabcabc");
	string2 = new BmString("AbC");
	i = string1->IFindLast(*string2, 7);
	CPPUNIT_ASSERT(i == 3);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("abc abc abc");
	string2 = new BmString("abc");
	i = string1->IFindLast(*string2, -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	delete string2;

	//IFindLast(const char*, int32)
//#ifndef TEST_R5
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->IFindLast("abc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;
//#endif
#ifndef TEST_R5	
	NextSubTest();
	string1 = new BmString("ABc abC aBC");
	i = string1->IFindLast("aBc", 9);
	CPPUNIT_ASSERT(i == 4);
	delete string1;
#endif	
	NextSubTest();
	string1 = new BmString("abc abc abc");
	i = string1->IFindLast("abc", -10);
	CPPUNIT_ASSERT(i == B_ERROR);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("abc def ghi");
	i = string1->IFindLast("abc",4);
	CPPUNIT_ASSERT(i == 0);
	delete string1;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringReplaceTest(void)
{
	BmString *str1;
	
	//&ReplaceFirst(char, char);
	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceFirst('t', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "best string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceFirst('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceLast(char, char);
	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceLast('t', 'w');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test swring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceLast('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&ReplaceAll(char, char, int32);
	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceAll('t', 'i');
	CPPUNIT_ASSERT(strcmp(str1->String(), "iesi siring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceAll('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->ReplaceAll('t', 'i', 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "tesi siring") == 0);
	delete str1;

	//&Replace(char, char, int32, int32)
	NextSubTest();
	str1 = new BmString("she sells sea shells on the sea shore");
	str1->Replace('s', 't', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "she tellt tea thells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString();
	str1->Replace('s', 'x', 12, 32);
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;

	//&ReplaceFirst(const char*, const char*)
	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->ReplaceFirst("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells the shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->ReplaceFirst("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&ReplaceLast(const char*, const char*)
	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->ReplaceLast("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->ReplaceLast("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&ReplaceAll(const char*, const char*, int32)
	NextSubTest();
	str1 = new BmString("abc abc abc");
	str1->ReplaceAll("ab", "abc");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"abcc abcc abcc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->ReplaceAll("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->IReplaceAll("sea", "the", 11);
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;
	
	//&IReplaceFirst(char, char);
	NextSubTest();
	str1 = new BmString("test string");
	str1->IReplaceFirst('t', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "best string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->IReplaceFirst('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&IReplaceLast(char, char);
	NextSubTest();
	str1 = new BmString("test string");
	str1->IReplaceLast('t', 'w');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test swring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->IReplaceLast('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	//&IReplaceAll(char, char, int32);
	NextSubTest();
	str1 = new BmString("TEST string");
	str1->IReplaceAll('t', 'i');
	CPPUNIT_ASSERT(strcmp(str1->String(), "iESi siring") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("test string");
	str1->IReplaceAll('x', 'b');
	CPPUNIT_ASSERT(strcmp(str1->String(), "test string") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("TEST string");
	str1->IReplaceAll('t', 'i', 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "TESi siring") == 0);
	delete str1;

	//&IReplace(char, char, int32, int32)
	NextSubTest();
	str1 = new BmString("She sells Sea shells on the sea shore");
	str1->IReplace('s', 't', 4, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "She tellt tea thells on the sea shore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString();
	str1->IReplace('s', 'x', 12, 32);
	CPPUNIT_ASSERT(strcmp(str1->String(), "") == 0);
	delete str1;

	//&IReplaceFirst(const char*, const char*)
	NextSubTest();
	str1 = new BmString("she sells SeA shells on the seashore");
	str1->IReplaceFirst("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells the shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->IReplaceFirst("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&IReplaceLast(const char*, const char*)
#ifndef TEST_R5	
	NextSubTest();
	str1 = new BmString("she sells sea shells on the SEashore");
	str1->IReplaceLast("sea", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the theshore") == 0);
	delete str1;
#endif
	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->IReplaceLast("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	//&IReplaceAll(const char*, const char*, int32)
	NextSubTest();
	str1 = new BmString("abc ABc aBc");
	str1->IReplaceAll("ab", "abc");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"abcc abcc abcc") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells sea shells on the seashore");
	str1->IReplaceAll("tex", "the");
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells sea shells on the seashore") == 0);
	delete str1;

	NextSubTest();
	str1 = new BmString("she sells SeA shells on the sEashore");
	str1->IReplaceAll("sea", "the", 11);
	CPPUNIT_ASSERT(strcmp(str1->String(),
		"she sells SeA shells on the theshore") == 0);
	delete str1;
	
	//ReplaceSet(const char*, char)
	NextSubTest();
	str1 = new BmString("abc abc abc");
	str1->ReplaceSet("ab", 'x');
	CPPUNIT_ASSERT(strcmp(str1->String(), "xxc xxc xxc") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BmString("abcabcabcbababc");
	str1->ReplaceSet("abc", 'c');
	CPPUNIT_ASSERT(strcmp(str1->String(), "ccccccccccccccc") == 0);
	delete str1;
	
#ifndef TEST_R5
	//ReplaceSet(const char*, const char*)
	NextSubTest();
	str1 = new BmString("abcd abcd abcd");
	str1->ReplaceSet("ad", "da");
	CPPUNIT_ASSERT(strcmp(str1->String(), "dabcda dabcda dabcda") == 0);
	delete str1;
#endif
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringRemoveTest(void)
{
	BmString *string1, *string2;
	
	//Truncate(int32 newLength, bool lazy);
	//lazy = true
	NextSubTest();
	string1 = new BmString("This is a long string");
	string1->Truncate(14, true);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long") == 0);
	CPPUNIT_ASSERT(string1->Length() == 14);
	delete string1;
	
	//lazy = false
	NextSubTest();
	string1 = new BmString("This is a long string");
	string1->Truncate(14, false);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long") == 0);
	CPPUNIT_ASSERT(string1->Length() == 14);
	delete string1;

#ifndef TEST_R5	
	//new length is < 0
	//it crashes r5 implementation, but ours works fine here,
	//in this case, we just truncate to 0
	NextSubTest();
	string1 = new BmString("This is a long string");
	string1->Truncate(-3);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	CPPUNIT_ASSERT(string1->Length() == 0);
	delete string1;	
#endif
	
	//new length is > old length
	NextSubTest();
	string1 = new BmString("This is a long string");
	string1->Truncate(45);
	CPPUNIT_ASSERT(strcmp(string1->String(), "This is a long string") == 0);
	CPPUNIT_ASSERT(string1->Length() == 21);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BmString;
	string1->Truncate(0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	CPPUNIT_ASSERT(string1->Length() == 0);
	delete string1;
	
	//Remove(int32 from, int32 length)
	NextSubTest();
	string1 = new BmString("a String");
	string1->Remove(2, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a ring") == 0);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BmString;
	string1->Remove(2, 1);
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
	
	//from is beyond the end of the string
	NextSubTest();
	string1 = new BmString("a String");
	string1->Remove(20, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a String") == 0);
	delete string1;
	
	//from + length is > Length()
	NextSubTest();
	string1 = new BmString("a String");
	string1->Remove(4, 30);
	CPPUNIT_ASSERT(strcmp(string1->String(), "a String") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("a String");
	string1->Remove(-3, 5);
	CPPUNIT_ASSERT(strcmp(string1->String(), "ing") == 0);
	delete string1;
	
	//RemoveFirst(BmString&)
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("first");
	string1->RemoveFirst(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), " second first") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("noway");
	string1->RemoveFirst(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveLast(Bstring&)
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("first");
	string1->RemoveLast(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second ") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("noway");
	string1->RemoveLast(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveAll(BmString&)
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("first");
	string1->RemoveAll(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), " second ") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string2 = new BmString("noway");
	string1->RemoveAll(*string2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	delete string2;
	
	//RemoveFirst(const char*)
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveFirst("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), " second first") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveFirst("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveFirst((char*)NULL);
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveLast(const char*)
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveLast("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second ") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveLast("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveAll(const char*)
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveAll("first");
	CPPUNIT_ASSERT(strcmp(string1->String(), " second ") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("first second first");
	string1->RemoveAll("noway");
	CPPUNIT_ASSERT(strcmp(string1->String(), "first second first") == 0);
	delete string1;
	
	//RemoveSet(const char*)
	NextSubTest();
	string1 = new BmString("a sentence with (3) (642) numbers (2) in it");
	string1->RemoveSet("()3624 ");
	CPPUNIT_ASSERT(strcmp(string1->String(), "asentencewithnumbersinit") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("a string");
	string1->RemoveSet("1345");
	CPPUNIT_ASSERT(strcmp(string1->String(), "a string") == 0);
	delete string1;
	
	//MoveInto(BmString &into, int32, int32)
	NextSubTest();
	string1 = new BmString("some text");
	string2 = new BmString("string");
	string2->MoveInto(*string1, 3, 2);
	CPPUNIT_ASSERT(strcmp(string1->String(), "in") == 0);
	CPPUNIT_ASSERT(strcmp(string2->String(), "strg") == 0);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("some text");
	string2 = new BmString("string");
	string2->MoveInto(*string1, 0, 200);
	CPPUNIT_ASSERT(strcmp(string1->String(), "string") == 0);
	CPPUNIT_ASSERT(strcmp(string2->String(), "string") == 0);
	delete string1;
	delete string2;
	
	//MoveInto(char *, int32, int32)
	NextSubTest();
	char dest[100];
	memset(dest, 0, 100);
	string1 = new BmString("some text");
	string1->MoveInto(dest, 3, 2);
	CPPUNIT_ASSERT(strcmp(dest, "e ") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "somtext") == 0);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("some text");
	memset(dest, 0, 100);
	string1->MoveInto(dest, 0, 50);
	CPPUNIT_ASSERT(strcmp(dest, "some text") == 0);
	CPPUNIT_ASSERT(strcmp(string1->String(), "some text") == 0);
	delete string1;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringPrependTest(void)
{
	BmString *str1, *str2;
	
	//Prepend(BmString&)
	NextSubTest();
	str1 = new BmString("a String");
	str2 = new BmString("PREPENDED");
	str1->Prepend(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDEDa String") == 0);
	delete str1;
	delete str2;
	
	//Prepend(const char*)
	NextSubTest();
	str1 = new BmString("String");
	str1->Prepend("PREPEND");
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDString") == 0);
	delete str1;
	
	//Prepend(const char*) (NULL)
	NextSubTest();
	str1 = new BmString("String");
	str1->Prepend((char*)NULL);
	CPPUNIT_ASSERT(strcmp(str1->String(), "String") == 0);
	delete str1;
	
	//Prepend(const char*, int32
	NextSubTest();
	str1 = new BmString("String");
	str1->Prepend("PREPENDED", 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREString") == 0);
	delete str1;
	
	//Prepend(BmString&, int32)
	NextSubTest();
	str1 = new BmString("String");
	str2 = new BmString("PREPEND", 4);
	str1->Prepend(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPString") == 0);
	delete str1;
	delete str2;
	
	//Prepend(char, int32)
	NextSubTest();
	str1 = new BmString("aString");
	str1->Prepend('c', 4);
	CPPUNIT_ASSERT(strcmp(str1->String(), "ccccaString") == 0);
	delete str1;
	
	//String was empty
	NextSubTest();
	str1 = new BmString;
	str1->Prepend("PREPENDED");
	CPPUNIT_ASSERT(strcmp(str1->String(), "PREPENDED") == 0);
	delete str1;

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringInsertTest(void)
{
	BmString *str1, *str2;
	
	//&Insert(const char *, int32 pos);
	NextSubTest();
	str1 = new BmString("String");
	str1->Insert("INSERTED", 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "StrINSERTEDing") == 0);
	delete str1;
	
	//This test crashes both implementations
	//NextSubTest();
	//str1 = new BmString("String");
	//str1->Insert("INSERTED", 10);
	//CPPUNIT_ASSERT(strcmp(str1->String(), "string") == 0);
	//delete str1;
	
	NextSubTest();
	str1 = new BmString;
	str1->Insert("INSERTED", -1);
	CPPUNIT_ASSERT(strcmp(str1->String(), "NSERTED") == 0);
	delete str1;
	
	//&Insert(const char *, int32 length, int32 pos);
	NextSubTest();
	str1 = new BmString("string");
	str1->Insert("INSERTED", 2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	delete str1;
	
	//This test crashes both implementations
	//NextSubTest();
	//str1 = new BmString("string");
	//str1->Insert("INSERTED", 2, 30);
	//CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	//delete str1;
	
	NextSubTest();
	str1 = new BmString("string");
	str1->Insert("INSERTED", 10, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINSERTEDring") == 0);
	delete str1;
	
	//&Insert(const char *, int32 fromOffset, int32 length, int32 pos);
	NextSubTest();
	str1 = new BmString("string");
	str1->Insert("INSERTED", 4, 30, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stRTEDring") == 0);
	delete str1;
	
	//Insert(char c, int32 count, int32 pos)
	NextSubTest();
	str1 = new BmString("string");
	str1->Insert('P', 5, 3);
	CPPUNIT_ASSERT(strcmp(str1->String(), "strPPPPPing") == 0);
	delete str1;
	
	//Insert(BmString&)
	NextSubTest();
	str1 = new BmString("string");
	str2 = new BmString("INSERTED");
	str1->Insert(*str2, 0);
	CPPUNIT_ASSERT(strcmp(str1->String(), "INSERTEDstring") == 0);
	delete str1;
	delete str2;
	
	NextSubTest();
	str1 = new BmString("string");
	str1->Insert(*str1, 0);
	CPPUNIT_ASSERT(strcmp(str1->String(), "string") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BmString;
	str2 = new BmString("INSERTED");
	str1->Insert(*str2, -1);
	CPPUNIT_ASSERT(strcmp(str1->String(), "NSERTED") == 0);
	delete str1;
	delete str2;
	
	//&Insert(BmString &, int32 length, int32 pos);
	NextSubTest();
	str1 = new BmString("string");
	str2 = new BmString("INSERTED");
	str1->Insert(*str2, 2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stINring") == 0);
	delete str1;
	delete str2;
		
	//&Insert(BmString&, int32 fromOffset, int32 length, int32 pos);
	NextSubTest();
	str1 = new BmString("string");
	str2 = new BmString("INSERTED");
	str1->Insert(*str2, 4, 30, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "stRTEDring") == 0);
	delete str1;
	delete str2;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringFormatAppendTest(void)
{
	BmString *string, *string2;
	
	//operator<<(const char *);
	NextSubTest();
	string = new BmString("some");
	*string << " ";
	*string << "text";
	CPPUNIT_ASSERT(strcmp(string->String(), "some text") == 0);
	delete string;
	
	//operator<<(const BmString &);
	NextSubTest();
	string = new BmString("some ");
	string2 = new BmString("text");
	*string << *string2;
	CPPUNIT_ASSERT(strcmp(string->String(), "some text") == 0);
	delete string;
	delete string2;
	
	//operator<<(char);
	NextSubTest();
	string = new BmString("str");
	*string << 'i' << 'n' << 'g';
	CPPUNIT_ASSERT(strcmp(string->String(), "string") == 0);
	delete string;
		
	//operator<<(int);
	NextSubTest();
	string = new BmString("level ");
	*string << (int)42;
	CPPUNIT_ASSERT(strcmp(string->String(), "level 42") == 0);
	delete string;
	
	NextSubTest();
	string = new BmString("error ");
	*string << (int)-1;
	CPPUNIT_ASSERT(strcmp(string->String(), "error -1") == 0);
	delete string;
	
	//operator<<(unsigned int);
	NextSubTest();
	string = new BmString("number ");
	*string << (unsigned int)296;
	CPPUNIT_ASSERT(strcmp(string->String(), "number 296") == 0);
	delete string;
	
	//operator<<(uint32);
	NextSubTest();
	string = new BmString;
	*string << (uint32)102456;
	CPPUNIT_ASSERT(strcmp(string->String(), "102456") == 0);
	delete string;

	//operator<<(int32);
	NextSubTest();
	string = new BmString;
	*string << (int32)112456;
	CPPUNIT_ASSERT(strcmp(string->String(), "112456") == 0);
	delete string;

	NextSubTest();
	string = new BmString;
	*string << (int32)-112475;
	CPPUNIT_ASSERT(strcmp(string->String(), "-112475") == 0);
	delete string;

	//operator<<(uint64);
	NextSubTest();
	string = new BmString;
	*string << (uint64)1145267987;
	CPPUNIT_ASSERT(strcmp(string->String(), "1145267987") == 0);
	delete string;

	//operator<<(int64);
	NextSubTest();
	string = new BmString;
	*string << (int64)112456;
	CPPUNIT_ASSERT(strcmp(string->String(), "112456") == 0);
	delete string;

	NextSubTest();
	string = new BmString;
	*string << (int64)-112475;
	CPPUNIT_ASSERT(strcmp(string->String(), "-112475") == 0);
	delete string;

	//operator<<(float);
	NextSubTest();
	string = new BmString;
	*string << (float)34.542;
	CPPUNIT_ASSERT(strcmp(string->String(), "34.54") == 0);
	delete string;
	
	//Misc test
	NextSubTest();
	BmString s;
	s << "This" << ' ' << "is" << ' ' << 'a' << ' ' << "test" << ' ' << "sentence";		
	CPPUNIT_ASSERT(strcmp(s.String(), "This is a test sentence") == 0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringEscapeTest(void)
{
	BmString *string1;
	
	//CharacterEscape(char*, char)
	NextSubTest();
	string1 = new BmString("abcdefghi");
	string1->CharacterEscape("acf", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/ab/cde/fghi") == 0);
	delete string1;
	
	//BmString is null
	NextSubTest();
	string1 = new BmString;
	string1->CharacterEscape("abc", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
	
	//BmString doesn't contain wanted characters
	NextSubTest();
	string1 = new BmString("abcdefghi");
	string1->CharacterEscape("z34", 'z');
	CPPUNIT_ASSERT(strcmp(string1->String(), "abcdefghi") == 0);
	delete string1;
	
	//CharacterEscape(char *, char*, char)
	NextSubTest();
	string1 = new BmString("something");
	string1->CharacterEscape("newstring", "esi", '0');
	CPPUNIT_ASSERT(strcmp(string1->String(), "n0ew0str0ing") == 0);
	delete string1;

#ifndef TEST_R5	
	//assigned string is NULL
	//it crashes r5 implementation, but not ours :)
	NextSubTest();
	string1 = new BmString("something");
	string1->CharacterEscape((char*)NULL, "ei", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
#endif
	
	//String was empty
	NextSubTest();
	string1 = new BmString;
	string1->CharacterEscape("newstring", "esi", '0');
	CPPUNIT_ASSERT(strcmp(string1->String(), "n0ew0str0ing") == 0);
	delete string1;
	
	//CharacterDeescape(char)
	NextSubTest();
	string1 = new BmString("/a/nh/g/bhhgy/fgtuhjkb/");
	string1->CharacterDeescape('/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "anhgbhhgyfgtuhjkb") == 0);
	delete string1;
	
	//String was empty
	NextSubTest();
	string1 = new BmString;
	string1->CharacterDeescape('/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;	
	
	//String doesn't contain character to escape
	NextSubTest();
	string1 = new BmString("/a/nh/g/bhhgy/fgtuhjkb/");
	string1->CharacterDeescape('-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/a/nh/g/bhhgy/fgtuhjkb/") == 0);
	delete string1;
	
	//CharacterDeescape(char* original, char)
	NextSubTest();
	string1 = new BmString("oldString");
	string1->CharacterDeescape("-ne-ws-tri-ng-", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "newstring") == 0);
	delete string1;	
	
	//String was empty
	NextSubTest();
	string1 = new BmString;
	string1->CharacterDeescape("new/str/ing", '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "newstring") == 0);
	delete string1;	

#ifndef TEST_R5	
	//assigned string is empty
	//it crashes r5 implementation, but not ours :)
	NextSubTest();
	string1 = new BmString("pippo");
	string1->CharacterDeescape((char*)NULL, '/');
	CPPUNIT_ASSERT(strcmp(string1->String(), "") == 0);
	delete string1;
#endif	

	//String doesn't contain character to escape
	NextSubTest();
	string1 = new BmString("Old");
	string1->CharacterDeescape("/a/nh/g/bhhgy/fgtuhjkb/", '-');
	CPPUNIT_ASSERT(strcmp(string1->String(), "/a/nh/g/bhhgy/fgtuhjkb/") == 0);
	delete string1;	
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringConstructionTest(void)
{
	BmString *string;
	const char *str = "Something";
	
	//BmString()
	NextSubTest();
	string = new BmString;
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	//BmString(const char*)
	NextSubTest();
	string = new BmString(str);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT(string->Length() == (int32)strlen(str));
	delete string;
	
	//BmString(NULL)
	NextSubTest();
	string = new BmString(NULL);
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	CPPUNIT_ASSERT(string->Length() == 0);
	delete string;
	
	//BmString(BmString&)
	NextSubTest();
	BmString anotherString("Something Else");
	string = new BmString(anotherString);
	CPPUNIT_ASSERT(strcmp(string->String(), anotherString.String()) == 0);
	CPPUNIT_ASSERT(string->Length() == anotherString.Length());
	delete string;
	
	//BmString(const char*, int32)
	NextSubTest();
	string = new BmString(str, 5);
	CPPUNIT_ASSERT(strcmp(string->String(), str) != 0);
	CPPUNIT_ASSERT(strncmp(string->String(), str, 5) == 0);
	CPPUNIT_ASSERT(string->Length() == 5);
	delete string;
	
	NextSubTest();
	string = new BmString(str, 255);
	CPPUNIT_ASSERT(strcmp(string->String(), str) == 0);
	CPPUNIT_ASSERT(string->Length() == (int32)strlen(str));
	delete string;	
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringCompareTest(void)
{
	BmString *string1, *string2;
	
	//operator<(const BmString &) const;
	NextSubTest();
	string1 = new BmString("11111_a");
	string2 = new BmString("22222_b");
	CPPUNIT_ASSERT(*string1 < *string2);
	delete string1;
	delete string2;
	
	//operator<=(const BmString &) const;
	NextSubTest();
	string1 = new BmString("11111_a");
	string2 = new BmString("22222_b");
	CPPUNIT_ASSERT(*string1 <= *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("11111");
	string2 = new BmString("11111");
	CPPUNIT_ASSERT(*string1 <= *string2);
	delete string1;
	delete string2;
	
	//operator==(const BmString &) const;
	NextSubTest();
	string1 = new BmString("string");
	string2 = new BmString("string");
	CPPUNIT_ASSERT(*string1 == *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("text");
	string2 = new BmString("string");
	CPPUNIT_ASSERT((*string1 == *string2) == false);
	delete string1;
	delete string2;
	
	//operator>=(const BmString &) const;
	NextSubTest();
	string1 = new BmString("BBBBB");
	string2 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 >= *string2);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("11111");
	string2 = new BmString("11111");
	CPPUNIT_ASSERT(*string1 >= *string2);
	delete string1;
	delete string2;
	
	//operator>(const BmString &) const;
	NextSubTest();
	string1 = new BmString("BBBBB");
	string2 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 > *string2);
	delete string1;
	delete string2;
	
	//operator!=(const BmString &) const;
	NextSubTest();
	string1 = new BmString("string");
	string2 = new BmString("string");
	CPPUNIT_ASSERT((*string1 != *string2) == false);
	delete string1;
	delete string2;
	
	NextSubTest();
	string1 = new BmString("text");
	string2 = new BmString("string");
	CPPUNIT_ASSERT(*string1 != *string2);
	delete string1;
	delete string2;
	
	//operator<(const char *) const;
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 < "BBBBB");
	delete string1;
	
	//operator<=(const char *) const;
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 <= "BBBBB");
	CPPUNIT_ASSERT(*string1 <= "AAAAA");
	delete string1;
	
	//operator==(const char *) const;
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 == "AAAAA");
	delete string1;
	
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT((*string1 == "BBBB") == false);
	delete string1;
	
	//operator>=(const char *) const;
	NextSubTest();
	string1 = new BmString("BBBBB");
	CPPUNIT_ASSERT(*string1 >= "AAAAA");
	CPPUNIT_ASSERT(*string1 >= "BBBBB");
	delete string1;
	
	//operator>(const char *) const;
	NextSubTest();
	string1 = new BmString("BBBBB");
	CPPUNIT_ASSERT(*string1 > "AAAAA");
	delete string1;
	
	//operator!=(const char *) const;
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT((*string1 != "AAAAA") == false);
	delete string1;
	
	NextSubTest();
	string1 = new BmString("AAAAA");
	CPPUNIT_ASSERT(*string1 != "BBBB");
	delete string1;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringCharAccessTest(void)
{
	BmString string("A simple string");
	
	//operator[]
	NextSubTest();
	CPPUNIT_ASSERT(string[0] == 'A');
	CPPUNIT_ASSERT(string[1] == ' ');

	//&operator[]
	NextSubTest();
	string[0] = 'a';
	CPPUNIT_ASSERT(strcmp(string.String(), "a simple string") == 0);
	
	//ByteAt(int32)
	NextSubTest();
	CPPUNIT_ASSERT(string.ByteAt(-10) == 0);
	CPPUNIT_ASSERT(string.ByteAt(200) == 0);
	CPPUNIT_ASSERT(string.ByteAt(1) == ' ');
	CPPUNIT_ASSERT(string.ByteAt(7) == 'e');
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringCaseTest(void)
{
	BmString *string;
	
	//Capitalize
	NextSubTest();
	string = new BmString("this is a sentence");
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "This is a sentence") == 0);
	delete string;
	
	NextSubTest();
	string = new BmString("134this is a sentence");
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "134this is a sentence") == 0);
	delete string;

	NextSubTest();
	string = new BmString;
	string->Capitalize();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//ToLower
	NextSubTest();
	string = new BmString("1a2B3c4d5e6f7G");
	string->ToLower();
	CPPUNIT_ASSERT(strcmp(string->String(), "1a2b3c4d5e6f7g") == 0);
	delete string;
	
	NextSubTest();
	string = new BmString;
	string->ToLower();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//ToUpper
	NextSubTest();
	string = new BmString("1a2b3c4d5E6f7g");
	string->ToUpper();
	CPPUNIT_ASSERT(strcmp(string->String(), "1A2B3C4D5E6F7G") == 0);
	delete string;
	
	NextSubTest();
	string = new BmString;
	string->ToUpper();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;
	
	//CapitalizeEachWord
	NextSubTest();
	string = new BmString("each wOrd 3will_be >capiTalized");
	string->CapitalizeEachWord();
	CPPUNIT_ASSERT(strcmp(string->String(), "Each Word 3Will_Be >Capitalized") == 0);
	delete string;
	
	NextSubTest();
	string = new BmString;
	string->CapitalizeEachWord();
	CPPUNIT_ASSERT(strcmp(string->String(), "") == 0);
	delete string;		
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringAssignTest(void)
{
	//=(BmString&)
	NextSubTest();
	BmString string;
	BmString string2("Something");
	string = string2;
	CPPUNIT_ASSERT(strcmp(string.String(), string2.String()) == 0);
	CPPUNIT_ASSERT(strcmp(string.String(), "Something") == 0);
	
	//=(const char*)
	NextSubTest();
	BmString *str = new BmString();
	*str = "Something Else";
	CPPUNIT_ASSERT(strcmp(str->String(), "Something Else") == 0);
	delete str;
	
	//char ptr is NULL
	NextSubTest();
	char *s = NULL;
	str = new BmString;
	*str = s;
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	//SetTo(const char *) (NULL)
	NextSubTest();
	str = new BmString;
	str->SetTo(s);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	NextSubTest();
	str = new BmString;
	str->SetTo("BLA");
	CPPUNIT_ASSERT(strcmp(str->String(), "BLA") == 0);
	delete str;
	
	//SetTo(BmString&)
	NextSubTest();
	str = new BmString;
	str->SetTo(string);
	CPPUNIT_ASSERT(strcmp(str->String(), string.String()) == 0);
	delete str;
	
	//SetTo(char, int32)
	NextSubTest();
	str = new BmString;
	str->SetTo('C', 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "CCCCCCCCCC") == 0);
	delete str;
	
	NextSubTest();
	str = new BmString("ASDSGAFA");
	str->SetTo('C', 0);
	CPPUNIT_ASSERT(strcmp(str->String(), "") == 0);
	delete str;
	
	//SetTo(const char*, int32)
	NextSubTest();
	str = new BmString;
	str->SetTo("ABC", 10);
	CPPUNIT_ASSERT(strcmp(str->String(), "ABC") == 0);
	delete str;
	
	//Adopt(BmString&)
	NextSubTest();
	const char *oldString2 = string2.String();
	str = new BmString;
	str->Adopt(string2);
	CPPUNIT_ASSERT(strcmp(str->String(), oldString2) == 0);
	CPPUNIT_ASSERT(strcmp(string2.String(), "") == 0);
	delete str;
	
	NextSubTest();
	BmString newstring("SomethingElseAgain");
	const char *oldString = newstring.String();
	str = new BmString;
	str->Adopt(newstring, 2);
	CPPUNIT_ASSERT(strncmp(str->String(), oldString, 2) == 0);
	CPPUNIT_ASSERT(str->Length() == 2);
	CPPUNIT_ASSERT(strcmp(newstring.String(), "") == 0);
	delete str;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringAppendTest(void)
{
	BmString *str1, *str2;
	
	//+=(BmString&)
	NextSubTest();
	str1 = new BmString("BASE");
	str2 = new BmString("APPENDED");
	*str1 += *str2;
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	//+=(const char *)
	NextSubTest();
	str1 = new BmString("Base");
	*str1 += "APPENDED";
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BmString;
	*str1 += "APPENDEDTONOTHING";
	CPPUNIT_ASSERT(strcmp(str1->String(), "APPENDEDTONOTHING") == 0);
	delete str1;
	
	//char pointer is NULL
	NextSubTest();
	char *tmp = NULL;
	str1 = new BmString("Base");
	*str1 += tmp;
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	//+=(char)
	NextSubTest();
	str1 = new BmString("Base");
	*str1 += 'C';
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseC") == 0);
	delete str1;
	
	//Append(BmString&)
	NextSubTest();
	str1 = new BmString("BASE");
	str2 = new BmString("APPENDED");
	str1->Append(*str2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAPPENDED") == 0);
	delete str1;
	delete str2;
	
	//Append(const char*)
	NextSubTest();
	str1 = new BmString("Base");
	str1->Append("APPENDED");
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	delete str1;
	
	NextSubTest();
	str1 = new BmString;
	str1->Append("APPENDEDTONOTHING");
	CPPUNIT_ASSERT(strcmp(str1->String(), "APPENDEDTONOTHING") == 0);
	delete str1;
	
	//char ptr is NULL
	NextSubTest();
	str1 = new BmString("Base");
	str1->Append(tmp);
	CPPUNIT_ASSERT(strcmp(str1->String(), "Base") == 0);
	delete str1;
	
	//Append(BmString&, int32)
	NextSubTest();
	str1 = new BmString("BASE");
	str2 = new BmString("APPENDED");
	str1->Append(*str2, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BASEAP") == 0);
	delete str1;
	delete str2;
	
	//Append(const char*, int32)
	NextSubTest();
	str1 = new BmString("Base");
	str1->Append("APPENDED", 40);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseAPPENDED") == 0);
	CPPUNIT_ASSERT(str1->Length() == (int32)strlen("BaseAPPENDED"));
	delete str1;
	
	//char ptr is NULL
	NextSubTest();
	str1 = new BmString("BLABLA");
	str1->Append(tmp, 2);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BLABLA") == 0);
	delete str1;
	
	//Append(char, int32)
	NextSubTest();
	str1 = new BmString("Base");
	str1->Append('C', 5);
	CPPUNIT_ASSERT(strcmp(str1->String(), "BaseCCCCC") == 0);
	delete str1;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringAccessTest(void)
{
	//CountChars(), Length(), String()
	NextSubTest();
	BmString string("Something"B_UTF8_ELLIPSIS);
	CPPUNIT_ASSERT(string.CountChars() == 10);
	CPPUNIT_ASSERT(string.Length() == (int32)strlen(string.String()));

	NextSubTest();
	BmString string2("ABCD");
	CPPUNIT_ASSERT(string2.CountChars() == 4);
	CPPUNIT_ASSERT(string2.Length() == (int32)strlen(string2.String()));

	NextSubTest();
	static char s[64];
	strcpy(s, B_UTF8_ELLIPSIS);
	strcat(s, B_UTF8_SMILING_FACE);
	BmString string3(s);
	CPPUNIT_ASSERT(string3.CountChars() == 2);	
	CPPUNIT_ASSERT(string3.Length() == (int32)strlen(string3.String()));
	
	//An empty string
	NextSubTest();
	BmString empty;
	CPPUNIT_ASSERT(strcmp(empty.String(), "") == 0);
	CPPUNIT_ASSERT(empty.Length() == 0);
	CPPUNIT_ASSERT(empty.CountChars() == 0);

	//Truncate the string at end so we are left with an invalid
	//UTF8 character
	NextSubTest();
	BmString invalid("some text with utf8 characters"B_UTF8_ELLIPSIS);
	invalid.Truncate(invalid.Length() -1);
	CPPUNIT_ASSERT(invalid.CountChars() == 31);

	//LockBuffer(int32) and UnlockBuffer(int32)
	NextSubTest();
	BmString locked("a string");
	char *ptrstr = locked.LockBuffer(20);
	CPPUNIT_ASSERT(strcmp(ptrstr, "a string") == 0);
	strcat(ptrstr, " to be locked");
	locked.UnlockBuffer();
	CPPUNIT_ASSERT(strcmp(ptrstr, "a string to be locked") == 0);
	
	NextSubTest();
	BmString locked2("some text");
	char *ptr = locked2.LockBuffer(3);
	CPPUNIT_ASSERT(strcmp(ptr, "some text") == 0);
	locked2.UnlockBuffer(4);
	CPPUNIT_ASSERT(strcmp(locked2.String(), "some") == 0);
	CPPUNIT_ASSERT(locked2.Length() == 4);
	
/* [zooey, 2003-09-21]:
		this test seems bogus, since I think it is relying on whether or not
		realloc() fills the allocated area with zeros or not. On R5 it doesn't,
		under obos it may be different.
	NextSubTest();
	BmString emptylocked;
	ptr = emptylocked.LockBuffer(10);
	CPPUNIT_ASSERT(strcmp(ptr, "") == 0);
	strcat(ptr, "pippo");
	emptylocked.UnlockBuffer();
	CPPUNIT_ASSERT(strcmp(emptylocked.String(), "pippo") == 0);
*/
	
	// LockBuffer(0) and UnlockBuffer(-1) on a zero lenght string
#ifndef TEST_R5	
	NextSubTest();
	BmString crashesR5;
	ptr = crashesR5.LockBuffer(0);
	crashesR5.UnlockBuffer(-1);
	CPPUNIT_ASSERT(strcmp(crashesR5.String(), "") == 0);
#endif
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
StringTest::StringBeamExtensionsTest(void)
{
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
