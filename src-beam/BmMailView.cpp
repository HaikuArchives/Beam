/*
	BmMailView.cpp
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/

#include <Clipboard.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <UTF8.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmBusyView.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmMailHeaderView.h"
#include "BmMailRef.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRulerView.h"
#include "BmSignature.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

/*------------------------------------------------------------------------------*\
	WordWrap( in, out, maxLineLen)
		-	wraps given in-string along word-boundary
		-	param maxLineLen indicates right border for wrap
		-	resulting text is stored in param out
		-	the string in has to be UTF8-encoded for this function to work 
			correctly!
		-	urls will be preserved (i.e. not be wrapped).
\*------------------------------------------------------------------------------*/
void WordWrap( const BmString& in, BmString& out, int32 maxLineLen, 
					BmString nl) {
	if (!in.Length()) {
		out.Truncate( 0, false);
		return;
	}
	Regexx rx;
	Regexx rxUrl;
	int32 lastPos = 0;
	const char *s = in.String();
	bool needBreak = false;
	bool isUrl = false;
	BmStringOBuf tempIO( in.Length()*1.1, 1.1);
	for( int32 pos = 0;  !needBreak;  pos += nl.Length(), lastPos = pos) {
		pos = in.FindFirst( nl, pos);
		if (pos == B_ERROR) {
			// handle the characters between last newline and end of string:
			pos = in.Length();
			needBreak = true;
		}
		// determine length of line in UTF8-characters (not bytes)
		// and find last space before maxLineLen-border:
		int32 lastSpcPos = B_ERROR;
		int32 lineLen = 0;
		for( int i=lastPos; i<pos; ++i) {
			while( i<pos && IS_WITHIN_UTF8_MULTICHAR(s[i]))
				i++;
			if (s[i] == ' ' && lineLen<maxLineLen)
				lastSpcPos = i;
			lineLen++;
		}
		while (lineLen > maxLineLen) {
			if (lastSpcPos>lastPos) {
				// special-case lines containing only quotes and a long word,
				// since in this case we want to avoid wrapping between quotes
				// and the word:
				BmString lineBeforeSpace;
				in.CopyInto( lineBeforeSpace, lastPos, 1+lastSpcPos-lastPos);
				if (rx.exec( lineBeforeSpace, 
								 ThePrefs->GetString( "QuotingLevelRX"))) {
					BmString text=rx.match[0].atom[1];
					if (!text.Length()) {
						// the subpart before last space consists only of the quote,
						// we avoid wrapping this line:
						lastSpcPos=B_ERROR;
					}
				}
			}
			if (lastSpcPos==B_ERROR || lastSpcPos<lastPos) {
				// line doesn't contain any space character (before maxline-length), 
				// we simply break it at right margin (unless it's an URL):
				isUrl = (
					rxUrl.exec( 
						BmString( in.String()+lastPos, pos-lastPos),
						"(https?://|ftp://|nntp://|file://|mailto:)",
						Regexx::nocase
					)
				);
				if (isUrl) {
					// find next space or end of line and break line there:
					int32 nextSpcPos = in.FindFirst( " ", lastPos+maxLineLen);
					int32 nlPos = in.FindFirst( nl, lastPos+maxLineLen);
					if (nextSpcPos==B_ERROR || nlPos<nextSpcPos) {
						// have no space in line, we keep whole line:
						if (nlPos == B_ERROR) {
							tempIO.Write( in.String()+lastPos, in.Length()-lastPos);
							tempIO.Write( nl);
							lastPos = in.Length();
						} else {
							tempIO.Write( in.String()+lastPos, 
											  nl.Length()+nlPos-lastPos);
							lastPos = nlPos + nl.Length();
						}
					} else {
						// break long line at a space behind right margin:
						tempIO.Write( in.String()+lastPos, 1+nextSpcPos-lastPos);
						tempIO.Write( nl);
						lastPos = nextSpcPos+1;
					}
				} else {
					// break line at right margin:
					tempIO.Write( in.String()+lastPos, maxLineLen);
					tempIO.Write( nl);
					lastPos += maxLineLen;
				}
			} else {
				// wrap line after last space:
				tempIO.Write( in.String()+lastPos, 1+lastSpcPos-lastPos);
				tempIO.Write( nl);
				lastPos = lastSpcPos+1;
			}
			lineLen = 0;
			lastSpcPos = B_ERROR;
			for( int i=lastPos; i<pos; ++i) {
				while( i<pos && IS_WITHIN_UTF8_MULTICHAR(s[i]))
					i++;
				if (s[i] == ' ' && lineLen<maxLineLen)
					lastSpcPos = i;
				lineLen++;
			}
		}
		if (needBreak)
			break;
		tempIO.Write( in.String()+lastPos, nl.Length()+pos-lastPos);
	}
	if (lastPos < in.Length())
		tempIO.Write( in.String()+lastPos, in.Length()-lastPos);
	out.Adopt( tempIO.TheString());
}

