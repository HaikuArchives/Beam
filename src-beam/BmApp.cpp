/*
	BmApp.cpp
		$Id$
*/

#include "BmApp.h"
#include "BmBasics.h"
#include "BmDataModel.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

int BmApplication::InstanceCount = 0;

BmApplication* bmApp = NULL;

/*------------------------------------------------------------------------------*\
	BmApplication()
		-	constructor
\*------------------------------------------------------------------------------*/
BmApplication::BmApplication( const char* sig)
	:	inherited( sig)
	,	mIsQuitting( false)
{
	if (InstanceCount > 0)
		throw BM_runtime_error("Trying to initialize more than one instance of class Beam");

	bmApp = this;

	try {
		// create the log-handler:
		BmLogHandler::CreateInstance( 1);

		// load/determine all needed resources:
		BmResources::CreateInstance();

		// load the preferences set by user (if any):
		BmPrefs::CreateInstance();
		
		InstanceCount++;
	} catch (exception& err) {
		ShowAlert( err.what());
		exit( 10);
	}
}

/*------------------------------------------------------------------------------*\
	~BmApplication()
		-	standard destructor
\*------------------------------------------------------------------------------*/
BmApplication::~BmApplication()
{
	delete ThePrefs;
	delete TheResources;
	BmDataModel::RefManager.PrintStatistics();
	BmListModelItem::RefManager.PrintStatistics();
	delete TheLogHandler;
	InstanceCount--;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmApplication::QuitRequested() {
	mIsQuitting = true;
	bool shouldQuit = inherited::QuitRequested();
	if (!shouldQuit) {
		mIsQuitting = false;
	}
	return shouldQuit;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( )
		-	
\*------------------------------------------------------------------------------*/
void BmApplication::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_NODE_MONITOR: {
				TheMailFolderList->HandleNodeMonitorMsg( msg);
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

