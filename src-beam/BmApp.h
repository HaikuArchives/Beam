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
#include <Rect.h>
#include <String.h>

class BDeskbar;
class BView;
class BmWindow;

extern const char* BM_APP_SIG;

#define BMM_SHOW_NEWMAIL_ICON			'bMxa'
#define BMM_HIDE_NEWMAIL_ICON			'bMxb'

class BmApplication : public BApplication
{
	typedef BApplication inherited;

public:
	//
	BmApplication( const char *sig);
	~BmApplication();

	// native methods:
	bool HandlesMimetype( const BString mimetype);
	BRect ScreenFrame();
	void SetNewWorkspace( uint32 newWorkspace);
	void LaunchURL( const BString url);

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();
	void AboutRequested();
	void ReadyToRun();
	void ArgvReceived( int32 argc, char** argv);
	void RefsReceived( BMessage* msg);
	thread_id Run();


	// getters
	inline bool IsQuitting()				{ return mIsQuitting; }

	BString BmAppVersion;
	BString BmAppName;
	BString BmAppNameWithVersion;

	// message-fields:
	static const char* const MSG_MAILREF = "bm:mref";
	static const char* const MSG_STATUS = 	"bm:status";
	static const char* const MSG_WHO_TO = 	"bm:to";
	static const char* const MSG_SELECTED_TEXT = 	"bm:seltext";

	status_t mInitCheck;
	BmWindow* mMailWin;
	bool mIsQuitting;

	BDeskbar mDeskbar;
	BView* mDeskbarView;

	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
