/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <memory>
#include <stdio.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailRef.h"
#include "BmMailRefViewFilterJob.h"

				
/********************************************************************************\
	BmMailRefItemFilter
\********************************************************************************/

const char* const BmMailRefItemFilter::FILTER_SUBJECT_OR_ADDRESS 
	= "Subject or address";
const char* const BmMailRefItemFilter::FILTER_MAILTEXT
	= "Mail text";

/*------------------------------------------------------------------------------*\
	BmMailRefItemFilter()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailRefItemFilter::BmMailRefItemFilter(const BmString& filterKind, 
													  const BmString& filterText)
	:	mFilterKind(filterKind)
	,	mFilterText(filterText)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailRefItemFilter()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailRefItemFilter::~BmMailRefItemFilter()
{
}

/*------------------------------------------------------------------------------*\
	Matches(viewItem)
		-	applies the filter against the given item and returns true if the
			item has matched
\*------------------------------------------------------------------------------*/
bool BmMailRefItemFilter::Matches(const BmListViewItem* viewItem) const
{
	BmMailRef* ref = dynamic_cast<BmMailRef*>(viewItem->ModelItem());
	if (ref) {
		if (mFilterText.Length() == 0) {
			// no text means always match (shouldn't occur actually, as in this
			// case no filter at all should have been created in the first place
			return true;
		}
		if (mFilterKind == FILTER_SUBJECT_OR_ADDRESS) {
			if (ref->Subject().IFindFirst(mFilterText) >= 0
			|| ref->From().IFindFirst(mFilterText) >= 0
			|| ref->To().IFindFirst(mFilterText) >= 0
			|| ref->Cc().IFindFirst(mFilterText) >= 0)
				return true;
		}
	}
	return false;
}
			


/********************************************************************************\
	BmMailRefViewFilterJob
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMailRefViewFilter()
		-	contructor
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterJob::BmMailRefViewFilterJob(BmViewItemFilter* filter, 
															  BmMailRefView* mailRefView)
	:	BmJobModel("MailRefViewFilterJob")
	,	mFilter(filter)
	,	mMailRefView(mailRefView)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailRefViewFilterJob()
		-	destructor
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterJob::~BmMailRefViewFilterJob() { 
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	the job, executes the filter on all given mail-refs
\*------------------------------------------------------------------------------*/
bool BmMailRefViewFilterJob::StartJob() {
	struct ContinueCallback : public BmViewItemManager::ContinueCallback
	{
		ContinueCallback(BmMailRefViewFilterJob* job)
			: mJob(job)
		{
		}

		bool operator() () { 
			return mJob->ShouldContinue();
		}

		BmMailRefViewFilterJob* mJob;
	};

	try {
		ContinueCallback callback(this);
		return mMailRefView->ApplyViewItemFilter(mFilter, callback);
	}
	catch( BM_runtime_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR(BmString("BmMailRefViewFilterJob: ") << "\n\n" << err.what());
	}
	return false;
}
