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

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterControl::BmMailRefViewFilterControl()
	:	inheritedView(
			mMenuControl = new BmMenuControl(
				"Filter on:",
				new BmMenuController(
					BmMailRefItemFilter::FILTER_SUBJECT_OR_ADDRESS, this,
					new BMessage(BM_MAILREF_VIEW_FILTER_CHANGED), 
					&BmGuiRosterBase::RebuildMailRefViewFilterMenu,
					BM_MC_LABEL_FROM_MARKED
				), 1, 1E5, BmMailRefItemFilter::FILTER_SUBJECT_OR_ADDRESS
			),
			mTextControl = new BmTextControl(NULL, false, 0, 30),
			NULL
		)
	,	inheritedController("RefViewFilterController")
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
	delete mMsgRunner;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::AttachedToWindow()
{
	inheritedView::AttachedToWindow();
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
				delete mMsgRunner;
				mMsgRunner = NULL;
				BmString kind = mMenuControl->MenuItem()->Label();
				BmString content = mTextControl->Text();
				BmMailRefItemFilter* filter 
					= content.Length() > 0
						? new BmMailRefItemFilter(kind, content)
						: NULL;
				StartJob(new BmMailRefViewFilterJob(filter, mPartnerMailRefView));
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
			case BM_JOB_DONE: {
				if (!IsMsgFromCurrentModel( msg))
					break;
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			default:
				inheritedView::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailRefViewFilterControl: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::JobIsDone(bool completed)
{
	BM_LOG2( BM_LogModelController, 
				BmString("Controller <") << ControllerName() 
					<< "> has been told that job " << ModelName() << " is done");
}
