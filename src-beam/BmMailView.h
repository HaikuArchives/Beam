/*
	BmMailView.h
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


#ifndef _BmMailView_h
#define _BmMailView_h

#include <layout.h>

#include "BetterScrollView.h"
#include "Colors.h"
#include "WrappingTextView.h"

#include "BmController.h"
#include "BmMail.h"
#include "BmMemIO.h"

class BMessageRunner;
class BmBodyPart;
class BmBodyPartView;
class BmMailRef;
class BmMailViewContainer;
class BmMailHeaderView;
class BmMailRefView;
class BmRulerView;

/*------------------------------------------------------------------------------*\
	types of messages sent via the observe/notify system:
\*------------------------------------------------------------------------------*/
enum {
	BM_NTFY_MAIL_VIEW							= 'bmbc',
						// sent from BmMailView to observers whenever 
						// selection changes
	BM_MAILVIEW_SHOWRAW						= 'bmMa',
	BM_MAILVIEW_SHOWCOOKED					= 'bmMb',
	BM_MAILVIEW_SHOWINLINES_SEPARATELY	= 'bmMc',
	BM_MAILVIEW_SHOWINLINES_CONCATENATED= 'bmMd',
	BM_MAILVIEW_SELECT_CHARSET				= 'bmMe',
	BM_MAILVIEW_COPY_URL						= 'bmMf',
	BM_MAILVIEW_HIGHLIGHT_SIG				= 'bmMg',
	BM_MAILVIEW_HIGHLIGHT_URL				= 'bmMh'
};

/*------------------------------------------------------------------------------*\
	utility function to wrap lines at word boundary:
\*------------------------------------------------------------------------------*/
void WordWrap( const BmString& in, BmString& out, int32 maxLineLen, 
					BmString nl);

/*------------------------------------------------------------------------------*\
	BmMailView
		-	
\*------------------------------------------------------------------------------*/
class BmMailView : public WrappingTextView, public BmJobController {
	typedef WrappingTextView inherited;
	typedef BmJobController inheritedController;
	struct BmTextRunInfo {
		BmTextRunInfo( rgb_color c= ui_color(B_UI_DOCUMENT_TEXT_COLOR), bool url=false) { 
			color = c; isURL = url;
		}
		rgb_color color;
		bool isURL;
	};
	typedef map<int32,BmTextRunInfo> BmTextRunMap;
	typedef BmTextRunMap::const_iterator BmTextRunIter;
	
	// archival-fieldnames:
	static const char* const MSG_VERSION;
	static const char* const MSG_RAW;
	static const char* const MSG_FONTNAME;
	static const char* const MSG_FONTSIZE;
	static const char* const MSG_HIGHLIGHT;
	//
	static const char* const MSG_MAIL;

	static const int16 nArchiveVersion;
	
	static const int16 HIGHLIGHT_SIG = 1<<0;
	static const int16 HIGHLIGHT_URL = 1<<1;

public:
	static const char* const MSG_HAS_MAIL;

	// creator-func, c'tors and d'tor:
	static BmMailView* CreateInstance( BRect frame, bool outbound);
	~BmMailView();

	// native methods:
	void ShowMail( BmMailRef* ref, bool async=true);
	void ShowMail( BmMail* mail, bool async=true);
	void DisplayBodyPart( BmStringOBuf& displayBuf, BmBodyPart* bodyPart);
	void UpdateParsingStatus();
	status_t Archive( BMessage* archive, bool deep=true) const;
	status_t Unarchive( BMessage* archive, bool deep=true);
	bool WriteStateInfo();
	void GetWrappedText( BmString& out, bool hardWrapIfNeeded=true);
	void SetSignatureByName( const BmString sigName);
	void UpdateFont( const BFont& font);
	bool IsOverURL( BPoint point);
	BmString GetTextForTextrun( BmTextRunIter run);
	void SendNoticesIfNeeded( bool haveMail);
	bool IsDisplayComplete();
	//
	void StartIncrementalSearch();

	// overrides of BTextView base:
	bool AcceptsDrop( const BMessage* msg);
	void AttachedToWindow();
	bool CanEndLine(int32 offset);
	void FrameResized( float newWidth, float newHeight);
	void InsertText(const char *text, int32 length, int32 offset,
						 const text_run_array *runs);
	void KeyDown(const char *bytes, int32 numBytes);
	void MakeFocus(bool focused);
	void MessageReceived( BMessage* msg);
	void MouseDown( BPoint point);
	void MouseUp( BPoint point);
	void MouseMoved( BPoint point, uint32 transit, const BMessage *message);
	
	// overrides of BmController base:
	BHandler* GetControllerHandler()		{ return this; }
	void DetachModel();

	// getters:
	inline BmMailViewContainer* ContainerView() const	
													{ return mScrollView; }
	inline BmRef<BmMail> CurrMail()		{ return mCurrMail; }
	inline bool ShowRaw()					{ return mShowRaw; }
	inline bool ShowInlinesSeparately()	{ return mShowInlinesSeparately; }
	inline BmBodyPartView* BodyPartView()
													{ return mBodyPartView; }
	inline BmMailHeaderView* HeaderView()
													{ return mHeaderView; }

	// setters:
	inline void TeamUpWith( BmMailRefView* v)
													{ mPartnerMailRefView = v; }
	inline void ShowRaw( bool b) 			{ mShowRaw = b; }
	inline void ShowInlinesSeparately( bool b)	
													{ mShowInlinesSeparately = b; }
	void AddParsingError( const BmString& errStr);

protected:
	void HandleIncrementalSearchKeys(const char* bytes, int32 numBytes);
	void IncrementalSearch(const BmString& search, bool next=false);
	void JobIsDone( bool completed);

private:
	void ShowMenu( BPoint point);
	BmTextRunIter TextRunInfoAt( int32 pos) const;

	// will not be archived:
	bool mOutbound;
	BmRef< BmMail> mCurrMail;
	BmMailViewContainer* mScrollView;
	BmMailHeaderView* mHeaderView;
	BmBodyPartView* mBodyPartView;
	BmRulerView* mRulerView;
	BmMailRefView* mPartnerMailRefView;
	BmTextRunMap mTextRunMap;
	BmTextRunIter mClickedTextRun;
	BMessageRunner* mReadRunner;
	bool mShowingUrlCursor;
	bool mHaveMail;
	BmString mParsingErrors;
	int32 mIncrSearchPos;
	
	// will be archived:
	BmString mFontName;
	int16 mFontSize;
	int16 mHighlightFlags;
	BFont mFont;
	bool mShowRaw;
	bool mShowInlinesSeparately;
	bool mDisplayInProgress;

	// Hide copy-constructor and assignment:
	BmMailView( BRect frame, bool outbound);
	BmMailView( const BmMailView&);
	BmMailView operator=( const BmMailView&);
};

class BmBusyView;
class BmCaption;
/*------------------------------------------------------------------------------*\
	BmMailViewContainer
		-	
\*------------------------------------------------------------------------------*/
class BmMailViewContainer : public BetterScrollView {
	typedef BetterScrollView inherited;

public:
	BmMailViewContainer( minimax minmax, BmMailView* target);
	~BmMailViewContainer();

	// native methods:
	void RedrawScrollbars();

private:
	// Hide copy-constructor and assignment:
	BmMailViewContainer( const BmMailViewContainer&);
	BmMailViewContainer operator=( const BmMailViewContainer&);
};

#endif
