/*
	SieveTest.cpp
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

#include "SieveTest.h"

#include "BmSieveFilter.h"
#include "BmMail.h"

static BMessage msg;
static BmSieveFilter filter("TestFilter",&msg);
static BmString dummyText;
static BmString mailAcc("testacc@test.org");
static BmRef<BmMail> mail;
static BmString mailId;
static BmMsgContext* msgContext = NULL;
static BmString targetFolder;
static BmString targetStatus;
static BmString targetIdentity;
static BmString targetMsg;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void 
SetupMsgContext(BmString text)
{
	try {
		if (!mail)
			mail = new BmMail(text,mailAcc);
		else
			mail->SetTo(text,mailAcc);
		static int count=0;
		mailId = BmString("testmail-")<<++count;
		delete msgContext;
		msgContext = new BmMsgContext(mail->RawText(), mailId, false, "Read", mailAcc);
		mail->Header()->GetAllFieldValues( *msgContext);
	} catch( BM_error& e) {
		cerr << e.what() << endl;
		throw;
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static bool 
CompErr()
{
	cerr << filter.ErrorString().String() << endl;
	return false;
}

enum {
	RES_KEEP = 		0,
	RES_STOP = 		1<<0,
	RES_IDENTITY = 1<<1,
	RES_STATUS = 	1<<2,
	RES_FILEINTO = 1<<3,
	RES_REJECT = 	1<<4,
	RES_TRASH = 	1<<5,
	RES_MAX = 		1<<31
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static int32
Result()
{
	int32 res = RES_KEEP;
	if (msgContext->stopProcessing) {
		res |= RES_STOP;
		msgContext->stopProcessing = false;
	}
	if (msgContext->identity.Length()) {
		targetIdentity = msgContext->identity;
		res |= RES_IDENTITY;
		msgContext->identity = "";
	}
	if (msgContext->status != "Read") {
		targetStatus = msgContext->status;
		res |= RES_STATUS;
		msgContext->status = "Read";
	}
	if (msgContext->folderName.Length()) {
		targetFolder = msgContext->folderName;
		res |= RES_FILEINTO;
		msgContext->folderName = "";
	}
	if (msgContext->rejectMsg.Length()) {
		targetMsg = msgContext->rejectMsg;
		res |= RES_REJECT;
		msgContext->rejectMsg = "";
	}
	if (msgContext->moveToTrash) {
		res |= RES_TRASH;
		msgContext->moveToTrash = false;
	}
	return res;
}




// setUp
void
SieveTest::setUp()
{
	inherited::setUp();
	SetupMsgContext("\
Date: Mon, 25 Feb 2003 08:51:06 -0500\r\n\
Received: through3\r\n\
Received: through2\r\n\
Received: through1\r\n\
From: them\r\n\
To: you\r\n\
Cc: cc1, the_cc2 <cc2@test.org>\r\n\
Cc: cc3\r\n\
To: you\r\n\
Subject: A simple testmail for SIEVE filtering\r\n\
X-Priority: 3\r\n\
\r\n\
blah (just to have a body)\
");
}
	
// tearDown
void
SieveTest::tearDown()
{
	mail = NULL;
	inherited::tearDown();
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::BasicActionTest(void)
{
	// implicit keep
	NextSubTest();
	filter.mContent = "";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// explicit keep
	NextSubTest();
	filter.mContent = "keep;";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// implicit keep after stop
	NextSubTest();
	filter.mContent = "stop;";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// no implicit keep after discard and stop
	NextSubTest();
	filter.mContent = "discard; stop;";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// ignore redirect, since we are a mail-client, not a server 
	// (we do keep instead)
	NextSubTest();
	filter.mContent = "redirect \"test2@test.org\"; stop;";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::ExtendedActionTest(void)
{
	// fileinto needs to be required
	NextSubTest();
	filter.mContent = "\
fileinto \"a_folder\";\n\
";
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("fileinto not required") != B_ERROR);

	// reject
	NextSubTest();
	filter.mContent = "\
require [\"fileinto\"];\n\
fileinto \"a_folder\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_FILEINTO && targetFolder == "a_folder");

	// reject needs to be required
	NextSubTest();
	filter.mContent = "\
reject \"rejected\";\n\
";
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("reject not required") != B_ERROR);

	// reject
	NextSubTest();
	filter.mContent = "\
require [\"reject\"];\n\
reject \"rejected\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_REJECT && targetMsg == "rejected");

	// notify needs to be required
	NextSubTest();
	filter.mContent = "notify;";
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("notify not required") != B_ERROR);

	// notify
	NextSubTest();
	filter.mContent = "\
require \"notify\";\n\
notify :method \"kind\" :options \"options\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	
	// setting the identity (through notify)
	NextSubTest();
	filter.mContent = "\
require \"notify\";\n\
notify :method \"BeamSetIdentity\" :options \"test-identity\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_IDENTITY 
						 && targetIdentity == "test-identity");
	
	// setting the status (through notify)
	NextSubTest();
	filter.mContent = "\
require \"notify\";\n\
notify :method \"BeamSetStatus\" :options \"test-status\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_STATUS
						 && targetStatus == "test-status");
	
	// setting the stop-processing flag (through notify)
	NextSubTest();
	filter.mContent = "\
require \"notify\";\n\
notify :method \"BeamStopProcessing\";\n\
";
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_STOP);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::BasicTestsTest(void)
{
	// true
	NextSubTest();
	filter.mContent = "if true { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// false
	NextSubTest();
	filter.mContent = "if false { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// not
	NextSubTest();
	filter.mContent = "if not true { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if not not not false { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// anyof
	NextSubTest();
	filter.mContent = "if anyof (false, false) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if anyof (true, false) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.mContent = "if anyof (false, true) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.mContent = "if anyof (true, true) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// allof
	NextSubTest();
	filter.mContent = "if allof (false, false) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if allof (true, false) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if allof (false, true) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if allof (true, true) { keep; } else { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// size
	NextSubTest();
	filter.mContent = "if size :over 100 { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = "if size :under 10000 { discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.mContent = BmString("if not anyof (\n")
		<< "size :over " << mail->RawText().Length() << ",\n"
		<< "size :under " << mail->RawText().Length() << ")\n"
		<< "{ discard; }";
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::AddressTestTest(void)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::ExistsTestTest(void)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::HeaderTestTest(void)
{
}

