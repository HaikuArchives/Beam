/*
	BmBusyView.h
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


#ifndef _BmBusyView_h
#define _BmBusyView_h

#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>

#include "BmGuiBase.h"

class BmBitmapHandle;

class IMPEXPBMGUIBASE BmBusyView : public BView
{
	typedef BView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmBusyView( BRect frame);
	~BmBusyView();

	// native methods:
	void SetBusy();
	void UnsetBusy();
	void SetErrorText(const BmString& txt);
	void UnsetErrorText();
	void Pulse();

	static void SetErrorIcon( const BmBitmapHandle* icon)
													{ nErrorIcon = icon; }

	// overrides of BStringView base:
	void Draw( BRect bounds);
	void MessageReceived( BMessage* msg);
	
private:
	void UpdateErrorStatus();

	BRect mCachedBounds;
	BMessageRunner* mMsgRunner;
	int16 mBusyCount;
	int32 mCurrState;
	BmString mErrorText;
	static const BmBitmapHandle* nErrorIcon;

	// Hide copy-constructor and assignment:
	BmBusyView( const BmBusyView&);
	BmBusyView operator=( const BmBusyView&);
};


#endif
