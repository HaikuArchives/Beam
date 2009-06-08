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

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmMailRefViewFilterControl::BmMailRefViewFilterControl()
	:	inheritedView(
			mMenuControl = new BmMenuControl(
				"Narrow Down:",
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
	mTextControl->TextView()->AddFilter(new BMessageFilter(
		B_ANY_DELIVERY, B_ANY_SOURCE, B_KEY_DOWN, MessageFilterHook
	));
	mLastKind = mMenuControl->MenuItem()->Label();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::MakeFocus(bool focus)
{
	if (focus)
		mTextControl->MakeFocus(true);
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
				if (content != mLastContent || kind != mLastKind) {
					BmMailRefItemFilter* filter
						= content.Length() > 0
							? new BmMailRefItemFilter(kind, content)
							: NULL;
					StartJob(
						new BmMailRefViewFilterJob(filter, mPartnerMailRefView)
					);
					mLastKind = kind;
					mLastContent = content;
				}
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
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::KeyDown(const char *bytes, int32 numBytes)
{
	if (numBytes == 1) {
		switch(bytes[0]) {
			case B_ESCAPE:
				ClearFilter();
				return;
			case B_DOWN_ARROW:
				// shift focus to ref-view
				if (mPartnerMailRefView) {
					mPartnerMailRefView->MakeFocus(true);
					mPartnerMailRefView->ScrollToSelection();
				}
				return;
			default:
				break;
		}
	}
	inheritedView::KeyDown( bytes, numBytes);
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

/*------------------------------------------------------------------------------*\
	ClearFilter()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefViewFilterControl::ClearFilter()
{
	mTextControl->TextView()->SetText("");
}

/*------------------------------------------------------------------------------*\
	MessageFilterHook()
		-	intercepts key-down messages for B_ESCAPE and B_DOWN_ARROW from the
			textview and passes them on to the control, which will trigger
			some specific actions for those
\*------------------------------------------------------------------------------*/
filter_result BmMailRefViewFilterControl::MessageFilterHook(BMessage* msg, 
	BHandler** handler, BMessageFilter* messageFilter)
{
	if (msg->what == B_KEY_DOWN) {
		BmString bytes = msg->FindString( "bytes");
		if (bytes.Length() 
		&& (bytes[0] == B_ESCAPE || bytes[0] == B_DOWN_ARROW)) {
			// dispatch to the control:
			BView* view = (BView*)(*handler);
			*handler =  view->Parent()->Parent();
		}
	}
	return B_DISPATCH_MESSAGE;
}
