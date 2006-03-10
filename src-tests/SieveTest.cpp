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

static BmString mailText("\
Date: Mon, 25 Feb 2003 08:51:06 -0500\r\n\
Received: through3\r\n\
Received: through2\r\n\
Received: through1\r\n\
From: them\r\n\
To: you+sieve@test.org\r\n\
Cc: cc1, the_cc2 <cc2@test.org>\r\n\
Cc: <cc3>\r\n\
To: you\r\n\
Sender: <test+@test.org>\r\n\
Subject: A simple testmail for SIEVE filtering\r\n\
X-Priority: 3\r\n\
X-Empty: \r\n\
\r\n\
blah (just to have a body)\
");

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
		msgContext = new BmMsgContext();
		msgContext->mail = mail.Get();
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
	bool stopProcessing = msgContext->GetBool("StopProcessing");
	if (stopProcessing) {
		res |= RES_STOP;
	}
	BmString newIdentity = msgContext->GetString("Identity");
	if (newIdentity.Length()) {
		targetIdentity = newIdentity;
		res |= RES_IDENTITY;
	}
	BmString newStatus = msgContext->GetString("Status");
	if (newStatus.Length()) {
		targetStatus = newStatus;
		res |= RES_STATUS;
	}
	BmString newFolderName = msgContext->GetString("FolderName");
	if (newFolderName.Length()) {
		targetFolder = newFolderName;
		res |= RES_FILEINTO;
	}
	BmString rejectMsg = msgContext->GetString("RejectMsg");
	if (rejectMsg.Length()) {
		targetMsg = rejectMsg;
		res |= RES_REJECT;
	}
	bool moveToTrash = msgContext->GetBool("MoveToTrash");
	if (moveToTrash) {
		res |= RES_TRASH;
	}
	msgContext->ResetData();
	msgContext->ResetChanges();
//	cerr << res << endl;
	return res;
}




