/*
	BmApp.h
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


#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>
#include <Deskbar.h>
#include <PrintJob.h>
#include <Rect.h>
#include "BmString.h"

class BDeskbar;
class BLocker;
class BView;
class BmWindow;

extern const char* BM_APP_SIG;
extern const char* BM_TEST_APP_SIG;

enum {
	BMM_SET_BUSY					= 'bMxa',
	BMM_UNSET_BUSY					= 'bMxb'
};

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();

	// native methods:
	bool HandlesMimetype( const BmString mimetype);
	BRect ScreenFrame();
	void SetNewWorkspace( uint32 newWorkspace);
	uint32 CurrWorkspace();
	void LaunchURL( const BmString url);
	void ForwardMails( BMessage* msg, bool join);
	void ReplyToMails( BMessage* msg, bool join, bool joinIntoOne);
	void PageSetup();
	void PrintMails( BMessage* msg);

	void InstallDeskbarItem();
	void RemoveDeskbarItem();

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();
	void AboutRequested();
	void ReadyToRun();
	void ArgvReceived( int32 argc, char** argv);
	void RefsReceived( BMessage* msg);
	void AppActivated( bool active);
	thread_id Run();

	// getters
	inline bool IsQuitting()				{ return mIsQuitting; }
	inline BLocker* StartupLocker()		{ return mStartupLocker; }
	inline const BmString& AppPath()		{ return mAppPath; }

	BmString BmAppVersion;
	BmString BmAppName;
	BmString BmAppNameWithVersion;

	// message-fields:
	static const char* const MSG_MAILREF;
	static const char* const MSG_STATUS;
	static const char* const MSG_WHO_TO;
	static const char* const MSG_OPT_FIELD;
	static const char* const MSG_OPT_VALUE;
	static const char* const MSG_SUBJECT;
	static const char* const MSG_SELECTED_TEXT;
	static const char* const MSG_SENDING_REFVIEW;

private:
	status_t mInitCheck;
	BmWindow* mMailWin;
	bool mIsQuitting;

	BDeskbar mDeskbar;

	BMessage* mPrintSetup;
	BPrintJob mPrintJob;

	BLocker* mStartupLocker;
	
	bool mDeskbarItemIsOurs;

	BmString mAppPath;
	
	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
