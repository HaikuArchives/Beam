/*
	MailMonitorTest.cpp
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

#include "MailMonitorTest.h"
#include "TestBeam.h"

#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmStorageUtil.h"

MailMonitorTest::MailMonitorTest()
{
	BmMailMonitor::CreateInstance();
	TheMailFolderList->NeedControllersToContinue(false);
	TheMailFolderList->StartJobInThisThread();
}

// setUp
void
MailMonitorTest::setUp()
{
	inherited::setUp();
}
	
// tearDown
void
MailMonitorTest::tearDown()
{
	inherited::tearDown();
}


static const int32 NAPTIME = 10*1000;
							// this may need tweaking if run on slow machine!

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::MailRefTest(void)
{
	BNode node;
	node_ref nref;
	BmMailRef* ref = NULL;

	node.SetTo( "mail/in");
	node.GetNodeRef( &nref);
	BmRef<BmMailFolder> in = 
		dynamic_cast< BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
		);
	CPPUNIT_ASSERT( in != NULL);
	BmRef<BmMailRefList> inList( in->MailRefList());
	CPPUNIT_ASSERT( inList != NULL);
	inList->NeedControllersToContinue( false);
	inList->StartJobInThisThread();
	CPPUNIT_ASSERT( inList->InitCheck() == B_OK);
	
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
	snooze( NAPTIME);
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);

	// (and backwards...)
	NextSubTest();
	node.SetTo( "mail/folder1/testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/folder1/testmail_1 mail/in/");
	snooze( NAPTIME);
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) == NULL);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// move of one mail into another folder (*outside* of mailbox)
	NextSubTest();
	node.SetTo( "mail/in/testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv mail/in/testmail_1 .");
	snooze( NAPTIME);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);

	// move of one mail from outside of mailbox into it
	NextSubTest();
	node.SetTo( "./testmail_1");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	system("mv ./testmail_1 mail/folder1/");
	snooze( NAPTIME);
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// creation of a new mail
	NextSubTest();
	system("copyattr -d mail/in/testmail_2 mail/folder1/");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	snooze( NAPTIME);
	CPPUNIT_ASSERT( folder1List->FindItemByKey( BM_REFKEY(nref)) != NULL);
	node.SetTo( "mail/in/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) != NULL);

	// changing the mimetype of a mail so that it is *not* a mail anymore
	NextSubTest();
	system("addattr BEOS:TYPE text/plain mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	snooze( NAPTIME);
	ref = dynamic_cast< BmMailRef*>(
		folder1List->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	// mailref should exist...
	CPPUNIT_ASSERT( ref != NULL);
	// ...but must be invalid:
	CPPUNIT_ASSERT( ref->ItemIsValid() == false);

	// changing the mimetype of a file so that it now *is* a mail
	NextSubTest();
	system("addattr BEOS:TYPE text/x-email mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	snooze( NAPTIME);
	ref = dynamic_cast< BmMailRef*>(
		folder1List->FindItemByKey( BM_REFKEY(nref)).Get()
	);
	// mailref should exist...
	CPPUNIT_ASSERT( ref != NULL);
	// ...and must be valid:
	CPPUNIT_ASSERT( ref->ItemIsValid() == true);

	// changing an attribute of a mail
	NextSubTest();
	system("addattr MAIL:account testacc mail/folder1/testmail_2");
	node.SetTo( "mail/folder1/testmail_2");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	snooze( NAPTIME);
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
	snooze( NAPTIME);
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
	snooze( NAPTIME);
	CPPUNIT_ASSERT( inList->FindItemByKey( BM_REFKEY(nref)) == NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void 
MailMonitorTest::MailFolderTest(void)
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

	node.SetTo( "mail/in");
	node.GetNodeRef( &nref);
	BmRef<BmMailFolder> in = 
		dynamic_cast< BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY(nref)).Get()
		);
	CPPUNIT_ASSERT( in != NULL);
	
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
	snooze( NAPTIME);
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( in->FindItemByKey( BM_REFKEY(nref)) != NULL);
	}

	// moving of a folder within the mailbox (no change of name)
	NextSubTest();
	system("mv mail/in/subFolder mail/folder2/");
	node.SetTo( "mail/folder2/subFolder");
	CPPUNIT_ASSERT( node.GetNodeRef( &nref) == B_OK);
	snooze( NAPTIME);
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
	snooze( NAPTIME);
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
	snooze( NAPTIME);
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
	snooze( NAPTIME);
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
	snooze( NAPTIME);
	{
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		CPPUNIT_ASSERT(lock.IsLocked());
		CPPUNIT_ASSERT( mail->FindItemByKey( BM_REFKEY(nref)) == NULL);
	}
}
