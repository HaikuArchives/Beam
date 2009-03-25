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
#include "BmMailRefFilterControl.h"
#include "BmMenuController.h"

const char* const BmMailRefFilterControl::MSG_TIME_SPAN = "bm:timsp";

const char* const BmMailRefFilterControl::TIME_SPAN_NONE  = "<No Limit>";
const char* const BmMailRefFilterControl::TIME_SPAN_YEAR  = "One Year";
const char* const BmMailRefFilterControl::TIME_SPAN_MONTH = "One Month";
const char* const BmMailRefFilterControl::TIME_SPAN_WEEK  = "One Week";

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefFilterControl::BmMailRefFilterControl()
	:	inherited(
			"Active Time Span:",
			new BmMenuController(
				TIME_SPAN_NONE, this, 
				new BMessage(BM_MAILREF_FILTER_CHANGED), 
				&BmGuiRosterBase::RebuildMailRefFilterMenu,
				BM_MC_LABEL_FROM_MARKED
			),
			1, 1E5, TIME_SPAN_NONE
	)
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
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefFilterControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefFilterControl::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_MAILREF_FILTER_CHANGED: {
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

