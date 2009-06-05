/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefFilter_h
#define _BmMailRefFilter_h

#include "BmMailKit.h"

#include <Message.h>

#include "BmBasics.h"
#include "BmDataModel.h"


class IMPEXPBMMAILKIT BmMailRefFilter : public BmListModelItemFilter
{
public:
	BmMailRefFilter(const BmString& filterLabel, int32 numberOfDays);
	BmMailRefFilter(const BMessage *archive);
	virtual ~BmMailRefFilter()				{}
	
	// overrides of base
	virtual bool Matches(const BmListModelItem* modelItem) const;
	virtual status_t Archive(BMessage* archive) const;
 	virtual const BmString& Label() const	{ return mTimeSpan; }

	static const char* const MSG_TIME_SPAN;

private:
	void _SetThreshold(int32 numberOfDays);

	BmString mTimeSpan;
	bigtime_t mThresholdTime;
};


#endif
