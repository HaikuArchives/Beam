/*
	Beam.cpp
		$Id$
*/

#include "Beam.h"
#include "BmConnectionWin.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

BeamApp* beamApp = NULL;

BeamApp::BeamApp()
	:	inherited( "application/x-vnd.zooey-Beam")
	,	mInitCheck( B_NO_INIT)
{
	try {
		beamApp = this;
		MainWindow = new BmMainWindow();
		MainWindow->Show();
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

void BeamApp::MessageReceived(BMessage* msg) {
	switch( msg->what) {
		case B_QUIT_REQUESTED: 
			BM_LOG3( BM_LogAll, "App: quit requested");
		default:
			inherited::MessageReceived( msg);
	}
}

thread_id BeamApp::Run() {
	if (InitCheck() != B_OK) {
		exit(10);
	}
	thread_id tid = 0;
	try {
		MainWindow->BeginLife();
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

