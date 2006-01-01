/*
	BmMailHeaderView.cpp
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


#include <Application.h>
#include <Clipboard.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <StringView.h>

#include "Colors.h"

#include "regexx.hh"
#include "split.hh"
using namespace regexx;


#include "BeamApp.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmMailHeaderView.h"
#include "BmMailView.h"
#include "BmMsgTypes.h"
#include "BmMultiLineStringView.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmResources.h"


static const float addrMenuWidth = 7;

/*------------------------------------------------------------------------------*\
	BmMailHeaderFieldView
		-	
\*------------------------------------------------------------------------------*/
class BmMailHeaderView::BmMailHeaderFieldView : public BView 
{
	typedef BView inherited;
	
	class BmAddrMenuView : public BView 
	{
		typedef BView inherited;
	public:
		BmAddrMenuView(BRect frame, const BmAddressList* addrList);
		virtual ~BmAddrMenuView();
		virtual void Draw(BRect updateRect);
		virtual void MouseDown( BPoint point);
		virtual void ShowMenu( BPoint point);
	private:
		const BmAddressList* mAddrList;
		float mTriangleYPos;
	};

	class BmMsgFilter : public BMessageFilter {
	public:
		BmMsgFilter( BHandler* destHandler, uint32 cmd)
			: 	BMessageFilter( B_ANY_DELIVERY, B_ANY_SOURCE, cmd) 
			,	mDestHandler( destHandler)
		{
		}
		filter_result Filter( BMessage* msg, BHandler** handler);
	private:
		BHandler* mDestHandler;
	};

public:
	// c'tors and d'tor:
	BmMailHeaderFieldView( const BmString& title, const BmAddressList* addrList,
								  BFont* font, float fixedWidth, float titleWidth=-1);
	BmMailHeaderFieldView( const BmString& title, const BmString& value, 
								  BFont* font, float fixedWidth, float titleWidth=-1);
	~BmMailHeaderFieldView();

	// native methods:
	void SetTitleWidth( float newWidth, float fixedWidth);
	float GetTitleWidth();
	void ShowMenu( BPoint point);

	// overrides of BView base:
	void MouseDown(BPoint point);

private:
	void SetTo( const BmString& title, const BmString& value, 
					const BmAddressList* addrList, BFont* font, float fixedWidth, 
					float titleWidth);
	BStringView* mTitleView;
	BmAddrMenuView* mAddrMenuView;
	BmMultiLineStringView* mContentView;

	// Hide copy-constructor and assignment:
	BmMailHeaderFieldView( const BmMailHeaderFieldView&);
	BmMailHeaderFieldView operator=( const BmMailHeaderFieldView&);
};

// #pragma mark - BmAddrMenuView
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderFieldView::BmAddrMenuView
::BmAddrMenuView(BRect frame, const BmAddressList* addrList)
	:	inherited(frame, "FieldTitleView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mAddrList(addrList)
{
	SetViewColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 2));
	SetLowColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 2));
	SetHighColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 4));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderFieldView::BmAddrMenuView
