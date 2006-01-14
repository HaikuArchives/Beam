/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmLogWindow_h
#define _BmLogWindow_h


#include "BmLogHandler.h"
#include "BmWindow.h"

enum {
	BM_LOGENTRY_ADDED = 'bmle'
};

class BmLogWindow : public BmWindow
{
	typedef BmWindow inherited;

public:
	// creator-func, c'tors and d'tor:
	static BmLogWindow* CreateAndStartInstanceFor( const char* logfileName,
																  bool showUponNews=false,
																  bool clingToMainWin=false);
	BmLogWindow( const BRect& frame, const BmString& title, 
					 const char* logfileName, bool showUponNews, 
					 bool clingToMainWin);
	virtual ~BmLogWindow();
	
	// overrides of BWindow-base:
	void MessageReceived( BMessage* msg);
	bool QuitRequested();

private:
	BmString mLogfileName;
	MTextView* mLogView;
	bool mShowUponNews;
	bool mClingToMainWin;

	static int32 nWinCount;

	// Hide copy-constructor and assignment:
	BmLogWindow( const BmLogWindow&);
	BmLogWindow operator=( const BmLogWindow&);
};

#endif
