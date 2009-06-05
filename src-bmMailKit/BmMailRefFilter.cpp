/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include "BmMailRefFilter.h"
#include "BmMailRef.h"

/*------------------------------------------------------------------------------*\
	BmMailRefFilter
		-	
\*------------------------------------------------------------------------------*/
const char* const BmMailRefFilter::MSG_TIME_SPAN = "bm:timsp";

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefFilter::BmMailRefFilter(const BmString& filterLabel, 
	int32 numberOfDays)
	:	mTimeSpan(filterLabel)
{
	_SetThreshold(numberOfDays);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefFilter::BmMailRefFilter(const BMessage* archive)
	:	mTimeSpan(archive->FindString(MSG_TIME_SPAN))
{
	int32 numberOfDays = atol(mTimeSpan.String());
	_SetThreshold(numberOfDays);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefFilter::Matches(const BmListModelItem* modelItem) const
{
	if (!modelItem)
		return false;
	
	const BmMailRef* mailRef = dynamic_cast<const BmMailRef*>(modelItem);
	if (!mailRef)
		return false;

	// do not filter any mails that have no date set, as this indicates an
	// error and we do not want to leave those mails unnoticed!
	if (mailRef->WhenCreated() == 0)
		return true;

	return mailRef->WhenCreated() > mThresholdTime;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRefFilter::Archive(BMessage* archive) const
{
	return archive->AddString(MSG_TIME_SPAN, mTimeSpan.String());
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefFilter::_SetThreshold(int32 numberOfDays)
{
	bigtime_t now = time(NULL);
	mThresholdTime = (now - numberOfDays * 60 * 60 * 24) * 1000 * 1000;
}