::~BmAddrMenuView()
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView::BmAddrMenuView
::Draw(BRect updateRect)
{
	inherited::Draw(updateRect);
	if (mAddrList && mAddrList->InitOK()) {
		BRect b = Bounds();
		float x = 2;
		float y = b.Height() / 2.0;
		FillTriangle( BPoint(x-2,y-1), BPoint(x+2,y-1), BPoint(x,y+1));
	}
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView::BmAddrMenuView
::MouseDown( BPoint point)
{
	inherited::MouseDown( point); 
	if (mAddrList && mAddrList->InitOK())
		ShowMenu( point);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView::BmAddrMenuView
::ShowMenu( BPoint point)
{
	BPopUpMenu* menu = new BPopUpMenu("NewPeopleMenu", false, false);
	BFont menuFont( *be_plain_font);
	menu->SetFont( &menuFont);
	menu->SetAsyncAutoDestruct(true);
	
	BMenu* createAndEditPersonMenu = NULL;
	BMenu* createPersonMenu = NULL;
	BMenu* editPersonMenu = NULL;
	BMenuItem* item = NULL;
	uint16 count = 0;
	BmAddrList::const_iterator iter;
	BMessage* allMsg = new BMessage(BMM_CREATE_PERSON_FROM_ADDR);
	for( iter = mAddrList->begin(); iter != mAddrList->end(); ++iter) {
		if (!ThePeopleList->FindPersonByEmail(iter->AddrSpec())) {
			if (!createAndEditPersonMenu) {
				createAndEditPersonMenu = new BMenu("Import & Edit Address");
				createAndEditPersonMenu->SetFont( &menuFont);
			}
	
			if (!createPersonMenu) {
				createPersonMenu = new BMenu("Import Address");
				createPersonMenu->SetFont( &menuFont);
			}
	
			BMessage* msg = new BMessage(BMM_CREATE_PERSON_FROM_ADDR);
			msg->AddString("email", iter->AddrSpec().String());
			msg->AddString("name", iter->Phrase().String());
			allMsg->AddString("email", iter->AddrSpec().String());
			allMsg->AddString("name", iter->Phrase().String());
			item = new BMenuItem( iter->AddrString().String(), msg);
			item->SetTarget( be_app);
			createPersonMenu->AddItem( item);
			BMessage* createAndEditMsg = new BMessage(*msg);
			createAndEditMsg->AddBool("edit", true);
			item = new BMenuItem( iter->AddrString().String(), createAndEditMsg);
			item->SetTarget( be_app);
			createAndEditPersonMenu->AddItem( item);
			count++;
		} else {
			if (!editPersonMenu) {
				editPersonMenu = new BMenu("Edit Address");
				editPersonMenu->SetFont( &menuFont);
			}
			BMessage* editMsg = new BMessage(BMM_EDIT_PERSON_WITH_ADDR);
			editMsg->AddString("email", iter->AddrSpec().String());
			item = new BMenuItem( iter->AddrString().String(), editMsg);
			item->SetTarget( be_app);
			editPersonMenu->AddItem( item);
		}
	}
	if (count > 1) {
		createPersonMenu->AddSeparatorItem();
		BMenuItem* item = new BMenuItem( "<All Addresses>", allMsg);
		item->SetTarget( be_app);
		createPersonMenu->AddItem( item);
		BMessage* allCreateAndEditMsg = new BMessage(*allMsg);
		allCreateAndEditMsg->AddBool("edit", true);
		item = new BMenuItem( "<All Addresses>", allCreateAndEditMsg);
		item->SetTarget( be_app);
		createAndEditPersonMenu->AddItem( item);
	}

	if (createAndEditPersonMenu)
		menu->AddItem(createAndEditPersonMenu);
	if (createPersonMenu)
		menu->AddItem(createPersonMenu);
	if (editPersonMenu)
		menu->AddItem(editPersonMenu);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	menu->Go( point, true, false, openRect, true);
}



// #pragma mark - BmMailHeaderFieldView
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderFieldView
::BmMailHeaderFieldView( const BmString& title, const BmAddressList* addrList, 
								 BFont* font, float fixedWidth, float titleWidth)
	:	inherited( BRect( 0, 0, 0, 0),
					  "MailHeaderFieldView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	BmString value = addrList ? addrList->AddrString() : BM_DEFAULT_STRING;
	SetTo(title, value, addrList, font, fixedWidth, titleWidth);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderFieldView
::BmMailHeaderFieldView( const BmString& title, const BmString& value, 
								 BFont* font, float fixedWidth, float titleWidth)
	:	inherited( BRect( 0, 0, 0, 0),
					  "MailHeaderFieldView", B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetTo(title, value, NULL, font, fixedWidth, titleWidth);
}


/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView
::SetTo( const BmString& title, const BmString& value, 
			const BmAddressList* addrList,
			BFont* font, float fixedWidth, float titleWidth)
{
	SetViewColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 2));
	const float leftOffs = 15;
	const float textInset = 1;
	font->SetFace( B_BOLD_FACE);
	if (titleWidth < 0)
		titleWidth = font->StringWidth( title.String());
	float fhl = ceil(TheResources->FontHeight(font));
	BRect titleRect(leftOffs, textInset, 2+leftOffs+titleWidth, textInset+fhl+1);

	mTitleView = new BStringView( titleRect, NULL, title.String());
	mTitleView->SetFont( font);
	mTitleView->SetAlignment( B_ALIGN_RIGHT);
	AddChild( mTitleView);

	BRect addrMenuRect( titleRect.right+1, 0,
							  titleRect.right+1+addrMenuWidth-1, 10);
	mAddrMenuView = new BmAddrMenuView( addrMenuRect, addrList);
	AddChild( mAddrMenuView);

	BRect contentRect( titleRect.right+1+addrMenuWidth, 0,
							 fixedWidth, 10);
	font->SetFace( B_REGULAR_FACE);
	mContentView = new BmMultiLineStringView( contentRect, "contentView", 
															value.String(),
															B_FOLLOW_NONE, B_WILL_DRAW);
	mContentView->MakeSelectable( true);
	mContentView->SetFont( font);
	mContentView->SetHighColor( ui_color( B_UI_PANEL_TEXT_COLOR));
	mContentView->SetViewColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 1));
	mContentView->SetLowColor( BmWeakenColor( B_UI_PANEL_BACKGROUND_COLOR, 1));
	AddChild( mContentView);
	float neededHeight = mContentView->TextHeight()+2*textInset;
	ResizeTo( fixedWidth, neededHeight);
	mAddrMenuView->ResizeTo( addrMenuWidth, mContentView->LineHeight());
	mContentView->ResizeTo( fixedWidth-contentRect.left, neededHeight);
	mTitleView->AddFilter( new BmMsgFilter( this, B_MOUSE_DOWN));
	mContentView->AddFilter( new BmMsgFilter( this, B_MOUSE_DOWN));
	mContentView->AddFilter( new BmMsgFilter( this, B_KEY_DOWN));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderFieldView
