/*
	BmMailQuery.cpp

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

#include <Entry.h>

#include "BmLogHandler.h"
#include "BmMailQuery.h"
#include "BmMailRef.h"
#include "BmPrefs.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailQuery::BmMailQuery()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailQuery::SetPredicate( const BmString& pred)
{
	mPredicate = pred;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailQuery::Execute()
{
	mMailVect.clear();
	status_t err;
	BEntry entry;
	entry_ref eref;
	struct stat st;
	BmRef<BmMailRef> mailRef;
	BmRef<BmMail> mail;
	BM_LOG2( BM_LogMailTracking, "Start of pending-mail-query");
	mQuery.Clear();
	err = mQuery.SetVolume( &ThePrefs->MailboxVolume);
	if (err != B_OK) {
		BM_LOGERR( BmString("MailQuery::Execute(): couldn't set volume\n\n")
							<< "Error: " << strerror(err));
		return;
	}
	err = mQuery.SetPredicate( mPredicate.String());
	if (err != B_OK) {
		BM_LOGERR( BmString("MailQuery::Execute(): couldn't set predicate\n\n")
							<< "Error: " << strerror(err));
		return;
	}
	err = mQuery.Fetch();
	if (err != B_OK) {
		BM_LOGERR( BmString("MailQuery::Execute(): couldn't fetch mails\n\n")
							<< "Error: " << strerror(err));
		return;
	}
	while ((err = mQuery.GetNextEntry(&entry)) == B_OK) {
		entry.GetRef(&eref);
		entry.GetStat(&st);
		mailRef = BmMailRef::CreateInstance( eref, st);
		if (mailRef->ItemIsValid()) {
			mail = BmMail::CreateInstance( mailRef.Get());
			if (mail) {
				if (mail->InitCheck() != B_OK)
					mail->StartJobInThisThread( BmMail::BM_READ_MAIL_JOB);
				if (mail->InitCheck() == B_OK)
					mMailVect.push_back(mail);
			}
		}
	}
	BM_LOG2( BM_LogMailTracking, "Done with pending-mail-query");
}

/*------------------------------------------------------------------------------*\
	HandoutMails()
		-	moves info about found mails into given vector
\*------------------------------------------------------------------------------*/
void BmMailQuery::HandoutMails(BmMailVect &outMailVect) {
	outMailVect = mMailVect;
	mMailVect.clear();
}

