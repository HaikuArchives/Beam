/*
	BmMailView.cpp
		$Id$
*/

#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"

/********************************************************************************\
	BmMailView
\********************************************************************************/

BmMailView* BmMailView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView* BmMailView::CreateInstance( minimax minmax, BRect frame, bool editable) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmMailView( minmax, frame, editable);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::BmMailView( minimax minmax, BRect frame, bool editable) 
	:	inherited( frame, "MailView", frame,
					  B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE)
	,	inheritedController( "MailViewController")
	,	mEditMode( editable)
	,	mCurrMail( NULL)
{
	MakeEditable( editable);
	SetStylable( !editable);
	SetWordWrap( editable);
	mScrollView = new BmMailViewContainer( minmax, this, B_FOLLOW_NONE, 
														B_WILL_DRAW | B_FRAME_EVENTS);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::~BmMailView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::FrameResized(float new_width, float new_height) {
	inherited::FrameResized( new_width, new_height);
	SetTextRect( BRect(1,1,new_width-2,new_height-2));
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_JOB_UPDATE_STATE: {
				if (!IsMsgFromCurrentModel( msg)) return;
//				UpdateModelState( msg);
				break;
			}
			case BM_JOB_DONE: {
				if (!IsMsgFromCurrentModel( msg)) return;
				BM_LOG2( BM_LogModelController, BString("Model <")<<FindMsgString( msg, BmDataModel::MSG_MODEL)<<"> has told it is done.");
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString(ControllerName()) << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMail( BmMailRef* ref) {
	try {
		StopJob();
		mCurrMail = new BmMail( ref);
		StartJob( mCurrMail.Get(), true);
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::JobIsDone( bool completed) {
	if (completed && mCurrMail) {
		BString mailText = mCurrMail->Text();
		BM_LOG2( BM_LogMailParse, BString("setting mailtext into textview"));
		SetText( mailText.String());
		BM_LOG2( BM_LogMailParse, BString("done, mail is visible"));
	} else 
		SetText( "");
}



/********************************************************************************\
	BmMailViewContainer
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer::BmMailViewContainer( minimax minmax, BmMailView* target, 
														uint32 resizingMode, uint32 flags)
	:	inherited( NULL, target, resizingMode, flags, true, true, true, B_PLAIN_BORDER)
{
	ct_mpm = minmax;
	target->TargetedByScrollView( this);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer::~BmMailViewContainer() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
minimax BmMailViewContainer::layoutprefs()
{
	return mpm=ct_mpm;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::ResizeTo(float new_width, float new_height) {
	inherited::ResizeTo( new_width-1, new_height-1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::FrameResized(float new_width, float new_height) {
	inherited::FrameResized( new_width, new_height);
	if (m_target)
		m_target->ResizeTo( new_width-B_V_SCROLL_BAR_WIDTH-2,new_height-B_H_SCROLL_BAR_HEIGHT-2);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BRect BmMailViewContainer::layout(BRect rect)
{
	MoveTo(rect.LeftTop());
	ResizeTo(rect.Width(),rect.Height());
	return rect;
}
