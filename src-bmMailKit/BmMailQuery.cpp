/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Entry.h>

#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailQuery.h"
#include "BmMailRef.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"

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
	status_t err;
	BM_LOG2( BM_LogMailTracking, "Start of pending-mail-query");
	mRefVect.clear();
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

	entry_ref eref;
	while ((err = mQuery.GetNextRef(&eref)) == B_OK) {
		if (LivesInMailbox(eref))
			mRefVect.push_back(eref);
	}
	BM_LOG2( BM_LogMailTracking, "Done with pending-mail-query");
}
