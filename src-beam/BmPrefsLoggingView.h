/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmPrefsLoggingView_h
#define _BmPrefsLoggingView_h

#include "BmPrefsView.h"

class BmTextControl;
class BmMenuControl;
/*------------------------------------------------------------------------------*\
	BmPrefsLoggingView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsLoggingView : public BmPrefsView {
	typedef BmPrefsView inherited;

	enum {
		BM_LOG_POP_SELECTED 				= 'bmPO',
		BM_LOG_JOBWIN_SELECTED 			= 'bmJW',
		BM_LOG_MAILPARSE_SELECTED 		= 'bmMP',
		BM_LOG_APP_SELECTED 				= 'bmAP',
		BM_LOG_MAILTRACKING_SELECTED	= 'bmMT',
		BM_LOG_GUI_SELECTED 				= 'bmGU',
		BM_LOG_MODELCONTROLLER_SELECTED = 'bmMC',
		BM_LOG_SMTP_SELECTED 			= 'bmSM',
		BM_LOG_FILTER_SELECTED 			= 'bmFI',
		BM_LOG_REFCOUNT_SELECTED 		= 'bmRC'
	};
	
public:
	// c'tors and d'tor:
	BmPrefsLoggingView();
	virtual ~BmPrefsLoggingView();
	
	// overrides of BmPrefsView base:
	void Initialize();
	void Update();
	void SaveData();
	void UndoChanges();

	// overrides of BView base:
	void MessageReceived( BMessage* msg);

private:

	BmTextControl* mMinLogfileSizeControl;
	BmTextControl* mMaxLogfileSizeControl;

	BmMenuControl* mLogPopControl;
	BmMenuControl* mLogJobWinControl;
	BmMenuControl* mLogMailParseControl;
	BmMenuControl* mLogAppControl;
	BmMenuControl* mLogMailTrackingControl;
	BmMenuControl* mLogGuiControl;
	BmMenuControl* mLogModelControllerControl;
	BmMenuControl* mLogSmtpControl;
	BmMenuControl* mLogFilterControl;
	BmMenuControl* mLogRefCountControl;

	// Hide copy-constructor and assignment:
	BmPrefsLoggingView( const BmPrefsLoggingView&);
	BmPrefsLoggingView operator=( const BmPrefsLoggingView&);
};

#endif
