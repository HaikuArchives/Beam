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

#include <Directory.h>
#include <UTF8.h>

#include "MailMonitorTest.h"
#include "TestBeam.h"

#include <ThreadedTestCaller.h>

#include "BmMail.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailMonitor.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmMainWindow.h"
#include "BmLogHandler.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"

static BmString mailText("\
Date: Mon, 25 Feb 2003 08:51:06 -0500\r\n\
Received: through3\r\n\
Received: through2\r\n\
Received: through1\r\n\
From: them\r\n\
To: you@test.org\r\n\
Cc: the_cc <cc@test.org>\r\n\
Sender: <test@test.org>\r\n\
Subject: A simple testmail for mail monitoring\r\n\
X-Priority: 3\r\n\
X-Empty: \r\n\
\r\n\
blah (just to have a body)\
");

static BmRef<BmMailFolder> inFolder;
static BmRef<BmMailRefList> inList;
static BmRef<BmMailRefList> refList;

class RefListSyncer
{
public:
	RefListSyncer(BmMailRefList* refList)
	:	mLock("SyncerLock", false)
	,	mRefList(refList)
	{
	}

	bool CheckSpecialCount()
	{
		BAutolock lock(&mLock);
		// wait for the mail monitor to process all outstanding messages:
		while(!TheMailMonitor->IsIdle(500))
			snooze(50*1000);

		BDirectory mailDir;
		entry_ref eref;
		dirent* dent;
		struct stat st;
		status_t err;
		char buf[4096];
		int32 count;
		int32 specialCount = 0;
		mailDir.SetTo( inFolder->EntryRefPtr());
		BNode node;
		BmString status;
	
		BmRef<BmMailRef> newRef;
		// ...and scan through all its entries for mails:
		while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
			dent = (dirent* )buf;
			while (count-- > 0) {
				if (!(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))) {
					if ((err=mailDir.GetStatFor( dent->d_name, &st)) != B_OK)
						BM_THROW_RUNTIME( 
							BmString("Could not get stat-info for \nmail-file <") 
								<< dent->d_name << "> \n\nError:" << strerror(err)
						);
					if (S_ISREG( st.st_mode)) {
						eref.device = dent->d_pdev;
						eref.directory = dent->d_pino;
						eref.set_name( dent->d_name);
						node.SetTo(&eref);
						BmReadStringAttr(&node, BM_MAIL_ATTR_STATUS, status);
						if (status == BM_MAIL_STATUS_NEW 
						|| status == BM_MAIL_STATUS_PENDING)
							specialCount++;
					}
				}
				// Bump the dirent-pointer by length of the dirent just handled:
				dent = (dirent* )((char* )dent + dent->d_reclen);
			}
		}

		bool val = specialCount == inFolder->SpecialMailCount();
		printf("<CheckSpecial: %ld>", inFolder->SpecialMailCount());fflush(stdout);
		if (!val)
			printf("\n\tspecial-count=%ld is=%ld\n", specialCount, inFolder->SpecialMailCount());
		return val;
	}

	int32 InFolderCount()
	{
		BDirectory mailDir;
		entry_ref eref;
		dirent* dent;
		struct stat st;
		status_t err;
		char buf[4096];
		int32 count;
		int32 refCount = 0;
		mailDir.SetTo( inFolder->EntryRefPtr());
	
		BmRef<BmMailRef> newRef;
		// ...and scan through all its entries for mails:
		while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
			dent = (dirent* )buf;
			while (count-- > 0) {
				if (!(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))) {
					if ((err=mailDir.GetStatFor( dent->d_name, &st)) != B_OK)
						BM_THROW_RUNTIME( 
							BmString("Could not get stat-info for \nmail-file <") 
								<< dent->d_name << "> \n\nError:" << strerror(err)
						);
					if (S_ISREG( st.st_mode))
						refCount++;
				}
				// Bump the dirent-pointer by length of the dirent just handled:
				dent = (dirent* )((char* )dent + dent->d_reclen);
			}
		}
		return refCount;
	}

	bool CheckRefList()
	{
		BAutolock lock(&mLock);
		// wait for the mail monitor to process all outstanding messages:
		while(!TheMailMonitor->IsIdle(500))
			snooze(50*1000);
		
		int32 refCount = InFolderCount();
		while(mRefList->IsJobRunning())
			snooze(50*1000);

		bool val = refCount == (int32)mRefList->size();
		printf("<Check: %ld>", mRefList->size());fflush(stdout);
		if (!val) {
			printf("\n\tCurrent: ref-count=%ld size=%ld\n", refCount, (int32)mRefList->size());
		}
		return val;
	}

	bool CheckStoredRefList()
	{
		BAutolock lock(&mLock);
		// wait for the mail monitor to process all outstanding messages:
		while(!TheMailMonitor->IsIdle(500))
			snooze(50*1000);
		
		int32 refCount = InFolderCount();
		refList->Cleanup();
		inList->FlushStoredActions();	// write any outstanding actions to disk
		refList->StartJobInThisThread();
		bool val = refCount == (int32)refList->size();
		printf("<CheckStored: %ld>", refList->size());fflush(stdout);
		if (!val) {
			printf("\n\tStored: ref-count=%ld size=%ld\n", refCount, (int32)refList->size());
			for(int i=0; !val && i<10; ++i) {
				refCount = InFolderCount();
				refList->Cleanup();
				refList->StartJobInThisThread();
				printf("\n\tStored: ref-count=%ld size=%ld\n", refCount, (int32)refList->size());
				snooze(1000*1000);
				val = refCount == (int32)refList->size();
			}
		}
		return val;
	}

	void Sync()
	{
		BAutolock lock(&mLock);
	}