::~BmMailHeaderFieldView()
{
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailHeaderView::BmMailHeaderFieldView::BmMsgFilter
::Filter( BMessage* msg, BHandler** handler) 
{
	if (msg->what == B_MOUSE_DOWN) {
		int32 buttons;
		int32 clicks;
		if (msg->FindInt32( "buttons", &buttons)==B_OK 
		&& msg->FindInt32( "clicks", &clicks)==B_OK) {
			if (buttons==B_SECONDARY_MOUSE_BUTTON) {
				BPoint where;
				if (msg->FindPoint( "where", &where) == B_OK) {
					// since we will be show the context-menu, we need to convert 
					// the local-view's coordinates into the receiving handler's 
					// coordinates:
					BView* view = (BView*)(*handler);
					where += view->Frame().LeftTop();
					msg->ReplacePoint( "where", where);
				}
				*handler = mDestHandler;
			}
		}
		return B_DISPATCH_MESSAGE;
	}
	if (msg->what == B_KEY_DOWN) {
		BmString bytes = msg->FindString( "bytes");
		if (bytes.Length() 
		&& (bytes[0]==B_UP_ARROW || bytes[0]==B_DOWN_ARROW 
			|| bytes[0]==B_RIGHT_ARROW || bytes[0]==B_LEFT_ARROW 
			|| bytes[0]==B_PAGE_UP || bytes[0]==B_PAGE_DOWN
		   || bytes[0]==B_HOME || bytes[0]==B_END )) {
			// dispatch any cursor movement to the mailview:
			BView* view = (BView*)(*handler);
			*handler =  view->Parent()->Parent()->Parent();
	   }
	}
	return B_DISPATCH_MESSAGE;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
float BmMailHeaderView::BmMailHeaderFieldView::GetTitleWidth() 
{
	return mContentView->Frame().left;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView
::SetTitleWidth( float newTitleWidth, float fixedWidth) 
{
	BRect titleRect = mTitleView->Frame();
	mTitleView->MoveTo( newTitleWidth-5-titleRect.Width(), titleRect.top);
	mAddrMenuView->MoveTo( newTitleWidth, 0);
	mContentView->MoveTo( newTitleWidth+addrMenuWidth, 0);
	mContentView->ResizeTo( fixedWidth-newTitleWidth-addrMenuWidth, 
									mContentView->Frame().Height());
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView::MouseDown( BPoint point) 
{
	inherited::MouseDown( point); 
	if (Parent())
		if (Parent()->Parent())
			Parent()->Parent()->MakeFocus( true);
	BMessage* msg = Looper()->CurrentMessage();
	int32 buttons;
	if (msg->FindInt32( "buttons", &buttons)==B_OK 
	&& buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::BmMailHeaderFieldView::ShowMenu( BPoint point) 
{
	BPopUpMenu* theMenu = new BPopUpMenu( "HeaderViewMenu", false, false);
	BFont font( *be_plain_font);
	font.SetSize( 10);

	// we fetch real point of click from message (since we may have modified it
	// in the message-filter):
	BMessage* msg = Looper()->CurrentMessage();
	if (msg)
		msg->FindPoint( "where", &point);

	BmMailHeaderView* headerView = dynamic_cast<BmMailHeaderView*>( Parent());
	if (!headerView)
		return;
	BMenuItem* item = new BMenuItem( 
		"Small Header", new BMessage( BM_HEADERVIEW_SMALL)
	);
	item->SetTarget( headerView);
	item->SetMarked( 
		headerView->DisplayMode() == BmMailHeaderView::SMALL_HEADERS
	);
	theMenu->AddItem( item);
	item = new BMenuItem( "Large Header", new BMessage( BM_HEADERVIEW_LARGE));
	item->SetTarget( headerView);
	item->SetMarked( 
		headerView->DisplayMode() == BmMailHeaderView::LARGE_HEADERS
	);
	theMenu->AddItem( item);
	item = new BMenuItem( 
		"Full Header (raw)", new BMessage( BM_HEADERVIEW_FULL)
	);
	item->SetTarget( headerView);
	item->SetMarked( 
		headerView->DisplayMode() == BmMailHeaderView::FULL_HEADERS
	);
	theMenu->AddItem( item);
	theMenu->AddSeparatorItem();
	item = new BMenuItem( "Show info from Resent-fields if present", 
								 new BMessage( BM_HEADERVIEW_SWITCH_RESENT));
	item->SetTarget( headerView);
	item->SetMarked( headerView->ShowRedirectFields());
	theMenu->AddItem( item);
	theMenu->AddSeparatorItem();
	item = new BMenuItem( "Copy Header to Clipboard", 
								 new BMessage( BM_HEADERVIEW_COPY_HEADER));
	item->SetTarget( headerView);
	theMenu->AddItem( item);
	theMenu->AddSeparatorItem();
	ChildAt(0)->GetFont(&font);
	TheResources->AddFontSubmenuTo( theMenu, headerView, &font);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
	theMenu->SetAsyncAutoDestruct( true);
  	theMenu->Go( point, true, false, openRect, true);
}



// #pragma mark - BmMailHeaderView

const char* const BmMailHeaderView::MSG_VERSION = 		"bm:version";
const char* const BmMailHeaderView::MSG_MODE = 			"bm:mode";
const char* const BmMailHeaderView::MSG_REDIRECT_MODE =	"bm:rmode";
const char* const BmMailHeaderView::MSG_FONTNAME = 	"bm:hfnt";
const char* const BmMailHeaderView::MSG_FONTSIZE =		"bm:hfntsz";

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderView( BmMailHeader* header)
	:	inherited( BRect( 0, 0, 0, 0),
					  "MailHeaderView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMailHeader( NULL)
	,	mDisplayMode( LARGE_HEADERS)
	,	mFont( be_bold_font)
	,	mShowRedirectFields( true)
{
	SetViewColor( B_TRANSPARENT_COLOR);
	ShowHeader( header);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::~BmMailHeaderView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Archive( BMessage* archive, bool) const {
	status_t ret = archive->AddInt16( MSG_MODE, mDisplayMode)
					|| archive->AddBool( MSG_REDIRECT_MODE, mShowRedirectFields);
	if (ret==B_OK) {
		font_family family;
		font_style style;
		mFont.GetFamilyAndStyle( &family, &style);
		BmString fontName = BmString(family) + "," + style;
		ret = archive->AddString( MSG_FONTNAME, fontName.String())
					|| archive->AddInt16( MSG_FONTSIZE, mFont.Size());
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Unarchive( BMessage* archive, bool) {
	int16 version;
	if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
		version = 1;
	status_t ret = archive->FindInt16( MSG_MODE, &mDisplayMode);
	if (ret==B_OK && version >= 2)
		ret = archive->FindBool( MSG_REDIRECT_MODE, &mShowRedirectFields);
	if (ret==B_OK && version >= 3) {
		BmString fontName = archive->FindString( MSG_FONTNAME);
		int16 fontSize = archive->FindInt16( MSG_FONTSIZE);
		int32 pos = fontName.FindFirst( ",");
		if (pos != B_ERROR) {
			BmString family( fontName.String(), pos);
			BmString style( fontName.String()+pos+1);
			mFont.SetFamilyAndStyle( family.String(), style.String());
		} else {
			mFont = *be_fixed_font;
		}
		mFont.SetSize( fontSize);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	ShowHeader()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::ShowHeader( BmMailHeader* header, bool invalidate) {
	bool hasErrors = false;
	if (mMailHeader != header) {
		mMailHeader = header;
		if (header && header->HasParsingErrors())
			hasErrors = true;
	}
	if (header) {
		float height = AddFieldViews();
		BmMailView* mailView = dynamic_cast<BmMailView*>( Parent());
		if (mailView) {
			mailView->ScrollTo( 0,0);
			ResizeTo( FixedWidth(), height);
			mailView->CalculateVerticalOffset();
			if (hasErrors) {
				mailView->AddParsingError(mMailHeader->ParsingErrors());
				BM_LOG( BM_LogMailParse, mMailHeader->ParsingErrors());
			}
		}
	} else {
		RemoveFieldViews();
		ResizeTo(0,0);
	}
	if (invalidate)
		Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::RemoveFieldViews() {
	for( uint32 i=0; i<mFieldViews.size(); ++i) {
		BmMailHeaderFieldView* fv = mFieldViews[i];
		RemoveChild( fv);
		delete fv;
	}
	mFieldViews.clear();
}
	
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
float BmMailHeaderView::AddFieldViews() {
	vector<BmString> titles;
	vector<BmString> fields;
	Regexx rx;

	RemoveFieldViews();

	if (mDisplayMode != FULL_HEADERS) {
		vector<BmString> fieldList;
		BmString fs;
		if (mDisplayMode == SMALL_HEADERS)
			fs = ThePrefs->GetString( "HeaderListSmall");
		else
			fs = ThePrefs->GetString( "HeaderListLarge");
		split( BmPrefs::nListSeparator, fs, fieldList);
		int numShown = fieldList.size();
		for( int i=0; i<numShown; ++i) {
			BmString field = fieldList[i];
			field.CapitalizeEachWord();
			fields.push_back( field);
			int32 pos = field.FindFirst("/");
			if (pos != B_ERROR)
				field.Truncate( pos);
			pos = field.FindFirst("?");
			if (pos != B_ERROR)
				field.Truncate( pos);
			titles.push_back( field<<":");
		}
	}

	float yPos = 0;
	if (mDisplayMode != FULL_HEADERS) {
		float maxTitleWidth = 0;
		mFont.SetFace( B_BOLD_FACE);
		for( uint32 i=0; i<fields.size(); ++i) {
			float w = mFont.StringWidth( titles[i].String());
			maxTitleWidth = MAX( w, maxTitleWidth);
		}
		mFont.SetFace( B_REGULAR_FACE);
		// small or large mode, display fields as defined by prefs:
		for( uint32 l=0; l<fields.size(); ++l) {
			vector<BmString> fieldVals;
			int count = rx.exec( fields[l], "[\\w\\-]+\\??", Regexx::global);
			const BmAddressList* addrList = NULL;
			bool skipEmpty = false;
			for( int i=0; i<count && fieldVals.empty(); ++i) {
				BmString fieldName = rx.match[i];
				if (fieldName[fieldName.Length()-1] == '?') {
					skipEmpty = true;
					fieldName.Truncate(fieldName.Length()-1);
				}
				BmString fieldVal;
				uint32 valCount = mMailHeader->CountFieldVals(fieldName);
				for( uint32 v=0; v<valCount; ++v) {
					BmString fn = fieldName;
					if (mMailHeader->IsRedirect()) {
						if (mShowRedirectFields && fieldName == BM_FIELD_FROM)
							fn = BM_FIELD_RESENT_FROM;
						else if (mShowRedirectFields && fieldName == BM_FIELD_TO)
							fn = BM_FIELD_RESENT_TO;
						else if (mShowRedirectFields && fieldName == BM_FIELD_SENDER)
							fn = BM_FIELD_RESENT_SENDER;
						else if (mShowRedirectFields && fieldName == BM_FIELD_CC)
							fn = BM_FIELD_RESENT_CC;
						else if (mShowRedirectFields && fieldName == BM_FIELD_BCC)
							fn = BM_FIELD_RESENT_BCC;
						else if (mShowRedirectFields && fieldName == BM_FIELD_DATE)
							fn = BM_FIELD_RESENT_DATE;
						else if (mShowRedirectFields && fieldName == BM_FIELD_MESSAGE_ID)
							fn = BM_FIELD_RESENT_MESSAGE_ID;
					}
					fieldVal = mMailHeader->GetFieldVal( fn, v);
					if (fieldName == BM_FIELD_DATE) {
						BmString timeMode 
							= ThePrefs->GetString( "TimeModeInHeaderView", "native");
						if (timeMode.ICompare( "native") != 0) {
							time_t lt = 0;
							ParseDateTime( fieldVal, lt);
							if (timeMode.ICompare( "swatch") == 0)
								fieldVal = TimeToSwatchString( lt, "%Y-%m-%d @");
							else if (timeMode.ICompare( "local") == 0)
								fieldVal = TimeToString( lt, "%Y-%m-%d %H:%M:%S %Z");
						}
					}
					if (fieldVal.Length()) {
						fieldVals.push_back(fieldVal);
						if (mMailHeader->IsAddressField(fn))
							addrList = &mMailHeader->GetAddressList(fn);
					}
				}
			}
			if (fieldVals.empty() && !skipEmpty)
				fieldVals.push_back(BM_DEFAULT_STRING);
			BmMailHeaderFieldView* fv;
			for( uint32 v=0; v<fieldVals.size(); ++v) {
				if (addrList) {
					fv = new BmMailHeaderFieldView( titles[l], addrList, &mFont, 
															  FixedWidth(), maxTitleWidth);
				} else {
					fv = new BmMailHeaderFieldView( titles[l], fieldVals[v], &mFont, 
															  FixedWidth(), maxTitleWidth);
				}
				mFieldViews.push_back( fv);
				fv->MoveTo( 0, yPos);
				AddChild( fv);
				yPos += fv->Frame().Height();
			}
		}
	} else {
		// full mode, display complete header:
		int numLines = rx.exec( mMailHeader->HeaderString(), "\\n", 
										Regexx::newline | Regexx::global);

		const char *start = mMailHeader->HeaderString().String();
		BmString line;
		BmString field;
		BmString content;
		for( int l=0; l<numLines; l++) {
			const char* end = strchr( start, '\n');
			if (!end)
				break;
			if (*start!=' ' && *start!='\t') {
				char* colonPos = strchr( start, ':');
				if (!colonPos)
					break;
				if (field.Length()) {
					// output completed field:
					BmMailHeaderFieldView* fv 
						= new BmMailHeaderFieldView( field, content, &mFont, 
															  FixedWidth());
					mFieldViews.push_back( fv);
					fv->MoveTo( 0, yPos);
					AddChild( fv);
					yPos += fv->Frame().Height();
					content.Truncate(0);
				}
				field.SetTo( start, colonPos-start+1);
				start = colonPos+1;
			}
			for( ; start<end-1 && (*start==' ' || *start=='\t'); ++start)
				;
			if (end<=start) {
				// we make sure to ignore malformed header-lines
				start = end+1;
				continue;
			}
			line.SetTo( start, end-start-1);
			start = end+1;
			BmString utf8Line;
			ConvertToUTF8( BmEncoding::DefaultCharset, line, utf8Line);
			if (content.Length())
				content << "\n" << utf8Line;
			else
				content = utf8Line;
		}
		if (field.Length()) {
			// output last field:
			BmMailHeaderFieldView* fv 
				= new BmMailHeaderFieldView( field, content, &mFont, FixedWidth());
			mFieldViews.push_back( fv);
			fv->MoveTo( 0, yPos);
			AddChild( fv);
			yPos += fv->Frame().Height();
		}
		float maxTitleWidth = 0;
		for( uint32 i=0; i<mFieldViews.size(); ++i) {
			float w = mFieldViews[i]->GetTitleWidth();
			maxTitleWidth = MAX( w, maxTitleWidth);
		}
		for( uint32 i=0; i<mFieldViews.size(); ++i)
			mFieldViews[i]->SetTitleWidth( maxTitleWidth, FixedWidth());
	}
	return yPos;
}

/*------------------------------------------------------------------------------*\
	UISettingsChanged()
		-	for Zeta only, react on color changes
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::UISettingsChanged(const BMessage* changes, 
															uint32 flags) {
	ShowHeader( mMailHeader.Get());
	return B_OK;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BMM_SWITCH_HEADER: {
				mDisplayMode++;
				if (mDisplayMode > FULL_HEADERS)
					mDisplayMode = SMALL_HEADERS;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_SMALL: {
				mDisplayMode = SMALL_HEADERS;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_LARGE: {
				mDisplayMode = LARGE_HEADERS;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_FULL: {
				mDisplayMode = FULL_HEADERS;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_SWITCH_RESENT: {
				mShowRedirectFields = !mShowRedirectFields;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_COPY_HEADER: {
				BmString hdrStr;
				hdrStr.ConvertLinebreaksToLF( &mMailHeader->HeaderString());
				BMessage* clipMsg;
				if (be_clipboard->Lock()) {
					be_clipboard->Clear();
					if ((clipMsg = be_clipboard->Data())!=NULL) {
						clipMsg->AddData( "text/plain", B_MIME_TYPE, hdrStr.String(), 
												hdrStr.Length());
						be_clipboard->Commit();
					}
					be_clipboard->Unlock();
				}
				break;
			}
			case BM_FONT_SELECTED: {
				BmString family = msg->FindString( BmResources::BM_MSG_FONT_FAMILY);
				BmString style = msg->FindString( BmResources::BM_MSG_FONT_STYLE);
				mFont.SetFamilyAndStyle( family.String(), style.String());
				ShowHeader( mMailHeader.Get());
				BmMailView* mailView = dynamic_cast< BmMailView*>( Parent());
				if (mailView)
					mailView->WriteStateInfo();
				break;
			}
			case BM_FONTSIZE_SELECTED: {
				int16 fontSize = msg->FindInt16( BmResources::BM_MSG_FONT_SIZE);
				mFont.SetSize( fontSize);
				ShowHeader( mMailHeader.Get());
				BmMailView* mailView = dynamic_cast< BmMailView*>( Parent());
				if (mailView)
					mailView->WriteStateInfo();
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailHeaderView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::Draw( BRect bounds) {
	inherited::Draw( bounds);
	// ugly hack, force ScrollView to redraw its scrollbar:
	BmMailView* mv = dynamic_cast<BmMailView*>( Parent());
	if (mv)
		mv->ContainerView()->RedrawScrollbars();
}

