/*
	BmBusyView.cpp
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


#include <String.h>

#include "Colors.h"

#include "BmBasics.h"
#include "BmBusyView.h"
#include "BmLogHandler.h"
#include "BmUtil.h"

#define BM_PULSE 'bmPL'
static BMessage pulseMsg(BM_PULSE);

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
BmBusyView::BmBusyView( BRect frame)
	:	inherited( frame, "BmBusyView", B_FOLLOW_NONE, B_WILL_DRAW)
	,	mMsgRunner( NULL)
	,	mCachedBounds( Bounds())
	,	mBusyCount( 0)
	,	mCurrState( 0)
{
	SetViewColor( tint_color(BeBackgroundGrey, 1.072F));
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
	mBusyCount++;
	BM_LOG2( BM_LogUtil, BString("BusyView::SetBusy() busyCount:")<<mBusyCount);
	if (!mMsgRunner) {
		BMessenger ourselvesAsTarget( this);
		if (!ourselvesAsTarget.IsValid())
			BM_THROW_RUNTIME( "BusyView(): Could not init Messenger.");
		mMsgRunner = new BMessageRunner( ourselvesAsTarget, &pulseMsg, 100*1000, -1);
		status_t err;
		if ((err = mMsgRunner->InitCheck()) != B_OK)
		 	BM_THROW_RUNTIME( BString("BusyView(): Could not init MessageRunner. Error:") << strerror(err));
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::UnsetBusy() {
	if (mBusyCount > 0) {
		mBusyCount--;
		BM_LOG2( BM_LogUtil, BString("BusyView::UnsetBusy() busyCount:")<<mBusyCount);
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
void BmBusyView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_PULSE: {
				Pulse();
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BusyView: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::Pulse() {
	BM_LOG2( BM_LogUtil, BString("BusyView::Pulse()"));
	if (mBusyCount > 0) {
		mCurrState+=10;
		Invalidate();
	}
}

/*------------------------------------------------------------------------------*\
	( )
		-	
\*------------------------------------------------------------------------------*/
void BmBusyView::Draw( BRect bounds) {
	BRect r = mCachedBounds;
	SetHighColor( tint_color(BeBackgroundGrey, 1.072F));
	FillRect( r);
	SetHighColor( White);
	StrokeRect( BRect(1.0,1.0,r.right,r.bottom));
	SetHighColor( BeShadow);
	StrokeRect( r);
	if (mBusyCount <= 0) {
		return;
	}
	r.InsetBy( 1.0, 1.0);
	r.left++;
	r.top++;
	SetHighColor( White);
	FillEllipse( r);
	r.InsetBy( 1.0, 1.0);
	float start = 0;
	float end = (mCurrState % 360);
	SetHighColor( (mCurrState / 360) % 2 ? MedMetallicBlue : White);
	FillArc( r, end, 359.0);
	SetHighColor( (mCurrState / 360) % 2 ? White : MedMetallicBlue);
	FillArc( r, start, end);
}
