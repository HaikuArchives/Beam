/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <cctype>

#include <Clipboard.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <UTF8.h>
#include <Window.h>

#include "regexx.hh"
using namespace regexx;

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
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
#include "BmRosterBase.h"
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
const char* const BmMailView::MSG_HIGHLIGHT ="bm:hil";

const char* const BmMailView::MSG_MAIL =		"bm:mail";

const int16 BmMailView::nArchiveVersion = 4;

const char* const BmMailView::MSG_HAS_MAIL = "bm:hmail";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailView* BmMailView::CreateInstance( BRect frame, 
													 bool outbound) {
	// create standard mail-view:
	BmMailView* instance = new BmMailView( frame, outbound);
	// try to open state-cache-file...
	status_t err;
	BFile archiveFile;
	BmString archiveFilename 
		= BmString("MailView") << (outbound ? "_out" : "_in");
	if ((err = archiveFile.SetTo( BeamRoster->StateInfoFolder(), 
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
BmMailView::BmMailView( BRect frame, bool outbound) 
	:	inherited( frame, "MailView", B_FOLLOW_NONE, B_WILL_DRAW | B_NAVIGABLE)
	,	inheritedController( "MailViewController")
	,	mOutbound( outbound)
	,	mCurrMail( NULL)
	,	mPartnerMailRefView( NULL)
	,	mRulerView( NULL)
	,	mShowRaw( false)
	,	mShowInlinesSeparately( true)
	,	mFontSize( 12)
	,	mHighlightFlags( 0xFFFF)
	,	mReadRunner( NULL)
	,	mShowingUrlCursor( false)
	,	mHaveMail( false)
	,	mIncrSearchPos( 0)
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
						|| archive->AddInt16( MSG_FONTSIZE, mFontSize)
						|| archive->AddInt16( MSG_HIGHLIGHT, mHighlightFlags);
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

	if (version >= 4)
		archive->FindInt16( MSG_HIGHLIGHT, &mHighlightFlags);
	else
		mHighlightFlags = HIGHLIGHT_SIG | HIGHLIGHT_URL;
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
	mScrollView = dynamic_cast<BmMailViewContainer*>(Parent());
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
				mBodyPartView->HandleDrop( msg);
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
				BmString charset = msg->FindString( "charset");
				if (mCurrMail && charset.Length()) {
					mCurrMail->SuggestedCharset(charset);
					mCurrMail->ResyncFromDisk();
				}
				break;
			}
			case BM_MAILVIEW_HIGHLIGHT_URL: {
				if (mCurrMail) {
					mHighlightFlags ^= HIGHLIGHT_URL;
					JobIsDone( true);
				}
				break;
			}
			case BM_MAILVIEW_HIGHLIGHT_SIG: {
				if (mCurrMail) {
					mHighlightFlags ^= HIGHLIGHT_SIG;
					JobIsDone( true);
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
			case BMM_FIND: {
				StartIncrementalSearch();
				break;
			}
			case BMM_FIND_NEXT: {
				if (mScrollView)
					IncrementalSearch( mScrollView->Caption()->Text(), true);
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
				return;
			}
			case B_DELETE: {
				if (mCurrMail && mPartnerMailRefView)
					mPartnerMailRefView->KeyDown( bytes, numBytes);
				else
					inherited::KeyDown( bytes, numBytes);
				return;
			}
		}
	}
	unsigned char c = bytes[0];
	if (!IsEditable() && (c>=32 || c==B_BACKSPACE || c==B_ESCAPE)) {
		// do incremental search:
		HandleIncrementalSearchKeys(bytes, numBytes);
	}
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
		if (buttons == B_PRIMARY_MOUSE_BUTTON 
		|| buttons == B_TERTIARY_MOUSE_BUTTON) {
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
			beamApp->LaunchURL( url);
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
		if (!focused)
			mScrollView->Caption()->SetHighlight(false);
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
	if (mCurrMail && mCurrMail->MailRef() == ref)
		return;
	try {
		StopJob();
		mIncrSearchPos = 0;
		if (!ref) {
			if (DataModel())
				DetachModel();
			mHeaderView->ShowHeader( NULL);
			mBodyPartView->ShowBody( NULL);
			mCurrMail = NULL;
			SendNoticesIfNeeded( false);
			ContainerView()->SetErrorText(BM_DEFAULT_STRING);
			return;
		}
		mCurrMail = BmMail::CreateInstance( ref);
		mDisplayInProgress = true;
		if (async)
			ContainerView()->SetBusy();
		StartJob( mCurrMail.Get(), async);
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
	if (mCurrMail == mail)
		return;
	try {
		StopJob();
		mIncrSearchPos = 0;
		if (!mail || mail->InitCheck() != B_OK) {
			if (DataModel())
				DetachModel();
			mCurrMail = NULL;
			mHeaderView->ShowHeader( NULL);
			mBodyPartView->ShowBody( NULL);
			SendNoticesIfNeeded( false);
			ContainerView()->SetErrorText(BM_DEFAULT_STRING);
			return;
		}
		mCurrMail = mail;
		mDisplayInProgress = true;
		if (async)
			ContainerView()->SetBusy();
		StartJob( mCurrMail.Get(), async);
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddParsingError()
	-	
\*------------------------------------------------------------------------------*/
void BmMailView::AddParsingError( const BmString& errStr)
{
	if (errStr.Length()) {
		if (mParsingErrors.Length())
			mParsingErrors << "\n\n";
		mParsingErrors << errStr;
		ContainerView()->SetErrorText(mParsingErrors);
	}
}

/*------------------------------------------------------------------------------*\
	JobIsDone( completed)
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::JobIsDone( bool completed) {
	if (completed && mCurrMail && mCurrMail->Header()) {
		mParsingErrors.Truncate( 0);
		ContainerView()->SetErrorText(mParsingErrors);
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
				BmAutolockCheckGlobal lock( body->ModelLocker());
				if (!lock.IsLocked())
					BM_THROW_RUNTIME( 
						body->ModelNameNC() << ": Unable to get lock"
					);
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
					if ((mHighlightFlags & HIGHLIGHT_SIG) > 0)
						mTextRunMap[displayBuf.CurrPos()] 
							= BmTextRunInfo(BmWeakenColor(B_UI_DOCUMENT_TEXT_COLOR,2));
					else
						mTextRunMap[displayBuf.CurrPos()] 
							= BmTextRunInfo(ui_color(B_UI_DOCUMENT_TEXT_COLOR));
					// signature within body is already in UTF8-encoding, so we
					// just add it to out display-text:
					displayBuf << "-- \n" << body->Signature();
				}
				displayText.Adopt( displayBuf.TheString());
			}
			if (!mOutbound && (mHighlightFlags & HIGHLIGHT_URL) > 0) {
				// highlight URLs:
				Regexx rx;
				int32 count;
				if ((count = rx.exec( 
					displayText, 
					"(https?://|ftp://|nntp://|file://|mailto:)[^][<>(){}|\",\\s]+", 
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
		mHeaderView->ShowHeader( mCurrMail->Header());
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
	} else {
		BM_LOG2( BM_LogMailParse, BmString("setting empty mail into textview"));
		mHeaderView->ShowHeader( NULL);
		SetText( "");
		ContainerView()->UnsetBusy();
		ContainerView()->SetErrorText(BM_DEFAULT_STRING);
		SendNoticesIfNeeded( false);
	}
out:
	ContainerView()->UnsetBusy();
	mDisplayInProgress = false;
}

/*------------------------------------------------------------------------------*\
	StartIncrementalSearch( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::StartIncrementalSearch()
{
	MakeFocus( true);
	mScrollView->Caption()->SetHighlight( true, "Search:");
}

/*------------------------------------------------------------------------------*\
	HandleIncrementalSearchKeys( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::HandleIncrementalSearchKeys(const char* bytes, 
															int32 numBytes)
{
	if (!mScrollView)
		return;
	BmString text = mScrollView->Caption()->Text();
	if (bytes[0] == B_ESCAPE) {
		text.Truncate(0);
		mScrollView->Caption()->SetHighlight( false);
	} else if (bytes[0] == B_BACKSPACE) {
		if (text.Length()) {
			int32 lastBytePos = text.Length()-1;
			// skip utf8 subsequence chars:
			while(lastBytePos>=0 && (text[lastBytePos] & 0xc0) == 0x80)
				lastBytePos--;
			// skip normal char or utf8-sequence start character
			// (this is done by truncating to lastBytePos):
			if (lastBytePos < 0)
				lastBytePos = 0;
			text.Truncate(lastBytePos);
		}
	} else {
		text.Append(bytes, numBytes);
		mScrollView->Caption()->SetHighlight( true, "Search:");
	}
	mScrollView->SetCaptionText(text.String());
	IncrementalSearch(text, false);
}

/*------------------------------------------------------------------------------*\
	IncrementalSearch()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::IncrementalSearch(const BmString& search, bool next)
{
	if (search.Length()) {
		BmString mailtext = Text();
		int32 newPos = mailtext.IFindFirst(search, mIncrSearchPos + (next?1:0));
		if (newPos < B_OK)
			newPos = mailtext.IFindFirst(search);
		if (newPos >= 0) {
			mIncrSearchPos = newPos;
			Select(mIncrSearchPos, mIncrSearchPos+search.Length());
			ScrollToSelection();
			return;
		}
	}
	Select(0,0);
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
			bodyPart->SuggestCharset(mCurrMail->DefaultCharset());
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
				if (bodyPart->HadErrorDuringConversion())
					AddParsingError(
						BmString("The mailtext contains characters that could not ")
							<< "be converted from the proposed charset\n"
							<< "   " << bodyPart->SuggestedCharset() << "\n"
							<< "into UTF-8.\n"
							<< "Some parts of the mailtext may be missing or may be "
							<< "displayed incorrectly. Please try another charset."
					);
				if (bodyPart->HadParsingErrors())
					AddParsingError(bodyPart->ParsingErrors());
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
	UpdateParsingStatus()
		-	
\*------------------------------------------------------------------------------*/
void BmMailView::UpdateParsingStatus() {
	struct ParsingStatusCollector : public BmListModelItem::Collector {
		ParsingStatusCollector(BmMailView* mv) 
			: mailView( mv) 					{}
		virtual bool operator() (BmListModelItem* listItem) 
		{
			BmBodyPart* bodyPart = dynamic_cast<BmBodyPart*>( listItem);
			if (bodyPart && !bodyPart->IsMultiPart()) {
				if (bodyPart->HadParsingErrors())
					mailView->AddParsingError(bodyPart->ParsingErrors());
			}
			return true;
		}
		BmMailView* mailView;
	};
	ParsingStatusCollector collector(this);
	mParsingErrors.Truncate( 0);
	if (mCurrMail && mCurrMail->Body())
		mCurrMail->Body()->ForEachItem( collector);
	ContainerView()->SetErrorText(mParsingErrors);
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
			BeamRoster->StateInfoFolder(), 
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
				BMenu* menu = new BMenu( "Try Charset");
				menu->SetFont( &font);
				BeamGuiRoster->AddCharsetMenu( menu, this, BM_MAILVIEW_SELECT_CHARSET);
				BmString currCharset = textBody->SuggestedCharset();
				currCharset.ToLower();
				BMenuItem* curr = menu->FindItem( currCharset.String());
				if (curr)
					curr->SetMarked( true);
				theMenu->AddItem( menu);
				theMenu->AddSeparatorItem();
			}
		}
	}
	TheResources->AddFontSubmenuTo( theMenu, this, &mFont);
	if (!mOutbound) {
		theMenu->AddSeparatorItem();
		BMenu* hiMenu = new BMenu( "Colorize");
		hiMenu->SetFont( &font);
		item = new BMenuItem("Links", new BMessage(BM_MAILVIEW_HIGHLIGHT_URL));
		if ((mHighlightFlags & HIGHLIGHT_URL) > 0)
			item->SetMarked( true);
		item->SetTarget( this);
		hiMenu->AddItem( item);
		item = new BMenuItem("Signatures", 
									new BMessage(BM_MAILVIEW_HIGHLIGHT_SIG));
		if ((mHighlightFlags & HIGHLIGHT_SIG) > 0)
			item->SetMarked( true);
		item->SetTarget( this);
		hiMenu->AddItem( item);
		theMenu->AddItem( hiMenu);
	}

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
BmMailViewContainer::BmMailViewContainer( minimax minmax, BmMailView* target)
	:	inherited( minmax, target, 
					  BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_BUSYVIEW
					  | BM_SV_CAPTION, "12345678901234567890")
{
	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	ct_mpm = minmax;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer::~BmMailViewContainer() {
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailViewContainer::RedrawScrollbars() {
/*
	BScrollBar* hScroller = ScrollBar( B_HORIZONTAL);
	if (hScroller)
		hScroller->Invalidate();
	BScrollBar* vScroller = ScrollBar( B_VERTICAL);
	if (vScroller)
		vScroller->Invalidate();
*/
}
