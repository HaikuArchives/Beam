/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <cstring>

#include <ScrollBar.h>

#include "BubbleHelper.h"
#include "Colors.h"

#include "BmBasics.h"
#include "BmBitmapHandle.h"
#include "BmBusyView.h"
#include "BmString.h"

#define BM_PULSE 'bmPL'
static BMessage pulseMsg(BM_PULSE);

const BmBitmapHandle* BmBusyView::nErrorIcon = NULL;

static const float kBvHeight = B_H_SCROLL_BAR_HEIGHT + 1;
static const float kBvWidth = kBvHeight - 1;

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmBusyView::BmBusyView( BPoint leftTop)
	:	inherited( BRect(leftTop.x, leftTop.y, 
							  leftTop.x+kBvWidth-1, leftTop.y+kBvHeight-1), 
					  "BmBusyView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMsgRunner( NULL)
	,	mBusyCount( 0)
	,	mCurrState( 0)
{
	SetViewUIColor( B_UI_PANEL_BACKGROUND_COLOR);
	SetLowUIColor( B_UI_PANEL_BACKGROUND_COLOR);
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmBusyView::~BmBusyView() {
	delete mMsgRunner;
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::SetBusy() {
	if (!mBusyCount)
		UnsetErrorText();
	mBusyCount++;
	if (!mMsgRunner) {
		BMessenger ourselvesAsTarget( this);
		if (!ourselvesAsTarget.IsValid())
			BM_THROW_RUNTIME( "BusyView(): Could not init Messenger.");
		mMsgRunner = new BMessageRunner( ourselvesAsTarget, &pulseMsg, 100*1000, -1);
		status_t err;
		if ((err = mMsgRunner->InitCheck()) != B_OK)
		 	BM_THROW_RUNTIME( BmString("BusyView(): Could not init MessageRunner. Error:") << strerror(err));
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::UnsetBusy() {
	if (mBusyCount > 0) {
		mBusyCount--;
		if (!mBusyCount) {
			delete mMsgRunner;
			mMsgRunner = NULL;
			mCurrState = 0;
			Invalidate();
		}
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::UnsetErrorText() 
{
	mErrorText.Truncate(0);
	UpdateErrorStatus();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::SetErrorText(const BmString& txt) 
{
	mErrorText = txt;
	UpdateErrorStatus();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::UpdateErrorStatus() 
{
	if (mErrorText.Length() > 0)
		TheBubbleHelper->SetHelp( this, mErrorText.String());
	else
		TheBubbleHelper->SetHelp( this, NULL);
	Invalidate();
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case BM_PULSE: {
			Pulse();
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::Pulse() {
	if (mBusyCount > 0) {
		mCurrState+=10;
		Invalidate();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::Draw( BRect) {
	BRect r = Bounds();
	SetHighColor( ui_color( B_UI_SHINE_COLOR));
	StrokeLine( BPoint(0.0,1.0), BPoint(r.right-1,1.0));
	StrokeLine( BPoint(0.0,1.0), r.LeftBottom());
	SetHighColor( BmWeakenColor( B_UI_SHADOW_COLOR, BeShadowMod));
	StrokeLine( r.LeftTop(), r.RightTop());
	StrokeLine( r.RightTop(), r.RightBottom());
	StrokeLine( r.LeftBottom(), r.RightBottom());
	if (mBusyCount <= 0) {
		if (mErrorText.Length() > 0 && nErrorIcon) {
			SetDrawingMode( B_OP_ALPHA);
			SetBlendingMode( B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			DrawBitmap( nErrorIcon->bitmap, BPoint(0,1));
		}
		return;
	}
	r.InsetBy( 1.0, 1.0);
	r.top++;
	SetHighColor( ui_color( B_UI_CONTROL_BACKGROUND_COLOR));
	StrokeEllipse( r);
	r.InsetBy( 1.0, 1.0);
	float start = 0;
	float end = (mCurrState % 360);
	SetHighColor( (mCurrState / 360) % 2 
		? ui_color( B_UI_CONTROL_HIGHLIGHT_COLOR)
		: ui_color( B_UI_CONTROL_BACKGROUND_COLOR)
	);
	FillArc( r, end, 359.0);
	SetHighColor( (mCurrState / 360) % 2 
		? ui_color( B_UI_CONTROL_BACKGROUND_COLOR)
		: ui_color( B_UI_CONTROL_HIGHLIGHT_COLOR)
	);
	FillArc( r, start, end);
}
