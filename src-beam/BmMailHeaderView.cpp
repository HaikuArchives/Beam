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


#include <MenuItem.h>
#include <PopUpMenu.h>

#include "Colors.h"

#include "regexx.hh"
using namespace regexx;

#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmMailHeaderView.h"
#include "BmMailView.h"
#include "BmPrefs.h"
#include "BmResources.h"

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::BmMailHeaderView( BmMailHeader* header)
	:	inherited( BRect( 0, 0, 0, 0),
					  "MailHeaderView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMailHeader( NULL)
	,	mDisplayMode( LARGE_HEADERS)
	,	mFont( new BFont( be_bold_font))
{
	SetViewColor( B_TRANSPARENT_COLOR);
	ShowHeader( header);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeaderView::~BmMailHeaderView() {
	delete mFont;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddInt16( MSG_MODE, mDisplayMode);
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailHeaderView::Unarchive( BMessage* archive, bool deep=true) {
	status_t ret = archive->FindInt16( MSG_MODE, &mDisplayMode);
	return ret;
}

/*------------------------------------------------------------------------------*\
	ShowHeader()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::ShowHeader( BmMailHeader* header, bool invalidate) {
	if (mMailHeader != header)
		mMailHeader = header;
	float height = 0;
	Regexx rx;
	if (header) {
		int numLines = 0;
		switch (mDisplayMode) {
			case SMALL_HEADERS: {
				// small mode, 3 fields are displayed (by default):
				BString fieldList = ThePrefs->GetString( "HeaderListSmall");
				numLines = rx.exec( fieldList, "[\\w\\-/]+", Regexx::global);
				break;
			}
			case FULL_HEADERS: {
				// full mode, ALL fields are displayed:
				numLines = mMailHeader->NumLines();
				break;
			}
			default: {
				// large mode, 5 fields are displayed (by default):
				BString fieldList = ThePrefs->GetString( "HeaderListLarge");
				numLines = rx.exec( fieldList, "[\\w\\-/]+", Regexx::global);
				break;
			}
		};
		height = 6+(TheResources->FontLineHeight( mFont)+3)*numLines;
		ResizeTo( FixedWidth(), height);
		BmMailView* mailView = (BmMailView*)Parent();
		if (mailView)
			mailView->CalculateVerticalOffset();
	}
	if (invalidate)
		Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::Draw( BRect bounds) {
	inherited::Draw( bounds);
	
	BRect r = Bounds();

	if (!mMailHeader) {
		SetHighColor( White);
		FillRect( r);
		return;
	}

	float fh = TheResources->FontHeight( mFont);
	float lh = TheResources->FontLineHeight( mFont)+3;

	vector<BString> titles;
	vector<BString> fields;
	BString fieldList;
	Regexx rx;

	if (mDisplayMode != FULL_HEADERS) {
		if (mDisplayMode == SMALL_HEADERS)
			fieldList = ThePrefs->GetString( "HeaderListSmall");
		else
			fieldList = ThePrefs->GetString( "HeaderListLarge");
		int numShown = rx.exec( fieldList, "[\\w\\-/]+", Regexx::global);
		for( int i=0; i<numShown; ++i) {
			BString field = rx.match[i];
			field.CapitalizeEachWord();
			fields.push_back( field);
			int32 pos = field.FindFirst("/");
			if (pos != B_ERROR)
				field.Truncate( pos);
			titles.push_back( field);
		}
	}

	mFont->SetFace( B_BOLD_FACE);
	SetFont( mFont);
	float titleWidth = 0.0;
	if (mDisplayMode == FULL_HEADERS) {
		// determine widest field name:
		int numLines = mMailHeader->NumLines();
		const char *start = mMailHeader->HeaderString().String();
		for( int l=0; l<numLines; l++) {
			const char* end = strchr( start, '\n');
			if (!end)
				break;
			if (*start!=' ' && *start!='\t') {
				char* colonPos = strchr( start, ':');
				if (!colonPos)
					break;
				BString field( start, colonPos-start);
				field << ":";
				float w = StringWidth( field.String()) + 20;
				if (w > titleWidth)
					titleWidth = w;
			}
			start = end+1;
		}
	} else {
		for( uint32 i=0; i<titles.size(); ++i) {
			float w = StringWidth( titles[i].String()) + 20;
			if (w > titleWidth)
				titleWidth = w;
		}
	}

	SetHighColor( BeListSelectGrey);
	FillRect( BRect(0, 0, titleWidth, r.Height()-1));

	SetHighColor( BeLightShadow);
	FillRect( BRect(0+titleWidth, 0, r.Width(), r.Height()-1));

	SetHighColor( BeShadow);
	StrokeLine( BPoint(0,r.Height()), BPoint( r.Width(), r.Height()));

	float x_off = 5.0;
	float y_off = 2.0;
	if (mDisplayMode != FULL_HEADERS) {
		// small or large mode, display just a small or a medium number of fields:
		SetHighColor( Black);
		SetLowColor( BeListSelectGrey);
		for( uint32 l=0; l<titles.size(); ++l) {
			DrawString( (titles[l]+":").String(), BPoint( titleWidth-StringWidth(titles[l].String())-x_off, y_off+l*lh+fh) );
		}
		mFont->SetFace( B_REGULAR_FACE);
		SetFont( mFont);
		SetLowColor( BeLightShadow);
		for( uint32 l=0; l<fields.size(); ++l) {
			BString fieldVal;
			int count = rx.exec( fields[l], "[\\w\\-]+", Regexx::global);
			for( int i=0; i<count && !fieldVal.Length(); ++i) {
				fieldVal = mMailHeader->GetFieldVal( rx.match[i]);
			}
			DrawString( fieldVal.String(), BPoint( titleWidth+x_off, y_off+l*lh+fh) );
		}
	} else {
		// full mode, display complete header:
		SetHighColor( Black);
		SetLowColor( BeLightShadow);
		int numLines = mMailHeader->NumLines();
		const char *start = mMailHeader->HeaderString().String();
		for( int l=0; l<numLines; l++) {
			const char* end = strchr( start, '\n');
			if (!end)
				break;
			if (*start!=' ' && *start!='\t') {
				char* colonPos = strchr( start, ':');
				if (!colonPos)
					break;
				BString field( start, colonPos-start);
				field << ":";
				mFont->SetFace( B_BOLD_FACE);
				SetFont( mFont);
				SetLowColor( BeListSelectGrey);
				DrawString( field.String(), BPoint( titleWidth-StringWidth(field.String())-x_off, y_off+l*lh+fh) );
				SetLowColor( BeLightShadow);
				start = colonPos+1;
			}
			for( ; start<end-1 && (*start==' ' || *start=='\t'); ++start)
				;
			BString line( start, end-start-1);
			start = end+1;
			mFont->SetFace( B_REGULAR_FACE);
			SetFont( mFont);
			DrawString( line.String(), BPoint( titleWidth+x_off, y_off+l*lh+fh) );
		}
	}
}

/*------------------------------------------------------------------------------*\
	MouseDown( point)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::MouseDown( BPoint point) {
	inherited::MouseDown( point); 
	if (Parent())
		Parent()->MakeFocus( true);
	BPoint mousePos;
	uint32 buttons;
	GetMouse( &mousePos, &buttons);
	if (buttons == B_SECONDARY_MOUSE_BUTTON) {
		ShowMenu( point);
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_HEADERVIEW_SMALL: {
				mDisplayMode = 0;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_LARGE: {
				mDisplayMode = 1;
				ShowHeader( mMailHeader.Get());
				break;
			}
			case BM_HEADERVIEW_FULL: {
				mDisplayMode = 2;
				ShowHeader( mMailHeader.Get());
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailHeaderView:\n\t") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeaderView::ShowMenu( BPoint point) {
	BPopUpMenu* theMenu = new BPopUpMenu( "HeaderViewMenu", false, false);

	BMenuItem* item = new BMenuItem( "Small Header", new BMessage( BM_HEADERVIEW_SMALL));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == SMALL_HEADERS);
	theMenu->AddItem( item);
	item = new BMenuItem( "Large Header", new BMessage( BM_HEADERVIEW_LARGE));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == LARGE_HEADERS);
	theMenu->AddItem( item);
	item = new BMenuItem( "Full Header (raw)", new BMessage( BM_HEADERVIEW_FULL));
	item->SetTarget( this);
	item->SetMarked( mDisplayMode == FULL_HEADERS);
	theMenu->AddItem( item);

   ConvertToScreen(&point);
	BRect openRect;
	openRect.top = point.y - 5;
	openRect.bottom = point.y + 5;
	openRect.left = point.x - 5;
	openRect.right = point.x + 5;
  	theMenu->Go( point, true, false, openRect);
  	delete theMenu;
}

