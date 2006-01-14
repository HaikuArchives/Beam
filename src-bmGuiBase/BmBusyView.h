/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmBusyView_h
#define _BmBusyView_h

#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>

#include "BmGuiBase.h"
#include "BmString.h"

class BmBitmapHandle;

class IMPEXPBMGUIBASE BmBusyView : public BView
{
	typedef BView inherited;

public:
	// creator-func, c'tors and d'tor:
	BmBusyView( BPoint leftTop);
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
