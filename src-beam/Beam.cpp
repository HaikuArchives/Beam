/*
	Beam.cpp
		$Id$
*/

#include "Beam.h"
#include "BmBasics.h"
#include "BmJobStatusWin.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmMailViewWin.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

BeamApp* beamApp = NULL;

BeamApp::BeamApp()
	:	inherited( "application/x-vnd.zooey-Beam")
	,	mInitCheck( B_NO_INIT)
	,	mMailWin( NULL)
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

BeamApp::~BeamApp() {
}

void BeamApp::ReadyToRun() {
	TheMainWindow->Show();
	if (mMailWin) {
		TheMainWindow->SendBehind( mMailWin);
		mMailWin = NULL;
	}
}

void BeamApp::RefsReceived( BMessage* msg) {
	if (!msg)
		return;
	entry_ref eref;
	BEntry entry;
	struct stat st;
	for( int index=0; msg->FindRef( "refs", index, &eref) == B_OK; ++index) {
		if (entry.SetTo( &eref) != B_OK)
			continue;
		if (entry.GetStat( &st) != B_OK)
			continue;
		BmRef<BmMailRef> ref = BmMailRef::CreateInstance( NULL, eref, st);
		if (ref->Status() == BM_MAIL_STATUS_DRAFT
		|| ref->Status() == BM_MAIL_STATUS_PENDING) {
			BmMailEditWin* editWin = BmMailEditWin::CreateInstance();
			if (editWin) {
				editWin->EditMail( ref.Get());
				editWin->Show();
				if (!mMailWin)
					mMailWin = editWin;
			}
		} else {
			BmMailViewWin* viewWin = BmMailViewWin::CreateInstance();
			if (viewWin) {
				viewWin->ShowMail( ref.Get());
				viewWin->Show();
				if (!mMailWin)
					mMailWin = viewWin;
			}
		}
	}
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

