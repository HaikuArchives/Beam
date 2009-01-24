/*
 * Copyright 2009, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <MenuItem.h>
#include <MessageRunner.h>

#include "BmBasics.h"
#include "BmGuiRoster.h"
#include "BmLogHandler.h"
#include "BmMailRefView.h"
#include "BmMailRefViewFilterControl.h"
#include "BmMenuController.h"
#include "BmMenuControl.h"
#include "BmTextControl.h"

const char* const BmMailRefViewFilterControl::MSG_FILTER_KIND = "bm:filknd";
const char* const BmMailRefViewFilterControl::MSG_FILTER_CONTENT = "bm:cont";

const char* const BmMailRefViewFilterControl::FILTER_SUBJECT_OR_ADDRESS 
	= "Subject or Address";
const char* const BmMailRefViewFilterControl::FILTER_MAILTEXT
	= "Mailtext";

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterControl::BmMailRefViewFilterControl()
	:	inherited(
			mMenuControl = new BmMenuControl(
				"Filter on:",
				new BmMenuController(
					"Subject or Address", this,
					new BMessage(BM_MAILREF_VIEW_FILTER_CHANGED), 
					&BmGuiRosterBase::RebuildMailRefViewFilterMenu,
					BM_MC_LABEL_FROM_MARKED
				), 1, 1E5, "Subject or Address"
			),
			mTextControl = new BmTextControl(NULL, false, 0, 30),
			NULL
		)
	,	mMsgRunner(NULL)
	,	mPartnerMailRefView(NULL)
{
	mTextControl->TextView()->SetStylable(false);
	mTextControl->TextView()->DisallowChar(B_ESCAPE);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterControl::~BmMailRefViewFilterControl() {
	if (mMsgRunner)
		delete mMsgRunner;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::AttachedToWindow()
{
	inherited::AttachedToWindow();
	mTextControl->SetTarget(this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_MAILREF_VIEW_FILTER_CHANGED: {
				if (mMsgRunner) {
					delete mMsgRunner;
					mMsgRunner = NULL;
				}
				msg->AddString(MSG_FILTER_KIND, mMenuControl->MenuItem()->Label());
				msg->AddString(MSG_FILTER_CONTENT, mTextControl->Text());
				BMessenger partner(mPartnerMailRefView);
				if (partner.IsValid())
					partner.SendMessage(msg, (BHandler*)NULL, 500*1000);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				if (!mMsgRunner) {
					BMessenger ourselvesAsTarget( this);
					BMessage changedMsg(BM_MAILREF_VIEW_FILTER_CHANGED);
					mMsgRunner = new BMessageRunner(
						ourselvesAsTarget, &changedMsg, 200*1000, 1
					);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefViewFilterControl: ") << err.what());
	}
}