/********************************************************************************\
	BmMailView
\********************************************************************************/

enum {
	BM_MARK_AS_READ = 'BmMR'
};

const char* const BmMailView::MSG_VERSION =	"bm:version";
const char* const BmMailView::MSG_RAW = 		"bm:raw";
const char* const BmMailView::MSG_FONTNAME = "bm:fnt";
const char* const BmMailView::MSG_FONTSIZE =	"bm:fntsz";

const char* const BmMailView::MSG_MAIL =		"bm:mail";

const int16 BmMailView::nArchiveVersion = 3;

const char* const BmMailView::MSG_HAS_MAIL = "bm:hmail";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView* BmMailView::CreateInstance( minimax minmax, BRect frame, 
													 bool outbound) {
	// create standard mail-view:
	BmMailView* instance = new BmMailView( minmax, frame, outbound);
	// try to open state-cache-file...
	status_t err;
	BFile archiveFile;
	BmString archiveFilename 
		= BmString("MailView") << (outbound ? "_out" : "_in");
	if ((err = archiveFile.SetTo( TheResources->StateInfoFolder(), 
											archiveFilename.String(), 
											B_READ_ONLY)) == B_OK) {
		// ...ok, archive file found, we fetch our state from it:
		try {
			BMessage archive;
			if ((err = archive.Unflatten( &archiveFile)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not fetch mail-view archive from file\n\t<") 
						<< archiveFilename << ">\n\n Result: " << strerror(err)
				);
			instance->Unarchive( &archive);
		} catch (BM_error &e) {
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
	,	mReadRunner( NULL)
	,	mShowingUrlCursor( false)
	,	mHaveMail( false)
	,	mDisplayInProgress( false)
{
	mHeaderView = new BmMailHeaderView( NULL);
	if (outbound)
		mHeaderView->ResizeTo( 0,0);
	else
		AddChild( mHeaderView);
	mBodyPartView 
		= new BmBodyPartView( minimax( 0, 0, 1E5, 1E5), 0, 0, outbound);
	mBodyPartView->RemoveSelf();
	AddChild( mBodyPartView);
	mBodyPartView->MoveTo( mHeaderView->Frame().LeftBottom());
	mBodyPartView->ResizeTo( 0,0);
	mFont = *be_fixed_font;
	mFont.SetSize( mFontSize);
	MakeEditable( outbound);
	SetDoesUndo( outbound);
	SetStylable( true);
	SetWordWrap( true);
	UpdateFont( mFont);
	if (outbound) {
		mRulerView = new BmRulerView( &mFont);
		AddChild( mRulerView);
		mRulerView->MoveTo( mBodyPartView->Frame().LeftBottom());
		BmString undoMode = ThePrefs->GetString( "UndoMode", "Words");
		if (undoMode.ICompare("Words") == 0)
			m_separator_chars = " \n\t,;.";
		else if (undoMode.ICompare("Paragraphs") == 0)
			m_separator_chars = "\n";
		else if (undoMode.ICompare("None") == 0)
			m_separator_chars = "";
	}
	CalculateVerticalOffset();
	mScrollView = new BmMailViewContainer( minmax, this, B_FOLLOW_NONE, 
														B_WILL_DRAW | B_FRAME_EVENTS);

	SetViewUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetLowUIColor(B_UI_DOCUMENT_BACKGROUND_COLOR);
	SetHighUIColor(B_UI_DOCUMENT_TEXT_COLOR);														
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::~BmMailView() {
	if (mOutbound)
		delete mHeaderView;
	delete mReadRunner;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailView::Archive( BMessage* archive, bool deep) const {
	status_t ret = archive->AddInt16( MSG_VERSION, nArchiveVersion)
						|| archive->AddBool( MSG_RAW, mOutbound ? false : mShowRaw)
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
status_t BmMailView::Unarchive( BMessage* archive, bool deep) {
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 1;
	status_t ret = archive->FindBool( MSG_RAW, &mShowRaw)
						|| archive->FindInt16( MSG_FONTSIZE, &mFontSize);
	mFontName = archive->FindString( MSG_FONTNAME);
	if (ret == B_OK && deep && mHeaderView)
		ret = mHeaderView->Unarchive( archive, deep);
	if (ret == B_OK && deep && mBodyPartView)
		ret = mBodyPartView->Unarchive( archive, deep);
	int32 pos = mFontName.FindFirst( ",");
	if (pos != B_ERROR) {
		BmString family( mFontName.String(), pos);
		BmString style( mFontName.String()+pos+1);
		mFont.SetFamilyAndStyle( family.String(), style.String());
	} else {
		mFont = *be_fixed_font;
	}
	mFont.SetSize( mFontSize);
	SetFont( &mFont);
	SetFontAndColor( &mFont);
	if (mOutbound && mRulerView)
		mRulerView->SetMailViewFont( mFont);
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
				break;
			}
			case BM_JOB_DONE: {
				if (!IsMsgFromCurrentModel( msg)) return;
				BM_LOG2( BM_LogModelController, 
							BmString("Model <") 
								<< FindMsgString( msg, BmDataModel::MSG_MODEL)
								<< "> has told it is done.");
				JobIsDone( FindMsgBool( msg, BmJobModel::MSG_COMPLETED));
				break;
			}
			case BMM_SWITCH_RAW: {
				ShowRaw( !ShowRaw());
				JobIsDone( true);				// trigger re-display:
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
			case BM_FONT_SELECTED: {
				BmString family = msg->FindString( BmResources::BM_MSG_FONT_FAMILY);
				BmString style = msg->FindString( BmResources::BM_MSG_FONT_STYLE);
				mFont.SetFamilyAndStyle( family.String(), style.String());
				mFontName = family + "," + style;
				if (mOutbound && mRulerView)
					mRulerView->SetMailViewFont( mFont);
				UpdateFont( mFont);
				ResetTextRect();
				WriteStateInfo();
				break;
			}
			case BM_FONTSIZE_SELECTED: {
				mFontSize = msg->FindInt16( BmResources::BM_MSG_FONT_SIZE);
				mFont.SetSize( mFontSize);
				if (mOutbound && mRulerView)
					mRulerView->SetMailViewFont( mFont);
				UpdateFont( mFont);
				ResetTextRect();
				WriteStateInfo();
				break;
			}
			case B_SIMPLE_DATA: {
				mBodyPartView->AddAttachment( msg);
				break;
			}
			case BM_MAILVIEW_COPY_URL: {
				BmString urlStr = msg->FindString( "url");
				BMessage* clipMsg;
				if (be_clipboard->Lock()) {
					be_clipboard->Clear();
					if ((clipMsg = be_clipboard->Data())!=NULL) {
						clipMsg->AddData( "text/plain", B_MIME_TYPE, urlStr.String(), 
												urlStr.Length());
						be_clipboard->Commit();
					}
					be_clipboard->Unlock();
				}
				break;
			}
			case BM_MAILVIEW_SELECT_CHARSET: {
				if (mCurrMail) {
					BmRef< BmBodyPart> textBody( 
						mCurrMail->Body()->EditableTextBody());
					if (textBody) {
						textBody->SuggestCharset( msg->FindString( MSG_CHARSET));
						JobIsDone( true);
					}
				}
				break;
			}
			case BM_MARK_AS_READ: {
				BmMail* mail=NULL;
				msg->FindPointer( MSG_MAIL, (void**)&mail);
				if (mCurrMail == mail)
					mCurrMail->MarkAs( BM_MAIL_STATUS_READ);
				break;
			}
			case B_MOUSE_WHEEL_CHANGED: {
				if (modifiers() 
				& (B_SHIFT_KEY | B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY)) {
					bool passedOn = false;
					if (mPartnerMailRefView 
					&& !(passedOn = msg->FindBool("bm:passed_on"))) {
						BMessage msg2(*msg);
						msg2.AddBool("bm:passed_on", true);
						Looper()->PostMessage( &msg2, mPartnerMailRefView);
						return;
					}
				}
				inherited::MessageReceived( msg);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString(ControllerName()) << ":\n\t" << err.what());
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
			case B_DOWN_ARROW: {
				int32 mods = Window()->CurrentMessage()->FindInt32("modifiers");
				if (mods 
				& (B_LEFT_CONTROL_KEY | B_RIGHT_OPTION_KEY | B_SHIFT_KEY)) {
					// remove modifiers so we don't ping-pong endlessly:
					Window()->CurrentMessage()->ReplaceInt32("modifiers", 0);
					if (mPartnerMailRefView)
						mPartnerMailRefView->KeyDown( bytes, numBytes);
				} else
					inherited::KeyDown( bytes, numBytes);
				break;
			}
			case B_DELETE: {
				if (mCurrMail && mPartnerMailRefView)
					mPartnerMailRefView->KeyDown( bytes, numBytes);
				else
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
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons)==B_OK) {
		if (buttons == B_PRIMARY_MOUSE_BUTTON) {
			int32 offset =  OffsetAt( point);
			mClickedTextRun = TextRunInfoAt( offset);
		} else if (buttons == B_SECONDARY_MOUSE_BUTTON) {
			ShowMenu( point);
			return;
		}
	}
	inherited::MouseDown( point);
}

/*------------------------------------------------------------------------------*\
	MouseUp( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::MouseUp( BPoint point) {
	inherited::MouseUp( point);
	int32 offset =  OffsetAt( point);
	if (mCurrMail && mClickedTextRun == TextRunInfoAt( offset)) {
		BmTextRunInfo runInfo = mClickedTextRun->second;
		if (runInfo.isURL) {
			BmString url = GetTextForTextrun( mClickedTextRun);
			bmApp->LaunchURL( url);
		}
	}
	mClickedTextRun = mTextRunMap.end();
}

/*------------------------------------------------------------------------------*\
	IsOverURL( point)
		-	returns true if given point is over an URL, false if not
\*------------------------------------------------------------------------------*/
bool BmMailView::IsOverURL( BPoint point) {
	if (mTextRunMap.size() > 0) {
		int32 currPos = OffsetAt( point);
		BmTextRunMap::const_iterator iter = TextRunInfoAt( currPos);
		return (iter != mTextRunMap.end() && iter->second.isURL);
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	GetTextForTextrun( run)
		-	returns text for given textrun
\*------------------------------------------------------------------------------*/
BmString BmMailView::GetTextForTextrun( BmTextRunIter run) {
	BmTextRunIter next = run;
	next++;
	Select( run->first, next->first);
	BmString url( Text()+run->first, next->first-run->first);
	return url;
}

/*------------------------------------------------------------------------------*\
	MouseMoved( point, transit, msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::MouseMoved( BPoint point, uint32 transit, 
									  const BMessage *msg) {
	inherited::MouseMoved( point, transit, msg);
	if (mCurrMail && IsOverURL( point)) {
		if (!mShowingUrlCursor) {
			SetViewCursor( &TheResources->mUrlCursor);
			mShowingUrlCursor = true;
		}
	} else {
		if (mShowingUrlCursor) {
			SetViewCursor( B_CURSOR_I_BEAM);
			mShowingUrlCursor = false;
		}
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
	float textWidth = TextRect().Width()+4;
	float widenedBy = newWidth-textWidth;
	if (mBodyPartView) {
		float height = mBodyPartView->Frame().Height();
		if (mOutbound)
			mBodyPartView->ResizeTo( MAX(textWidth,newWidth), height);
		else
			mBodyPartView->ResizeTo( mBodyPartView->FixedWidth(), height);
		if (widenedBy > 0)
			mBodyPartView->Invalidate( BRect( newWidth-widenedBy, 0, 
														 newWidth, height));
	}
	if (mHeaderView && !mOutbound) {
		float height = mHeaderView->Frame().Height();
		mHeaderView->ResizeTo( mHeaderView->FixedWidth(), height);
		if (widenedBy > 0)
			mHeaderView->Invalidate( BRect( newWidth-widenedBy, 0, 
													  newWidth, height));
	}
	if (mRulerView) {
		float height = mRulerView->Frame().Height();
		mRulerView->ResizeTo( MAX(textWidth,newWidth), height);
		if (widenedBy > 0)
			mRulerView->Invalidate( BRect( newWidth-widenedBy, 0, 
													 newWidth, height));
	}
}

/*------------------------------------------------------------------------------*\
	SetSignatureByName()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::SetSignatureByName( const BmString sigName) {
	if (!mOutbound)
		return;
	BmString text = Text();
	Regexx rx;
	BmString sigRX = ThePrefs->GetString( "SignatureRX");
	if (rx.exec( text, sigRX, Regexx::newline))
		text.Truncate( rx.match[0].start());	// cut off old signature
	if (text.ByteAt(text.Length()-1) != '\n')
		text << '\n';
	int32 trsiz = sizeof( struct text_run);
	text_run_array* textRunArray 
		= (text_run_array*)malloc( sizeof(int32)+trsiz*2);
	if (!textRunArray)
		return;
	textRunArray->count = 2;
	textRunArray->runs[0].offset = 0;
	textRunArray->runs[0].font = mFont;
	textRunArray->runs[0].color = ui_color(B_UI_DOCUMENT_TEXT_COLOR);
	textRunArray->runs[1].offset = text.Length();
	textRunArray->runs[1].font = mFont;
	textRunArray->runs[1].color = BmWeakenColor(B_UI_DOCUMENT_TEXT_COLOR,2);
	BmString sig = TheSignatureList->GetSignatureStringFor( sigName);
	int32 lastTextPos = max( (long)0, text.Length()-1);
	if (sig.Length())
		text << "-- \n" << sig;
	SetText( text.String(), text.Length(), textRunArray);
	// now show the signature that we've just inserted (position cursor right
	// before it):
	Select( lastTextPos, lastTextPos);
	ScrollToSelection();
}

/*------------------------------------------------------------------------------*\
	UpdateFont()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::UpdateFont( const BFont& font) {
	SetTabWidth( font.StringWidth( BM_SPACES.String(), 
					 ThePrefs->GetInt( "SpacesPerTab", 4)));
							// arrange tab-stops to correspond with tab-width
	SetFont( &font);
	SetFontAndColor( &font);
	int32 len = TextLength();
	text_run_array* textRunArray = RunArray( 0, len);
	if (!textRunArray)
		return;
	for( int i=0; i<textRunArray->count; ++i)
		textRunArray->runs[i].font = font;
	SetRunArray( 0, len, textRunArray);
	free( textRunArray);
}

/*------------------------------------------------------------------------------*\
	AcceptsDrop()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailView::AcceptsDrop( const BMessage*) {
	return IsEditable();
}

/*------------------------------------------------------------------------------*\
	CanEndLine()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailView::CanEndLine( int32 offset) {
	if (strchr( "/&=", ByteAt( offset)) != NULL)
		return false;
	return inherited::CanEndLine( offset);
}

/*------------------------------------------------------------------------------*\
	InsertText()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::InsertText( const char *text, int32 length, int32 offset,
									  const text_run_array *runs)
{
	if (mOutbound && mRulerView && mFont.IsFixed()) {
		int32 lineStart = OffsetAt( LineAt( offset));
		BmString line( Text()+lineStart, offset-lineStart);
		int32 currLinePos = line.CountChars();
		if (currLinePos > mRulerView->IndicatorPos()) {
			while( currLinePos+length > mRulerView->IndicatorPos()+1 
			&& (*text==B_SPACE || *text==B_TAB)) {
				text++;
				length--;
			}
		}
	}
  	if (mOutbound && runs) {
  		text_run_array* r = const_cast<text_run_array*>( runs);
  		for( int32 i=0; i<r->count; ++i)
  			r->runs[i].font = mFont;
  	}
	inherited::InsertText( text, length, offset, runs);
}

/*------------------------------------------------------------------------------*\
	GetWrappedText()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::GetWrappedText( BmString& out, bool hardWrapIfNeeded) {
	if (hardWrapIfNeeded && ThePrefs->GetBool( "HardWrapMailText")) {
		// we are in hard-wrap mode, so we use the right margin from the 
		// rulerview as right border and wrap the mail-text accordingly:
		BmString editedText = Text();
		int32 lineLen = mRulerView->IndicatorPos();
		WordWrap( editedText, out, lineLen, "\n");
	} else {
		// we are in softwrap mode:
		out = Text();
	}
	// update mail's right margin according to rulerview:
	mCurrMail->RightMargin( mRulerView->IndicatorPos());
}

/*------------------------------------------------------------------------------*\
	ShowMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::ShowMail( BmMailRef* ref, bool async) {
	try {
		StopJob();
		if (!ref) {
			if (DataModel())
				DetachModel();
			mHeaderView->ShowHeader( NULL);
			mBodyPartView->ShowBody( NULL);
			mCurrMail = NULL;
			SendNoticesIfNeeded( false);
			return;
		}
		mCurrMail = BmMail::CreateInstance( ref);
		mDisplayInProgress = true;
		StartJob( mCurrMail.Get(), async);
		if (async)
			ContainerView()->SetBusy();
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailView: ") << err.what());
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
			mCurrMail = NULL;
			mHeaderView->ShowHeader( NULL);
			mBodyPartView->ShowBody( NULL);
			SendNoticesIfNeeded( false);
			return;
		}
		mCurrMail = mail;
		mDisplayInProgress = true;
		StartJob( mCurrMail.Get(), async);
		if (async)
			ContainerView()->SetBusy();
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::JobIsDone( bool completed) {
	if (completed && mCurrMail && mCurrMail->Header()) {
		mConversionError.Truncate( 0);
		BmString displayText;
		BmStringOBuf displayBuf( mShowRaw 
											? mCurrMail->RawText().Length()
											: 65536);
		mTextRunMap.clear();
		mTextRunMap[0] = BmTextRunInfo( ui_color(B_UI_DOCUMENT_TEXT_COLOR));
		BmBodyPartList* body = mCurrMail->Body();
		mClickedTextRun = mTextRunMap.end();
		if (mRulerView)
			mRulerView->SetIndicatorPos( mCurrMail->RightMargin());
		if (body) {
			mBodyPartView->ShowBody( body);
			if (mShowRaw) {
				BM_LOG2( BM_LogMailParse, BmString("displaying raw message"));
				BmString charset = mCurrMail->DefaultCharset();
				BmStringIBuf text( mCurrMail->RawText());
				BmLinebreakDecoder decoder( &text);
				BmUtf8Encoder textConverter( &decoder, charset);
				displayBuf.Write( &textConverter);
				displayText.Adopt( displayBuf.TheString());
			} else {
				BM_LOG2( BM_LogMailParse, 
							BmString("extracting parts to be displayed from "
										"body-structure"));
				BmModelItemMap::const_iterator iter;
				for( iter=body->begin(); iter != body->end(); ++iter) {
					BmBodyPart* bodyPart 
						= dynamic_cast<BmBodyPart*>( iter->second.Get());
					DisplayBodyPart( displayBuf, bodyPart);
				}
				// add signature, if any:
				if (body->Signature().Length()) {
					uint32 len = displayBuf.CurrPos();
					if (!len || displayBuf.ByteAt(len-1) != '\n')
						displayBuf << "\n";
					mTextRunMap[displayBuf.CurrPos()] 
						= BmTextRunInfo( BmWeakenColor(B_UI_DOCUMENT_TEXT_COLOR,2));
					// signature within body is already in UTF8-encoding, so we
					// just add it to out display-text:
					displayBuf << "-- \n" << body->Signature();
				}
				displayText.Adopt( displayBuf.TheString());
			}
			if (!mOutbound) {
				// highlight URLs:
				Regexx rx;
				int32 count;
				if ((count = rx.exec( 
					displayText, 
					"(https?://|ftp://|nntp://|file://|mailto:)[^][<>(){}|\"\\s]+", 
					Regexx::nocase|Regexx::global|Regexx::newline
				)) > 0) {
					for( int i=0; i<count; ++i) {
						int32 start = rx.match[i].start();
						int32 end = start+rx.match[i].Length();
						BmTextRunIter iter = TextRunInfoAt( start);
						BmTextRunInfo runInfo = iter->second;
						mTextRunMap[start] = BmTextRunInfo( 
							ui_color( B_UI_CONTROL_HIGHLIGHT_COLOR), true
						);
						mTextRunMap[end] = runInfo;
					}
				}
			}
		}
//		BM_LOG2( BM_LogMailParse, BmString("remove <CR>s from mailtext"));
//		displayText.RemoveAll( "\r");
		BM_LOG2( BM_LogMailParse, BmString("setting mailtext into textview"));
		// set up textrun-array
		int32 trsiz = sizeof( struct text_run);
		text_run_array* textRunArray 
			= (text_run_array*)malloc( sizeof(int32)+trsiz*mTextRunMap.size());
		if (!textRunArray)
			goto out;
		textRunArray->count = mTextRunMap.size();
		int i=0;
		BmTextRunIter iter;
		for( iter = mTextRunMap.begin(); iter != mTextRunMap.end(); ++iter, ++i) {
			textRunArray->runs[i].offset = iter->first;
			textRunArray->runs[i].font = mFont;
			textRunArray->runs[i].color = iter->second.color;
		}
		SetText( displayText.String(), displayText.Length(), textRunArray);
		free( textRunArray);
		mHeaderView->ShowHeader( mCurrMail->Header().Get());
		BM_LOG2( BM_LogMailParse, BmString("done, mail is visible"));
		ContainerView()->UnsetBusy();
		ScrollTo( 0,0);
		if (mCurrMail->Status() == BM_MAIL_STATUS_NEW) {
			if (mReadRunner) {
				delete mReadRunner;
				mReadRunner = NULL;
			}
			int32 readDelay = ThePrefs->GetInt( "MarkAsReadDelay", 2000);
			if (readDelay>0) {
				BMessage msg( BM_MARK_AS_READ);
				msg.AddPointer( MSG_MAIL, (void*)mCurrMail.Get());
				BMessenger msgr( this);
				mReadRunner = new BMessageRunner( msgr, &msg, readDelay*1000, 1);
			}
		}
		SendNoticesIfNeeded( true);
		if (mConversionError.Length())
			BM_SHOWERR( mConversionError);
	} else {
		BM_LOG2( BM_LogMailParse, BmString("setting empty mail into textview"));
		mHeaderView->ShowHeader( NULL);
		SetText( "");
		ContainerView()->UnsetBusy();
		SendNoticesIfNeeded( false);
	}
out:
	mDisplayInProgress = false;
}

/*------------------------------------------------------------------------------*\
	IsDisplayComplete()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailView::IsDisplayComplete() {
	return !IsJobRunning() && !mBodyPartView->IsJobRunning()
			 && !mDisplayInProgress;
}

/*------------------------------------------------------------------------------*\
	SendNoticesIfNeeded()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::SendNoticesIfNeeded( bool haveMail) {
	if (haveMail != mHaveMail) {
		mHaveMail = haveMail;
		BMessage msg(BM_NTFY_MAIL_VIEW);
		msg.AddBool( MSG_HAS_MAIL, mHaveMail);
		SendNotices( BM_NTFY_MAIL_VIEW, &msg);
	}
}

/*------------------------------------------------------------------------------*\
	DisplayBodyPart( displayText, bodypart)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::DisplayBodyPart( BmStringOBuf& displayBuf, 
											 BmBodyPart* bodyPart) {
	if (!bodyPart->IsMultiPart()) {
		if (bodyPart->ShouldBeShownInline()) {
			// MIME-block should be shown inline, so we add it to our textview:
			if (displayBuf.CurrPos() && mShowInlinesSeparately) {
				// we show a separator between two inline bodyparts
				if (bodyPart->FileName().Length()) {
					displayBuf << "\n- - - - - - - - - - - - - - - - - - - -\n";
					displayBuf << "Inline Attachment <" << bodyPart->FileName() 
								  << "> follows:\n";
				}
				displayBuf << "- - - - - - - - - - - - - - - - - - - -\n\n";
			}
			if (bodyPart->IsBinary()) {
				// Binary subparts are not automatically being converted to local
				// newlines. Since we are going to display this binary subpart
				// in the mailview, we have to convert the newlines first:
				BmString convertedData;
				convertedData.ConvertLinebreaksToLF( &bodyPart->DecodedData());
				displayBuf << convertedData;
			} else {
				// standard stuff, bodypart has already been converted to local 
				// newlines
				displayBuf << bodyPart->DecodedData();
				if (bodyPart->HadErrorDuringConversion() 
				&& bodyPart == mCurrMail->Body()->EditableTextBody().Get()) {
					mConversionError 
						= BmString("The mailtext contains characters that ")
							<< "could not be converted from the\n"
							<< "proposed charset (" 
							<< bodyPart->SuggestedCharset() 
							<< ") into UTF-8.\n\nSome parts of the mailtext "
							<< "may be missing or\nmay be displayed incorrectly.\n\n"
							<< "Please try another charset.";
				}
			}
		}
	} else {
		BmModelItemMap::const_iterator iter;
		for( iter=bodyPart->begin(); iter != bodyPart->end(); ++iter) {
			BmBodyPart* subPart = dynamic_cast<BmBodyPart*>( iter->second.Get());
			DisplayBodyPart( displayBuf, subPart);
		}
	}
}

/*------------------------------------------------------------------------------*\
	TextRunInfoAt()
		-	
\*------------------------------------------------------------------------------*/
BmMailView::BmTextRunIter BmMailView::TextRunInfoAt( int32 pos) const {
	BmTextRunIter theIter;
	BmTextRunIter iter;
	for( iter = mTextRunMap.begin(); iter != mTextRunMap.end(); ++iter) {
		if (iter->first > pos)
			break;
		theIter = iter;
	}
	return theIter;
}

/*------------------------------------------------------------------------------*\
	DetachModel()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::DetachModel() {
	mBodyPartView->DetachModel();
	inheritedController::DetachModel();
	if (LockLooper()) {
		ContainerView()->UnsetBusy();
		SetText( "");
		mHeaderView->ShowHeader( NULL, false);
		UnlockLooper();
	}
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
		BmString filename = BmString( "MailView") << (mOutbound ? "_out": "_in");
		if (this->Archive( &archive, true) != B_OK)
			BM_THROW_RUNTIME("Unable to archive MailView-object");
		if ((err = cacheFile.SetTo( 
			TheResources->StateInfoFolder(), 
			filename.String(), 
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE
		)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not create cache file\n\t<") << filename 
					<< ">\n\n Result: " << strerror(err)
			);
		if ((err = archive.Flatten( &cacheFile)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not store state-cache into file\n\t<") 
					<< filename << ">\n\n Result: " << strerror(err)
			);
	} catch( BM_error &e) {
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

	BFont font( *be_plain_font);
	font.SetSize( 10);
	theMenu->SetFont( &font);

	BMenuItem* item = NULL;
	if (!mOutbound) {
		item = new BMenuItem( "Show All MIME-Bodies", 
									 new BMessage( mBodyPartView->ShowAllParts()
															  ? BM_BODYPARTVIEW_SHOWATTACHMENTS
															  : BM_BODYPARTVIEW_SHOWALL));
		item->SetTarget( mBodyPartView);
		item->SetMarked( mBodyPartView->ShowAllParts());
		theMenu->AddItem( item);
		item = new BMenuItem( "Show Raw Message", new BMessage( ShowRaw()
									 ? BM_MAILVIEW_SHOWCOOKED : BM_MAILVIEW_SHOWRAW));
		item->SetTarget( this);
		item->SetMarked( ShowRaw());
		theMenu->AddItem( item);

		item = new BMenuItem( 
			"Separate Inlines", 
			new BMessage( 
			 	ShowInlinesSeparately() 
					 ? BM_MAILVIEW_SHOWINLINES_CONCATENATED
					 : BM_MAILVIEW_SHOWINLINES_SEPARATELY
			)
		);
		item->SetTarget( this);
		item->SetMarked( ShowInlinesSeparately());
		theMenu->AddItem( item);
		theMenu->AddSeparatorItem();

		if (IsOverURL( point)) {
			BMessage* msg = new BMessage( BM_MAILVIEW_COPY_URL);
			int32 currPos = OffsetAt( point);
			BmTextRunMap::const_iterator run = TextRunInfoAt( currPos);
			msg->AddString( "url", GetTextForTextrun( run).String());
			item = new BMenuItem( "Copy URL to Clipboard", msg);
			item->SetTarget( this);
			theMenu->AddItem( item);
			theMenu->AddSeparatorItem();
		}
		
		if (mCurrMail && mCurrMail->Body()) {
			BmRef< BmBodyPart> textBody( mCurrMail->Body()->EditableTextBody());
			if (textBody) {
				BMenu* menu = new BMenu( "Try Charset...");
				menu->SetFont( &font);
				AddCharsetMenu( menu, this, BM_MAILVIEW_SELECT_CHARSET);
				BMenuItem* curr 
					= menu->FindItem( textBody->SuggestedCharset().String());
				if (curr)
					curr->SetMarked( true);
				theMenu->AddItem( menu);
				theMenu->AddSeparatorItem();
			}
		}
	}
	TheResources->AddFontSubmenuTo( theMenu, this, &mFont);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
	theMenu->SetAsyncAutoDestruct( true);
  	theMenu->Go( point, true, false, openRect, true);
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
	:	inherited( NULL, target, resizingMode, flags, true, true, false, 
					  B_FANCY_BORDER)
{
	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
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
		mBusyView = new BmBusyView( BRect( hsLT.x, hsLT.y, 
													  hsLT.x+bvSize, hsLT.y+bvSize));
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
		BPoint lb( bounds.right-B_V_SCROLL_BAR_WIDTH, bounds.bottom);
		BPoint lt( bounds.right-B_V_SCROLL_BAR_WIDTH, 
					  bounds.bottom-B_H_SCROLL_BAR_HEIGHT);
		BPoint rt( bounds.right, bounds.bottom-B_H_SCROLL_BAR_HEIGHT);
/*
		if (IsFocus() || m_target->IsFocus()) {
			SetHighColor( keyboard_navigation_color());
			StrokeLine( bounds.LeftBottom(), lb);
			StrokeLine( bounds.RightTop(), rt);
			StrokeLine( bounds.LeftTop(), bounds.RightTop());
			StrokeLine( bounds.LeftTop(), bounds.LeftBottom());
			StrokeLine( lb, lt);
			StrokeLine( lt, rt);
		} else {
			SetHighColor( ui_color( B_UI_SHINE_COLOR));
			StrokeLine( bounds.LeftBottom(), lb);
			StrokeLine( bounds.RightTop(), rt);
			StrokeLine( lb, lt);
			StrokeLine( lt, rt);
			SetHighColor( tint_color( 
				ui_color( B_UI_PANEL_BACKGROUND_COLOR), B_DARKEN_1_TINT
			));
			StrokeLine( bounds.LeftTop(), bounds.RightTop());
			StrokeLine( bounds.LeftTop(), bounds.LeftBottom());
		}
*/
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::FrameResized(float new_width, float new_height) {
	if (m_target)
		m_target->ResizeTo( new_width-B_V_SCROLL_BAR_WIDTH-4,
								  new_height-B_H_SCROLL_BAR_HEIGHT-4);
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
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::RedrawScrollbars() {
	BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
	if (hScroller)
		hScroller->Invalidate();
	BScrollBar* vScroller = ScrollBar( B_VERTICAL);
	if (vScroller)
		vScroller->Invalidate();
}

