/*
	BmMailView.cpp
		$Id$
*/

#include <UTF8.h>
#include <Window.h>

#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmBusyView.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeaderView.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"
#include "BmResources.h"

/********************************************************************************\
	BmMailView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView* BmMailView::CreateInstance( minimax minmax, BRect frame, bool editable) {
	// create standard mail-view:
	BmMailView* instance = new BmMailView( minmax, frame, editable);
	// try to open state-cache-file...
	status_t err;
	BFile archiveFile;
	BString archiveFilename = BString("MailView");
	if ((err = archiveFile.SetTo( TheResources->StateInfoFolder(), archiveFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, archive file found, we fetch our state from it:
		try {
			BMessage archive;
			(err = archive.Unflatten( &archiveFile)) == B_OK
												|| BM_THROW_RUNTIME( BString("Could not fetch mail-view archive from file\n\t<") << archiveFilename << ">\n\n Result: " << strerror(err));
			instance->Unarchive( &archive);
		} catch (exception &e) {
			BM_SHOWERR( e.what());
		}
	}
	return instance;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::BmMailView( minimax minmax, BRect frame, bool editable) 
	:	inherited( frame, "MailView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
	,	inheritedController( "MailViewController")
	,	mEditMode( editable)
	,	mCurrMail( NULL)
	,	mPartnerMailRefView( NULL)
	,	mRaw( false)
	,	mFontSize( 12)
{
	mHeaderView = new BmMailHeaderView( NULL);
	AddChild( mHeaderView);
	mBodyPartView = new BmBodyPartView( minimax( 0, 0, 1E5, 1E5), 800, 0);
	mBodyPartView->RemoveSelf();
	AddChild( mBodyPartView);
	mBodyPartView->MoveTo( mHeaderView->Frame().LeftBottom());
	mBodyPartView->ResizeTo( 800,40);
	CalculateVerticalOffset();
	MakeEditable( editable);
	SetStylable( !editable);
	SetWordWrap( true);
	SetFontAndColor( be_fixed_font);
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
status_t BmMailView::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddBool( MSG_RAW, mRaw)
						|| archive->AddString( MSG_FONTNAME, mFontName.String())
						|| archive->AddInt16( MSG_FONTSIZE, mFontSize);
	if (ret == B_OK && deep && mHeaderView)
		ret = mHeaderView->Archive( archive, deep);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailView::Unarchive( BMessage* archive, bool deep=true) {
	status_t ret = archive->FindBool( MSG_RAW, &mRaw)
						|| archive->FindString( MSG_FONTNAME, &mFontName)
						|| archive->FindInt16( MSG_FONTSIZE, &mFontSize);
	if (ret == B_OK && deep && mHeaderView)
		ret = mHeaderView->Unarchive( archive, deep);
	return ret;
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
	KeyDown()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::KeyDown(const char *bytes, int32 numBytes) { 
	if ( numBytes == 1 ) {
		switch( bytes[0]) {
			case B_PAGE_UP:
			case B_PAGE_DOWN:
			case B_UP_ARROW:
			case B_DOWN_ARROW:
			case B_LEFT_ARROW:
			case B_RIGHT_ARROW: {
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods & (B_CONTROL_KEY | B_SHIFT_KEY)) {
					// remove modifiers so we don't ping-pong endlessly:
					Window()->CurrentMessage()->ReplaceInt32("modifiers", 0);
					if (mPartnerMailRefView)
						mPartnerMailRefView->KeyDown( bytes, numBytes);
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			default:
				inherited::KeyDown( bytes, numBytes);
				break;
		}
	}
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMail( BmMailRef* ref) {
	try {
		StopJob();
		if (!ref) {
			DetachModel();
			mHeaderView->ShowHeader( NULL);
			return;
		}
		ContainerView()->SetBusy();
		mCurrMail = new BmMail( ref);
		StartJob( mCurrMail.Get(), true);
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	MakeFocus( focused)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::MakeFocus(bool focused) {
	inherited::MakeFocus(focused);
	if (mScrollView) {
		mScrollView->Invalidate();
	}
}

/*------------------------------------------------------------------------------*\
	FrameResized()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::FrameResized( float newWidth, float newHeight) {
	inherited::FrameResized( newWidth, newHeight);
	if (mBodyPartView) {
		float widenedBy = newWidth-mBodyPartView->Frame().Width();
		float height = mBodyPartView->Frame().Height();
		mBodyPartView->ResizeTo( mBodyPartView->FixedWidth(), height);
		if (widenedBy > 0)
			mBodyPartView->Invalidate( BRect( newWidth-widenedBy, 0, newWidth, height));
	}
	if (mHeaderView) {
		float widenedBy = newWidth-mHeaderView->Frame().Width();
		float height = mHeaderView->Frame().Height();
		mHeaderView->ResizeTo( mHeaderView->FixedWidth(), height);
		if (widenedBy > 0)
			mHeaderView->Invalidate( BRect( newWidth-widenedBy, 0, newWidth, height));
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::JobIsDone( bool completed) {
	if (completed && mCurrMail) {
		BString displayText;
		BmBodyPartList* body = mCurrMail->Body();
		if (body) {
			mBodyPartView->ShowBody( body);
			BM_LOG2( BM_LogMailParse, BString("extracting parts to be displayed from body-structure"));
			BmModelItemMap::const_iterator iter;
			for( iter=body->begin(); iter != body->end(); ++iter) {
				BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( iter->second);
				DisplayBodyPart( displayText, bodyPart);
			}
		}
		BM_LOG2( BM_LogMailParse, BString("remove <CR>s from mailtext"));
		RemoveSetFromString( displayText, "\r");
		BM_LOG2( BM_LogMailParse, BString("converting mailtext to UTF8"));
		displayText = ConvertToUTF8( B_ISO1_CONVERSION, displayText.String());
		BM_LOG2( BM_LogMailParse, BString("setting mailtext into textview"));
		SetText( displayText.String());
		BM_LOG2( BM_LogMailParse, BString("done, mail is visible"));
		mHeaderView->ShowHeader( mCurrMail->Header());
		ScrollTo( 0,0);
	} else {
		mHeaderView->ShowHeader( NULL);
		SetText( "");
	}
	ContainerView()->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	DisplayBodyPart( displayText, bodypart)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::DisplayBodyPart( BString& displayText, const BmBodyPart* bodyPart) {
	if (!bodyPart->mIsMultiPart) {
		if (bodyPart->ShouldBeShownInline()) {
			// MIME-block should be shown inline, so we add it to our textview:
			if (displayText.Length()) {
				// we show a separator between two inline bodyparts
				displayText<<"- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -";
			}
			displayText.Append( bodyPart->DecodedData());
		} else {
			// MIME-block should be shown as an attachment:
//			displayText << "<< "<<type.mValue<<" name="<<type.mParams["name"]<<" >>\r\n";
		}
	} else {
		BmModelItemMap::const_iterator iter;
		for( iter=bodyPart->begin(); iter != bodyPart->end(); ++iter) {
			BmBodyPart* subPart = dynamic_cast<BmBodyPart*>( iter->second);
			DisplayBodyPart( displayText, subPart);
		}
	}
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::DetachModel() {
	mBodyPartView->DetachModel();
	inheritedController::DetachModel();
	ContainerView()->UnsetBusy();
	SetText( "");
	mHeaderView->ShowHeader( NULL, false);
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
	:	inherited( NULL, target, resizingMode, flags, true, true, true, B_FANCY_BORDER)
{
	SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR));
	ct_mpm = minmax;
	target->TargetedByScrollView( this);
	BRect hsFrame;
	BPoint hsLT;
	BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
	if (hScroller) {
		hsFrame = hScroller->Frame();
		hsLT = hsFrame.LeftTop();
		float bvSize = hsFrame.Height();
		hScroller->ResizeBy( -bvSize, 0.0);
		hScroller->MoveBy( bvSize, 0.0);
		mBusyView = new BmBusyView( BRect( hsLT.x, hsLT.y, hsLT.x+bvSize, hsLT.y+bvSize));
		AddChild( mBusyView);
	}
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
void BmMailViewContainer::Draw( BRect bounds) {
	inherited::Draw( bounds);
	if (m_target) {
		BRect bounds = Bounds();
		if (IsFocus() || m_target->IsFocus()) {
			SetHighColor( keyboard_navigation_color());
			StrokeRect( bounds);
		} else {
			const rgb_color BeDarkBorderPart = {184,184,184,255};
			const rgb_color BeLightBorderPart = {255,255,255,255};
			SetHighColor( BeLightBorderPart);
			StrokeRect( bounds);
			SetHighColor( BeDarkBorderPart);
			StrokeLine( bounds.LeftTop(), bounds.RightTop());
			StrokeLine( bounds.LeftTop(), bounds.LeftBottom());
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::FrameResized(float new_width, float new_height) {
	if (m_target)
		m_target->ResizeTo( new_width-B_V_SCROLL_BAR_WIDTH-4,new_height-B_H_SCROLL_BAR_HEIGHT-4);
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BRect BmMailViewContainer::layout(BRect rect)
{
	MoveTo(rect.LeftTop());
	ResizeTo(rect.Width(),rect.Height());
	BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
	if (mBusyView && hScroller) {
		BRect bvFrame = mBusyView->Frame();
		BRect hsFrame = hScroller->Frame();
		mBusyView->MoveTo( bvFrame.left, hsFrame.bottom-bvFrame.Height());
	}
	return rect;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::SetBusy() {
	if (mBusyView) mBusyView->SetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::UnsetBusy() {
	if (mBusyView) mBusyView->UnsetBusy();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::PulseBusyView() {
	if (mBusyView) mBusyView->Pulse();
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores MailView-state inside StateCache-folder:
\*------------------------------------------------------------------------------*/
bool BmMailView::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BString filename = BString( "MailView");
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive MailView-object");
		(err = cacheFile.SetTo( TheResources->StateInfoFolder(), filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store state-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}
