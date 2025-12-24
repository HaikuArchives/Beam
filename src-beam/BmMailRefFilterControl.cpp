/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Message.h>

#include "BmBasics.h"
#include "BmGuiRoster.h"
#include "BmLogHandler.h"
#include "BmMailRef.h"
#include "BmMailRefFilter.h"
#include "BmMailRefFilterControl.h"
#include "BmMailRefView.h"
#include "BmMenuController.h"

// NOTE: The pragma(s) below do not change any functionality in the code. They only hide those 
// warnings when building. At some point, the lines that produce the(se) warning(s), should be
// reviewed to confirm, whether there is something that needs to be "fixed".
#pragma GCC diagnostic ignored "-Wconversion"

const char* const BmMailRefFilterControl::MSG_TIME_SPAN_LABEL = "bm:timsp";

const char* const BmMailRefFilterControl::TIME_SPAN_NONE  = "<No limit>";

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefFilterControl::BmMailRefFilterControl()
	:	inherited(
			"Active time span:",
			new BmMenuController(
				TIME_SPAN_NONE, this, 
				new BMessage(BM_MAILREF_FILTER_CHANGED), 
				&BmGuiRosterBase::RebuildMailRefFilterMenu,
				BM_MC_LABEL_FROM_MARKED
			),
			1, 1E5, TIME_SPAN_NONE
		)
	,	mPartnerMailRefView(NULL)
{
	SetFlags(Flags() & ~B_NAVIGABLE);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefFilterControl::~BmMailRefFilterControl()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefFilterControl::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_MAILREF_FILTER_CHANGED: {
				if (mPartnerMailRefView) {
					BmString label = msg->FindString(MSG_TIME_SPAN_LABEL);
					int32 numberOfDays = atol(label.String());
					BmMailRefFilter* filter 
						= label != TIME_SPAN_NONE
							? new BmMailRefFilter(label, numberOfDays)
							: NULL;
					mPartnerMailRefView->ApplyModelItemFilter(filter);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefFilterControl: ") << err.what());
	}
}

