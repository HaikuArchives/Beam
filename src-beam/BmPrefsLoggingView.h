/*
	BmPrefsLoggingView.h
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


#ifndef _BmPrefsLoggingView_h
#define _BmPrefsLoggingView_h

#include "BmPrefsView.h"

#define BM_LOG_POP_SELECTED 				'bmPO'
#define BM_LOG_JOBWIN_SELECTED 			'bmJW'
#define BM_LOG_MAILPARSE_SELECTED 		'bmMP'
#define BM_LOG_APP_SELECTED 				'bmAP'
#define BM_LOG_MAILTRACKING_SELECTED	'bmMT'
#define BM_LOG_GUI_SELECTED 				'bmGU'
#define BM_LOG_MODELCONTROLLER_SELECTED 'bmMC'
#define BM_LOG_SMTP_SELECTED 				'bmSM'
#define BM_LOG_FILTER_SELECTED 			'bmFI'
#define BM_LOG_REFCOUNT_SELECTED 		'bmRC'

class BmTextControl;
class BmMenuControl;
/*------------------------------------------------------------------------------*\
	BmPrefsLoggingView
		-	
\*------------------------------------------------------------------------------*/
class BmPrefsLoggingView : public BmPrefsView {
	typedef BmPrefsView inherited;

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
