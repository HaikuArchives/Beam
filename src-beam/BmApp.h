/*
	BmApp.h
		$Id$
*/

#ifndef _BmApp_h
#define _BmApp_h

#include <Application.h>
#include <Rect.h>
#include <String.h>

class BmWindow;

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

	// beos-stuff
	void MessageReceived( BMessage* msg);
	bool QuitRequested();
	void AboutRequested();
	void ReadyToRun();
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

private:
	status_t mInitCheck;
	BmWindow* mMailWin;
	bool mIsQuitting;

	inline status_t InitCheck() 			{ return mInitCheck; }

	static int InstanceCount;

};

extern BmApplication* bmApp;

#endif