// setUp
void
SieveTest::setUp()
{
	inherited::setUp();
	SetupMsgContext(mailText);
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
	filter.Content( "");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// explicit keep
	NextSubTest();
	filter.Content( "keep;");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// implicit keep after stop
	NextSubTest();
	filter.Content( "stop;");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// no implicit keep after discard and stop
	NextSubTest();
	filter.Content( "discard; stop;");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// ignore redirect, since we are a mail-client, not a server 
	// (we do keep instead)
	NextSubTest();
	filter.Content( "redirect \"test2@test.org\"; stop;");
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
	filter.Content( "\
fileinto \"a_folder\";\n\
");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("fileinto not required") != B_ERROR);

	// reject
	NextSubTest();
	filter.Content( "\
require [\"fileinto\"];\n\
fileinto \"a_folder\";\n\
");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_FILEINTO && targetFolder == "a_folder");

	// reject needs to be required
	NextSubTest();
	filter.Content( "\
reject \"rejected\";\n\
");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("reject not required") != B_ERROR);

	// reject
	NextSubTest();
	filter.Content( "\
require [\"reject\"];\n\
reject \"rejected\";\n\
");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_REJECT && targetMsg == "rejected");

	// notify needs to be required
	NextSubTest();
	filter.Content( "notify;");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("notify not required") != B_ERROR);

	// notify
	NextSubTest();
	filter.Content( "\
require \"notify\";\n\
notify :method \"kind\" :options \"options\";\n\
");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	
	// setting the identity (through notify)
	NextSubTest();
	filter.Content( "\
require \"notify\";\n\
notify :method \"BeamSetIdentity\" :options \"test-identity\";\n\
");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_IDENTITY 
						 && targetIdentity == "test-identity");
	
	// setting the status (through notify)
	NextSubTest();
	filter.Content( "\
require \"notify\";\n\
notify :method \"BeamSetStatus\" :options \"test-status\";\n\
");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_STATUS
						 && targetStatus == "test-status");
	
	// setting the stop-processing flag (through notify)
	NextSubTest();
	filter.Content( "\
require \"notify\";\n\
notify :method \"BeamStopProcessing\";\n\
");
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
	filter.Content( "if true { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// false
	NextSubTest();
	filter.Content( "if false { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// not
	NextSubTest();
	filter.Content( "if not true { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content( "if not not not false { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// anyof
	NextSubTest();
	filter.Content( "if anyof (false, false) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content( "if anyof (true, false) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content( "if anyof (false, true) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("if anyof (true, true) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// allof
	NextSubTest();
	filter.Content("if allof (false, false) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content("if allof (true, false) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content("if allof (false, true) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content("if allof (true, true) { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// size
	NextSubTest();
	filter.Content("if size :over 100 { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content("if size :under 10000 { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	//
	NextSubTest();
	filter.Content(BmString("if not anyof (\n")
		<< "size :over " << mail->RawText().Length() << ",\n"
		<< "size :under " << mail->RawText().Length() << ")\n"
		<< "{ discard; }");
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
	// simple :is comparison 
	NextSubTest();
	filter.Content("\
		if address \
			:all \
			:is \"To\" \"you+sieve@test.org\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// default addresspart is :all
	NextSubTest();
	filter.Content("\
		if address \
			:all \
			:is \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// default match-type should be :is
	NextSubTest();
	filter.Content("\
		if address \
			:localpart \
			\"From\" \"the\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// a nonexisting header never :is ""
	NextSubTest();
	filter.Content("\
		if address \
			:is \"X-frobnicle\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// :localpart addr-part comparison 
	NextSubTest();
	filter.Content("\
		if address \
			:localpart \
			:is \"To\" \"you+sieve\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// :domain addr-part comparison 
	NextSubTest();
	filter.Content("\
		if address \
			:domain \
			:is \"To\" \"test.org\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// subaddress-extension (:detail, :user) needs to be required
	NextSubTest();
	filter.Content("\
		if address \
			:detail \
			:is \"To\" \"sieve\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("subaddress not required") != B_ERROR);
	// :detail addr-part comparison 
	NextSubTest();
	filter.Content("\
		require \"subaddress\"; \
		if address \
			:detail \
			:is \"To\" \"sieve\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// :detail :is "" for empty detail
	NextSubTest();
	filter.Content("\
		require \"subaddress\"; \
		if address \
			:detail \
			:is \"Sender\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// :detail never :is "" if not separator (+) is found
	NextSubTest();
	filter.Content("\
		require \"subaddress\"; \
		if address \
			:detail \
			:is \"From\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// :user addr-part comparison 
	NextSubTest();
	filter.Content("\
		require \"subaddress\"; \
		if address \
			:user \
			:is \"To\" \"you\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"subaddress\"; \
		if address \
			:user \
			:is \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// an existing, empty header is no address (so never :is "")
	NextSubTest();
	filter.Content("\
		if address \
			:is \"X-Empty\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// yield true if any of multiple keys matches
	NextSubTest();
	filter.Content("\
		if address :is \"From\" [\"me\", \"myself\", \"I\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		if address :is \"From\" [\"me\", \"myself\", \"them\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// yield true if any of multiple headers matches
	NextSubTest();
	filter.Content("\
		if address :is [\"X-frobnicle\", \"X-shratful\"] \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		if address :is [\"X-frobnicle\", \"FrOm\"] \"thEm\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::ExistsTestTest(void)
{
	// simple test
	NextSubTest();
	filter.Content("if exists \"From\" { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// exist must match case-independent
	NextSubTest();
	filter.Content("if exists \"fRoM\" { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// checking more than one header means all of them must exist
	NextSubTest();
	filter.Content("if exists [\"From\", \"Cc\"] { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("if exists [\"From\", \"X-frobnicle\"] { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::HeaderTestTest(void)
{
	// simple :is comparison
	NextSubTest();
	filter.Content("\
		if header \
			:is \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// default match-type should be :is
	NextSubTest();
	filter.Content("if header \"From\" \"the\" { keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// a nonexisting header never :is ""
	NextSubTest();
	filter.Content("\
		if header \
			:is \"X-frobnicle\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// an existing, empty header always :is ""
	NextSubTest();
	filter.Content("\
		if header \
			:is \"X-Empty\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// yield true if any of multiple keys matches
	NextSubTest();
	filter.Content("\
		if header :is \"From\" [\"me\", \"myself\", \"I\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		if header :is \"From\" [\"me\", \"myself\", \"them\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// yield true if any of multiple headers matches
	NextSubTest();
	filter.Content("\
		if header :is [\"X-frobnicle\", \"X-shratful\"] \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		if header :is [\"X-frobnicle\", \"FrOm\"] \"thEm\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// default comparator should be i;ascii-casemap (case-insensitive)
	NextSubTest();
	filter.Content("\
		if header \
			:is \"From\" \"tHem\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// specifying a case-sensitive comparator should work
	NextSubTest();
	filter.Content("\
		if header \
			:comparator \"i;octet\" \
			:is \"From\" \"tHem\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// specifying a non-existent comparator should bail
	NextSubTest();
	filter.Content("\
		if header \
			:comparator \"i;cruxtet\" \
			:is \"From\" \"tHem\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("unknown comparator tag") != B_ERROR);

	// :contains comparison
	NextSubTest();
	filter.Content("\
		if header \
			:contains \"From\" \"hem\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:contains \"From\" \"hohum\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// existing header always :contains ""
	NextSubTest();
	filter.Content("\
		if header \
			:contains \"From\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:contains \"X-Empty\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// non-existing header never :contains ""
	NextSubTest();
	filter.Content("\
		if header \
			:contains \"X-frobnicle\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// :matches comparison
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"th?m\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"t*m\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"?h*\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"*\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"*them*\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"From\" \"*hum\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// non-existing header never matches anything
	NextSubTest();
	filter.Content("\
		if header \
			:matches \"X-frobnicle\" \"*\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// :regex needs to be required
	NextSubTest();
	filter.Content("\
		if header \
			:regex \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("regex not required") != B_ERROR);
	// :regex comparison
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"th.m\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"th.m\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"t.+m\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"^th.+$\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"^(.*)$\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \"^them$\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	//
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"From\" \".hum$\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// non-existing header never matches anything
	NextSubTest();
	filter.Content("\
		require \"regex\"; \
		if header \
			:regex \"X-frobnicle\" \".*\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript() || CompErr());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::RelationalValueTestsTest(void)
{
	// relational match-types (COUNT and VALUE) need to be required
	NextSubTest();
	filter.Content("\
		if header \
			:value \"eq\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("relational not required") != B_ERROR);

	// value eq
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"eq\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"eq\" \"From\" \"those\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"eq\" \"X-Nonexistant\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"eq\" \"X-Empty\" \"\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value ne
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"ne\" \"From\" \"tHem\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"ne\" \"From\" \"tHose\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value gt
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"gt\" \"From\" \"tham\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"gt\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"gt\" \"From\" \"thex\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// value ge
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"ge\" \"From\" \"tham\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"ge\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"ge\" \"From\" \"thex\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// value lt
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"lt\" \"From\" \"tham\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"lt\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"lt\" \"From\" \"thex\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value le
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"le\" \"From\" \"tham\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"le\" \"From\" \"them\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"le\" \"From\" \"thex\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::NumericRelationalValueTestsTest(void)
{
	// numerical comparator needs to be required
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		if header \
			:value \"eq\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( !filter.CompileScript() 
						 && filter.ErrorString()
						 		.FindFirst("i;ascii-numeric not required") != B_ERROR);
	// value eq
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"eq\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"eq\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"eq\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"000003\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value ne
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"ne\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"ne\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"33\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value gt
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"gt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"gt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"gt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"12\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// value ge
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"ge\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"ge\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"ge\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"12\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);

	// value lt
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"lt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"lt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"lt\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"00010\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// value le
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"le\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_TRASH);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"le\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:value \"le\" :comparator \"i;ascii-numeric\" \
			\"X-Priority\" \"4\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
SieveTest::NumericRelationalCountTestsTest(void)
{
	// counts on header
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"From\" \"1\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"Received\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"X-Nonexistant\" \"0\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:count \"ge\" :comparator \"i;ascii-numeric\" \
			[\"To\",\"Cc\"] \"4\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if header \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			[\"X-Nonexistant\",\"Cc\",\"Received\"] [\"1\",\"2\",\"3\",\"4\",\"5\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);

	// counts on address
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if address \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"From\" \"1\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if address \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"Cc\" \"3\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if address \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			\"To\" \"2\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if address \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			[\"To\",\"Cc\"] \"5\" \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
	// 
	NextSubTest();
	filter.Content("\
		require \"relational\"; \
		require \"comparator-i;ascii-numeric\"; \
		if address \
			:count \"eq\" :comparator \"i;ascii-numeric\" \
			[\"X-Nonexistant\",\"Cc\"] [\"1\",\"2\",\"3\"] \
		{ keep; } else { discard; }");
	CPPUNIT_ASSERT( filter.CompileScript());
	CPPUNIT_ASSERT( filter.Execute(msgContext));
	CPPUNIT_ASSERT( Result() == RES_KEEP);
}
