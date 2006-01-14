/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BeamApp_h
#define _BeamApp_h

#include <Deskbar.h>
#include <PrintJob.h>
#include <Rect.h>
#include "BmString.h"

#include "BmApp.h"

class BDeskbar;
class BLocker;
class BView;
class BmWindow;

extern const char* BM_APP_SIG;

enum {
	BMM_SET_BUSY						= 'bMxa',
	BMM_UNSET_BUSY						= 'bMxb',
	BMM_CREATE_PERSON_FROM_ADDR	= 'bMxc',
	BMM_EDIT_PERSON_WITH_ADDR		= 'bMxd'
};

class BeamApplication : public BmApplication
{
	typedef BmApplication inherited;

	friend int32 PrintMails( void* data);

public:
	//
	BeamApplication( const char *sig);
	~BeamApplication();

	// native methods:
	BRect ScreenFrame();
	void SetNewWorkspace( uint32 newWorkspace);
	uint32 CurrWorkspace();
	//
	bool HandlesMimetype( const BmString mimetype);
	void LaunchURL( const BmString url);

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();
	void AboutRequested();
	void ReadyToRun();
	void ArgvReceived( int32 argc, char** argv);
	void RefsReceived( BMessage* msg);
	void AppActivated( bool active);
	thread_id Run();

	// message-fields:
	static const char* const MSG_MAILREF_VECT;
	static const char* const MSG_STATUS;
	static const char* const MSG_WHO_TO;
	static const char* const MSG_OPT_FIELD;
	static const char* const MSG_OPT_VALUE;
	static const char* const MSG_SUBJECT;
	static const char* const MSG_SELECTED_TEXT;
	static const char* const MSG_SENDING_REFVIEW;

private:
	void PageSetup();

	void InstallDeskbarItem();
	void RemoveDeskbarItem();

	status_t mInitCheck;
	BmWindow* mMailWin;

	BDeskbar mDeskbar;

	BMessage* mPrintSetup;
	BPrintJob mPrintJob;

	bool mDeskbarItemIsOurs;

};

extern BeamApplication* beamApp;

#endif
