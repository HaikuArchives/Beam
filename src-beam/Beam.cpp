/*
	Beam.cpp
		$Id$
*/

#include "Beam.h"
#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

const char* BmAppVersion = "0.x (development version)";
const char* BmAppName = "Beam";

BeamApp* beamApp = NULL;

BeamApp::BeamApp()
	:	inherited( "application/x-vnd.zooey-Beam")
	,	mInitCheck( B_NO_INIT)
{
	try {
		beamApp = this;
		TheMainWindow = BmMainWindow::CreateInstance();
		mInitCheck = B_OK;
	}
	catch( exception &e) {
		ShowAlert( e.what());
		exit(10);
	}
}

BeamApp::~BeamApp()
{
}

void BeamApp::ReadyToRun()
{
	TheMainWindow->Show();
}

void BeamApp::MessageReceived(BMessage* msg) {
	try {
		switch( msg->what) {
			case B_SILENT_RELAUNCH: {
				BM_LOG2( BM_LogAll, "App: silently relaunched");
				if (TheMainWindow->IsMinimized())
					TheMainWindow->Minimize( false);
				inherited::MessageReceived( msg);
				break;
			}
			case B_QUIT_REQUESTED: 
				BM_LOG2( BM_LogAll, "App: quit requested");
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("BeamApp: ") << err.what());
	}
}

thread_id BeamApp::Run() {
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		TheMainWindow->BeginLife();
		tid = inherited::Run();
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		exit(10);
	}
	return tid;
}

int main()
{
	BeamApp* app = new BeamApp;
	app->Run();
	delete app;
}

