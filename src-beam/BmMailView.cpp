/*
	BmMailView.cpp
		$Id$
*/

#include <MenuItem.h>
#include <PopUpMenu.h>
#include <UTF8.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmBusyView.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmMailHeaderView.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRulerView.h"

/********************************************************************************\
	BmMailView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView* BmMailView::CreateInstance( minimax minmax, BRect frame, bool outbound) {
	// create standard mail-view:
	BmMailView* instance = new BmMailView( minmax, frame, outbound);
	// try to open state-cache-file...
	status_t err;
	BFile archiveFile;
	BString archiveFilename = BString("MailView") << (outbound ? "_out" : "_in");
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
BmMailView::BmMailView( minimax minmax, BRect frame, bool outbound) 
	:	inherited( frame, "MailView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
	,	inheritedController( "MailViewController")
	,	mOutbound( outbound)
	,	mCurrMail( NULL)
	,	mPartnerMailRefView( NULL)
	,	mRulerView( NULL)
	,	mShowRaw( false)
	,	mShowInlinesSeparately( true)
	,	mFontSize( 12)
{
	mHeaderView = new BmMailHeaderView( NULL);
	if (outbound)
		mHeaderView->ResizeTo( 0,0);
	else
		AddChild( mHeaderView);
	mBodyPartView = new BmBodyPartView( minimax( 0, 0, 1E5, 1E5), 0, 0, outbound);
	mBodyPartView->RemoveSelf();
	AddChild( mBodyPartView);
	mBodyPartView->MoveTo( mHeaderView->Frame().LeftBottom());
	mBodyPartView->ResizeTo( 0,0);
	BFont font( be_fixed_font);
	font.SetSize( mFontSize);
	SetFont( &font);
	SetFontAndColor( &font);
	if (outbound) {
		mRulerView = new BmRulerView( be_fixed_font);
		AddChild( mRulerView);
		mRulerView->MoveTo( mBodyPartView->Frame().LeftBottom());
	}
	CalculateVerticalOffset();
	MakeEditable( outbound);
	SetStylable( !outbound);
	SetWordWrap( true);
	mScrollView = new BmMailViewContainer( minmax, this, B_FOLLOW_NONE, 
														B_WILL_DRAW | B_FRAME_EVENTS);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::~BmMailView() {
	if (mOutbound)
		delete mHeaderView;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailView::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddBool( MSG_RAW, mOutbound ? false : mShowRaw)
						|| archive->AddString( MSG_FONTNAME, mFontName.String())
						|| archive->AddInt16( MSG_FONTSIZE, mFontSize);
	if (ret == B_OK && deep && mHeaderView)
		ret = mHeaderView->Archive( archive, deep);
	if (ret == B_OK && deep && mBodyPartView)
		ret = mBodyPartView->Archive( archive, deep);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailView::Unarchive( BMessage* archive, bool deep=true) {
	status_t ret = archive->FindBool( MSG_RAW, &mShowRaw)
						|| archive->FindString( MSG_FONTNAME, &mFontName)
						|| archive->FindInt16( MSG_FONTSIZE, &mFontSize);
	if (ret == B_OK && deep && mHeaderView)
		ret = mHeaderView->Unarchive( archive, deep);
	if (ret == B_OK && deep && mBodyPartView)
		ret = mBodyPartView->Unarchive( archive, deep);
	return ret;
}

/*------------------------------------------------------------------------------*\
	AttachedToWindow()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::AttachedToWindow() {
	inherited::AttachedToWindow();
	if (mOutbound) {
		SetFixedWidth( ThePrefs->GetInt( "MaxLineLen"));
	}
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
			case BM_MAILVIEW_SHOWRAW: {
				ShowRaw( true);
				JobIsDone( true);				// trigger re-display:
				break;
			}
			case BM_MAILVIEW_SHOWCOOKED: {
				ShowRaw( false);
				JobIsDone( true);				// trigger re-display:
				break;
			}
			case BM_MAILVIEW_SHOWINLINES_SEPARATELY: {
				ShowInlinesSeparately( true);
				JobIsDone( true);				// trigger re-display:
				break;
			}
			case BM_MAILVIEW_SHOWINLINES_CONCATENATED: {
				ShowInlinesSeparately( false);
				JobIsDone( true);				// trigger re-display:
				break;
			}
			case BM_RULERVIEW_NEW_POS: {
				SetFixedWidth( msg->FindInt32( BmRulerView::MSG_NEW_POS));
				break;
			}
			case B_SIMPLE_DATA: {
				mBodyPartView->AddAttachment( msg);
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
	if ( numBytes == 1 && !IsEditable()) {
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
	} else 
		inherited::KeyDown( bytes, numBytes);
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::MouseDown( BPoint point) {
	BPoint mousePos;
	uint32 buttons;
	GetMouse( &mousePos, &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowMenu( point);
		return;
	}
	inherited::MouseDown( point);
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
	float textWidth = TextRect().Width()+4;
	if (mBodyPartView) {
		float widenedBy = newWidth-mBodyPartView->Frame().Width();
		float height = mBodyPartView->Frame().Height();
		if (mOutbound)
			mBodyPartView->ResizeTo( MAX(textWidth,newWidth), height);
		else
			mBodyPartView->ResizeTo( mBodyPartView->FixedWidth(), height);
		if (widenedBy > 0)
			mBodyPartView->Invalidate( BRect( newWidth-widenedBy, 0, newWidth, height));
	}
	if (mHeaderView && !mOutbound) {
		float widenedBy = newWidth-mHeaderView->Frame().Width();
		float height = mHeaderView->Frame().Height();
		mHeaderView->ResizeTo( mHeaderView->FixedWidth(), height);
		if (widenedBy > 0)
			mHeaderView->Invalidate( BRect( newWidth-widenedBy, 0, newWidth, height));
	}
	if (mRulerView) {
		float widenedBy = newWidth-mRulerView->Frame().Width();
		float height = mRulerView->Frame().Height();
		mRulerView->ResizeTo( MAX(textWidth,newWidth), height);
		if (widenedBy > 0)
			mRulerView->Invalidate( BRect( newWidth-widenedBy, 0, newWidth, height));
	}
}

/*------------------------------------------------------------------------------*\
	AcceptsDrop()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailView::AcceptsDrop( const BMessage* msg) {
	return IsEditable();
}

/*------------------------------------------------------------------------------*\
	WrappedText()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::GetWrappedText( BString& out) {
	BString editedText = Text();
	WordWrap( editedText, out, FixedWidth(), "\n");
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMail( BmMailRef* ref, bool async) {
	try {
		StopJob();
		if (!ref) {
			DetachModel();
			mHeaderView->ShowHeader( NULL);
			return;
		}
		ContainerView()->SetBusy();
		mCurrMail = new BmMail( ref);
		StartJob( mCurrMail.Get(), async);
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMail( BmMail* mail, bool async) {
	try {
		StopJob();
		if (!mail || mail->InitCheck() != B_OK) {
			if (DataModel())
				DetachModel();
			mHeaderView->ShowHeader( NULL);
			return;
		}
		ContainerView()->SetBusy();
		mCurrMail = mail;
		StartJob( mCurrMail.Get(), async);
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
	if (completed && mCurrMail && mCurrMail->Header()) {
		BString displayText;
		mTextRunMap.clear();
		mTextRunMap[0] = Black;
		BmBodyPartList* body = mCurrMail->Body();
		if (body) {
			mBodyPartView->ShowBody( body);
			if (mShowRaw) {
				BM_LOG2( BM_LogMailParse, BString("displaying raw message"));
				uint32 encoding = mCurrMail->Header()->DefaultEncoding();
				ConvertToUTF8( encoding, mCurrMail->RawText(), displayText);
			} else {
				BM_LOG2( BM_LogMailParse, BString("extracting parts to be displayed from body-structure"));
				BmModelItemMap::const_iterator iter;
				for( iter=body->begin(); iter != body->end(); ++iter) {
					BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( iter->second.Get());
					DisplayBodyPart( displayText, bodyPart);
				}
				// add signature, if any:
				if (body->Signature().Length()) {
					mTextRunMap[displayText.Length()] = BeShadow;
					displayText << "-- \n" << body->Signature();
				}
				// highlight URLs:
				Regexx rx;
				int32 count;
				if ((count = rx.exec( displayText, "http://\\S+", 
					                  Regexx::global|Regexx::newline|Regexx::noatom)) > 0) {
					for( int i=0; i<count; ++i) {
						int32 start = rx.match[i].start();
						int32 end = start+rx.match[i].Length();
						mTextRunMap[start] = MedMetallicBlue;
						mTextRunMap[end] = Black;
					}
				}
			}
		}
		BM_LOG2( BM_LogMailParse, BString("remove <CR>s from mailtext"));
		RemoveSetFromString( displayText, "\r");
		BM_LOG2( BM_LogMailParse, BString("setting mailtext into textview"));
		// set up textrun-array
		int32 trsiz = sizeof( struct text_run);
		text_run_array* textRunArray = (text_run_array*)malloc( sizeof(int32)+trsiz*mTextRunMap.size());
		if (!textRunArray)
			return;
		textRunArray->count = mTextRunMap.size();
		BmTextRunMap::const_iterator iter;
		int i=0;
		for( iter = mTextRunMap.begin(); iter != mTextRunMap.end(); ++iter, ++i) {
			textRunArray->runs[i].offset = iter->first;
			textRunArray->runs[i].font = *be_fixed_font;
			textRunArray->runs[i].color = iter->second;
		}
		SetText( displayText.String(), displayText.Length(), textRunArray);
		free( textRunArray);
		BM_LOG2( BM_LogMailParse, BString("done, mail is visible"));
		mHeaderView->ShowHeader( mCurrMail->Header());
		ContainerView()->UnsetBusy();
		ScrollTo( 0,0);
		if (mCurrMail->Status() == BM_MAIL_STATUS_NEW)
			mCurrMail->MarkAs( BM_MAIL_STATUS_READ);
	} else {
		mHeaderView->ShowHeader( NULL);
		SetText( "");
		ContainerView()->UnsetBusy();
	}
}

/*------------------------------------------------------------------------------*\
	DisplayBodyPart( displayText, bodypart)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::DisplayBodyPart( BString& displayText, BmBodyPart* bodyPart) {
	if (!bodyPart->IsMultiPart()) {
		if (bodyPart->ShouldBeShownInline()) {
			// MIME-block should be shown inline, so we add it to our textview:
			if (displayText.Length() && mShowInlinesSeparately) {
				// we show a separator between two inline bodyparts
				if (bodyPart->FileName().Length()) {
					displayText << "\n- - - - - - - - - - - - - - - - - - - -\n";
					displayText << "Inline Attachment <" << bodyPart->FileName() << "> follows:\n";
				}
				displayText << "- - - - - - - - - - - - - - - - - - - -\n\n";
			}
			uint32 encoding = CharsetToEncoding( bodyPart->TypeParam("charset"));
			BString utf8;
			ConvertToUTF8( encoding, bodyPart->DecodedData(), utf8);
			displayText.Append( utf8);
		}
	} else {
		BmModelItemMap::const_iterator iter;
		for( iter=bodyPart->begin(); iter != bodyPart->end(); ++iter) {
			BmBodyPart* subPart = dynamic_cast<BmBodyPart*>( iter->second.Get());
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

/*------------------------------------------------------------------------------*\
	WriteStateInfo()
		-	stores MailView-state inside StateCache-folder:
\*------------------------------------------------------------------------------*/
bool BmMailView::WriteStateInfo() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BString filename = BString( "MailView") << (mOutbound ? "_out": "_in");
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

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "MailViewMenu", false, false);

	BMenuItem* item = NULL;
	if (!mOutbound) {
		item = new BMenuItem( "Show All MIME-Bodies", 
									 new BMessage( mBodyPartView->ShowAllParts()
															  ? BM_BODYPARTVIEW_SHOWATTACHMENTS
															  : BM_BODYPARTVIEW_SHOWALL));
		item->SetTarget( mBodyPartView);
		item->SetMarked( mBodyPartView->ShowAllParts());
		theMenu->AddItem( item);
	}

	item = new BMenuItem( mOutbound ? "Edit Raw Message": "Show Raw Message", new BMessage( ShowRaw()
								 ? BM_MAILVIEW_SHOWCOOKED : BM_MAILVIEW_SHOWRAW));
	item->SetTarget( this);
	item->SetMarked( ShowRaw());
	theMenu->AddItem( item);

	if (!mOutbound) {
		item = new BMenuItem( "Separate Inlines", new BMessage( ShowInlinesSeparately() 
									 ? BM_MAILVIEW_SHOWINLINES_CONCATENATED
									 : BM_MAILVIEW_SHOWINLINES_SEPARATELY));
		item->SetTarget( this);
		item->SetMarked( ShowInlinesSeparately());
		theMenu->AddItem( item);
	}

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
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