private:
	BLocker mLock;
	BmMailRefList* mRefList;
};

static RefListSyncer* refListSyncer;

MailMonitorTest::MailMonitorTest()
{
}

MailMonitorTest::~MailMonitorTest()
{
}

// setUp
void
MailMonitorTest::setUp()
{
	inherited::setUp();
	BNode node;
	node_ref nref;

	// we fetch info about original in-folder and then replace it by our
	// own version:
	node.SetTo( "mail/in");
	node.GetNodeRef( &nref);
	inFolder = dynamic_cast< BmMailFolder*>( 
		TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	CPPUNIT_ASSERT( inFolder != NULL);
	refList = new BmMailRefList(inFolder.Get());
	refList->NeedControllersToContinue(false);
	inList = inFolder->MailRefList();
	CPPUNIT_ASSERT( inList != NULL);

	inList->NeedControllersToContinue(false);
	inList->StartJobInThisThread();

	if (TheMailFolderView->LockLooper()) {
		int32 itemCount = TheMailFolderView->FullListCountItems();
		for(int32 i=0; i<itemCount; ++i)
			TheMailFolderView->Expand(TheMailFolderView->FullListItemAt(i));
		TheMailFolderView->Select(1);
		TheMailFolderView->UnlockLooper();
	}

	inList->NeedControllersToContinue(true);
	CPPUNIT_ASSERT( inList->InitCheck() == B_OK);

	refListSyncer = new RefListSyncer(inList.Get());
	CPPUNIT_ASSERT( refListSyncer != NULL);
	CPPUNIT_ASSERT( refListSyncer->CheckRefList());
}
	
// tearDown
void
MailMonitorTest::tearDown()
{
	CPPUNIT_ASSERT( refListSyncer->CheckRefList());
	delete refListSyncer;
	refList = NULL;
	inList = NULL;
	inFolder = NULL;
	inherited::tearDown();
}


CppUnit::Test*
MailMonitorTest::suite() {
	CppUnit::TestSuite *suite = new CppUnit::TestSuite("MailMonitorSuite");
	BThreadedTestCaller<MailMonitorTest> *caller;
	MailMonitorTest *test;
	
	// simple test for mail-ref-tracking:
	suite->addTest(new CppUnit::TestCaller<MailMonitorTest>(
		"MailMonitorTest::BasicMailRefTest", 
		&MailMonitorTest::BasicMailRefTest
	));

	// simple test for mail-folder-tracking:
	suite->addTest(new CppUnit::TestCaller<MailMonitorTest>(
		"MailMonitorTest::BasicMailFolderTest", 
		&MailMonitorTest::BasicMailFolderTest
	));

	// massive mail-ref-tracking test with multiple threads:
	test = new MailMonitorTest;
	caller = new BThreadedTestCaller<MailMonitorTest>(
		"MailMonitorTest::MassiveMailRefTest", test
	);
	caller->addThread("t1", &MailMonitorTest::MassiveMailRefCreator);
	caller->addThread("t2", &MailMonitorTest::MassiveMailRefCreator);
	caller->addThread("t3", &MailMonitorTest::MassiveMailRefRemover);
	caller->addThread("t4", &MailMonitorTest::MassiveMailRefCheckerTest);
	suite->addTest(caller);
	
	// massive new-status-tracking test with multiple threads:
	test = new MailMonitorTest;
	caller = new BThreadedTestCaller<MailMonitorTest>(
		"MailMonitorTest::MassiveSpecialMailTest", test
	);
	caller->addThread("t1", &MailMonitorTest::SpecialMailSetter);
	caller->addThread("t3", &MailMonitorTest::SpecialMailClearer);
	caller->addThread("t4", &MailMonitorTest::SpecialMailCheckerTest);
	suite->addTest(caller);

	// adding/removing to a specific ref-list while this is being stored
	// and loaded:
	test = new MailMonitorTest;
	caller = new BThreadedTestCaller<MailMonitorTest>(
		"MailMonitorTest::RefListStorageTest", test
	);
	caller->addThread("t1", &MailMonitorTest::RefListAdder);
	caller->addThread("t2", &MailMonitorTest::RefListAdder);
	caller->addThread("t3", &MailMonitorTest::RefListRemover);
	caller->addThread("t4", &MailMonitorTest::RefListStorageTest);
	suite->addTest(caller);

	return suite;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::SyncWithMailMonitor(void)
{
	// wait for the mail monitor to process all outstanding messages:
	while(!TheMailMonitor->IsIdle(0))
		snooze(20*1000);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::BasicMailRefTest(void)
{
	BNode node;
	node_ref nref;
	BmMailRef* ref = NULL;

	node.SetTo( "mail/folder1");
	node.GetNodeRef( &nref);
	BmRef<BmMailFolder> folder1 = 
		dynamic_cast< BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
		);
	CPPUNIT_ASSERT( folder1 != NULL);
	BmRef<BmMailRefList> folder1List( folder1->MailRefList());
	CPPUNIT_ASSERT( folder1List != NULL);
	folder1List->NeedControllersToContinue( false);
	folder1List->StartJobInThisThread();
	CPPUNIT_ASSERT( folder1List->InitCheck() == B_OK);

	// move of one mail into another folder (*inside* of mailbox)
	NextSubTest();
	node.SetTo( "mail/in/testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/in/testmail_1 mail/folder1/");
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);

	// (and backwards...)
	NextSubTest();
	node.SetTo( "mail/folder1/testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/folder1/testmail_1 mail/in/");
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) == NULL);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// move of one mail into another folder (*outside* of mailbox)
	NextSubTest();
	node.SetTo( "mail/in/testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/in/testmail_1 .");
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);

	// move of one mail from outside of mailbox into it
	NextSubTest();
	node.SetTo( "./testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv ./testmail_1 mail/folder1/");
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// creation of a new mail
	NextSubTest();
	system("copyattr -d mail/in/testmail_2 mail/folder1/");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);
	node.SetTo( "mail/in/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// changing the mimetype of a mail so that it is *not* a mail anymore
	NextSubTest();
	system("addattr BEOS:TYPE text/plain mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	ref = dynamic_cast< BmMailRef*>(
		folder1List->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	// mailref should exist...
	CPPUNIT_ASSERT( ref != NULL);
	// ...but must be invalid:
	CPPUNIT_ASSERT( ref->IsValid() == false);

	// changing the mimetype of a file so that it now *is* a mail
	NextSubTest();
	system("addattr BEOS:TYPE text/x-email mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	ref = dynamic_cast< BmMailRef*>(
		folder1List->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	// mailref should exist...
	CPPUNIT_ASSERT( ref != NULL);
	// ...and must be valid:
	CPPUNIT_ASSERT( ref->IsValid() == true);

	// changing an attribute of a mail
	NextSubTest();
	system("addattr MAIL:account testacc mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	ref = dynamic_cast< BmMailRef*>(
		folder1List->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	CPPUNIT_ASSERT( ref != NULL);
	CPPUNIT_ASSERT( ref->Account() == "testacc");

	// rename of a mail
	NextSubTest();
	node.SetTo( "mail/in/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/in/testmail_2 mail/in/testmail_x");
	SyncWithMailMonitor();
	ref = dynamic_cast< BmMailRef*>(
		inList->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	CPPUNIT_ASSERT( ref != NULL);
	CPPUNIT_ASSERT( BmString("testmail_x") == ref->TrackerName());
							// filename should have changed...
	CPPUNIT_ASSERT( ref->Name() == "imtarget");
							// ...name of mail-originator should not have changed

	// removal of a mail
	NextSubTest();
	node.SetTo( "mail/in/testmail_x");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("rm mail/in/testmail_x");
	SyncWithMailMonitor();
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::BasicMailFolderTest(void)
{
	BNode node;
	node_ref nref;

	node.SetTo( "mail");
	node.GetNodeRef( &nref);
	BmRef<BmMailFolder> mail = 
		dynamic_cast< BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
		);
	CPPUNIT_ASSERT( mail != NULL);

	node.SetTo( "mail/folder2");
	node.GetNodeRef( &nref);
	BmRef<BmMailFolder> folder2 = 
		dynamic_cast< BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
		);
	CPPUNIT_ASSERT( folder2 != NULL);

	// creation of a new folder
	NextSubTest();
	system("mkdir mail/in/subFolder");
	node.SetTo( "mail/in/subFolder");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( inFolder->FindItemByKey( BM_REFKEY(nref)) != NULL);
	}

	// moving of a folder within the mailbox (no change of name)
	NextSubTest();
	system("mv mail/in/subFolder mail/folder2/");
	node.SetTo( "mail/folder2/subFolder");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( folder2->FindItemByKey( BM_REFKEY(nref)) != NULL);
	}

	// moving of a folder within the mailbox (with change of name)
	NextSubTest();
	system("mv mail/folder2/subFolder mail/folderX");
	node.SetTo( "mail/folderX");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( mail->FindItemByKey( BM_REFKEY(nref)) != NULL);
	}

	// moving of a folder from inside of mailbox to outside
	NextSubTest();
	system("mv mail/folderX /var/tmp");
	node.SetTo( "/var/tmp/folderX");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( mail->FindItemByKey( BM_REFKEY(nref)) == NULL);
	}

	// moving of a folder from outside of mailbox to inside
	NextSubTest();
	system("mv /var/tmp/folderX mail/");
	node.SetTo( "mail/folderX");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( mail->FindItemByKey( BM_REFKEY(nref)) != NULL);
	}

	// removal of a folder
	NextSubTest();
	node.SetTo( "mail/folderX");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("rmdir mail/folderX");
	SyncWithMailMonitor();
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( mail->FindItemByKey( BM_REFKEY(nref)) == NULL);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::MassiveMailRefCreator(void)
{
	thread_id tid = find_thread(NULL);

	// create lots of mails and check if mail-monitor catches up as it should:
	BDirectory mailDir(&inFolder->NodeRef());
	BEntry mailEntry;
	for(int i=0; i<2500; ++i) {
		if (i%25 == 0)
			NextSubTest();
		BmMail mail(mailText, "");
		BmString name = BmString("massive_") << tid << "_" << i;
		refListSyncer->Sync();
		printf("+");fflush(stdout);
		mail.StoreIntoFile(&mailDir, name, BM_MAIL_STATUS_READ, system_time());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::MassiveMailRefRemover(void)
{
	// remove some mails and check if mail-monitor catches up as it should:
	BDirectory dir(&inFolder->NodeRef());
	BEntry entry;
	snooze(1000*1000);
	for(int i=0; i<5500; ++i) {
		if (i%25 == 0) {
			dir.Rewind();
			NextSubTest();
		}
		dir.GetNextEntry(&entry);
		refListSyncer->Sync();
		printf("-");fflush(stdout);
		entry.Remove();
		snooze(20*1000);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::MassiveMailRefCheckerTest(void)
{
	for(int i=0; i<100; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT(refListSyncer->CheckRefList());
		snooze(1000*1000);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::SpecialMailSetter(void)
{
	// set new status of some mails and check if mail-monitor 
	// catches up as it should:
	BDirectory dir(&inFolder->NodeRef());
	entry_ref eref;
	BNode node;
	for(int i=0; i<500; ++i) {
		if (i%25 == 0)
			NextSubTest();
		dir.GetNextRef(&eref);
		node.SetTo(&eref);
		if (node.InitCheck() == B_OK) {
			BmRef<BmMailRef> ref = BmMailRef::CreateInstance(eref);
			refListSyncer->Sync();
			printf("+");fflush(stdout);
			ref->MarkAs(BM_MAIL_STATUS_NEW);
			snooze(rand()%20*1000);
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::SpecialMailClearer(void)
{
	// remove new status of some mails and check if mail-monitor 
	// catches up as it should:
	BDirectory dir(&inFolder->NodeRef());
	entry_ref eref;
	BNode node;
	snooze(1000*1000);
	for(int i=0; i<250; ++i) {
		if (i%25 == 0)
			NextSubTest();
		dir.GetNextRef(&eref);
		node.SetTo(&eref);
		if (node.InitCheck() == B_OK) {
			BmRef<BmMailRef> ref = BmMailRef::CreateInstance(eref);
			refListSyncer->Sync();
			printf("-");fflush(stdout);
			ref->MarkAs(BM_MAIL_STATUS_READ);
			snooze(rand()%20*1000);
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::SpecialMailCheckerTest(void)
{
	for(int i=0; i<40; ++i) {
		NextSubTest();
		CPPUNIT_ASSERT(refListSyncer->CheckSpecialCount());
		snooze(20*1000);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::RefListAdder(void)
{
	thread_id tid = find_thread(NULL);

	// create lots of mails:
	BDirectory mailDir(&inFolder->NodeRef());
	BEntry mailEntry;
	for(int i=0; i<2500; ++i) {
		if (i%25 == 0)
			NextSubTest();
		BmMail mail(mailText, "");
		BmString name = BmString("reflist_") << tid << "_" << i;
		refListSyncer->Sync();
		printf("+");fflush(stdout);
		mail.StoreIntoFile(&mailDir, name, BM_MAIL_STATUS_READ, system_time());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::RefListRemover(void)
{
	// remove some mails:
	BDirectory dir(&inFolder->NodeRef());
	BEntry entry;
	snooze(1000*1000);
	for(int i=0; i<4500; ++i) {
		if (i%25 == 0) {
			dir.Rewind();
			NextSubTest();
		}
		dir.GetNextEntry(&entry);
		refListSyncer->Sync();
		printf("-");fflush(stdout);
		entry.Remove();
		snooze(20*1000);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::RefListStorageTest(void)
{
	inList->NeedControllersToContinue(true);
	for(int i=0; i<100; ++i) {
		snooze(1000*1000);
		NextSubTest();
		if (TheMailFolderView->LockLooper()) {
			TheMailFolderView->Select(i % 2);
			TheMailFolderView->UnlockLooper();
		}
		if (i % 2)
			CPPUNIT_ASSERT(refListSyncer->CheckRefList());
		else
			CPPUNIT_ASSERT(refListSyncer->CheckStoredRefList());
	}
}
