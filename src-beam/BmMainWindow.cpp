/*
	BmMainWindow.cpp
		$Id$
*/

#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "UserResizeSplitView.h"

#include "Beam.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmMainWindow.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	flag and access-function that indicate a user's request-to-stop:
\*------------------------------------------------------------------------------*/
bool BmMainWindow::nIsAlive = false;
bool BmMainWindow::IsAlive() {
	return BmMainWindow::nIsAlive;
}


BmMainWindow::BmMainWindow()
	:	inherited( BRect(50,50,800,600), "Beam 0.x", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mMailFolderView( NULL)
	,	mMailRefView( NULL)
{
	UserResizeSplitView* spl;

	MView* mOuterGroup = 
		new VGroup(
			minimax( 600, 200, 1E5, 1E5),
			new HGroup(
				minimax( -1, -1, 1E5, -1),
				new MButton( "Lass gut sein...", new BMessage(B_QUIT_REQUESTED), be_app, minimax(-1,-1,-1,-1)),
				new Space(),
				0
			),
			new HGroup(
/*
				CreateMailFolderView( minimax(0,100,300,1E5), 120, 400),
				CreateMailRefView( minimax(200,100,1E5,1E5), 400, 400),
*/
				spl = new UserResizeSplitView( 
					CreateMailFolderView( minimax(0,100,300,1E5), 120, 100),
					CreateMailRefView( minimax(200,100,1E5,1E5), 400, 100),
					BRect(0, 0, 800, 500), 
					"splitter1", 140, B_VERTICAL, true, true, false, B_FOLLOW_NONE
				),
				0
			),
			0
		);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

BmMainWindow::~BmMainWindow() {
}

void BmMainWindow::BeginLife() {
	nIsAlive = true;
	try {
		bmApp->MailFolderList = new BmMailFolderList();
		mMailFolderView->StartJob( bmApp->MailFolderList, true, false);
	} catch(...) {
		nIsAlive = false;
		throw;
	}
}

CLVContainerView* BmMainWindow::CreateMailFolderView( minimax minmax, int32 width, int32 height) {
	mMailFolderView = BmMailFolderView::CreateInstance( minmax, width, height);
	bmApp->MailFolderView = mMailFolderView;
	return mMailFolderView->ContainerView();
}

CLVContainerView* BmMainWindow::CreateMailRefView( minimax minmax, int32 width, int32 height) {
	mMailRefView = BmMailRefView::CreateInstance( minmax, width, height);
	bmApp->MailRefView = mMailRefView;
	return mMailRefView->ContainerView();
}

void BmMainWindow::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_QUIT_REQUESTED:
				beamApp->PostMessage( B_QUIT_REQUESTED);
				break;
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MainWindow: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMainWindow::QuitRequested() {
	BM_LOG3( BM_LogMainWindow, BString("MainWindow has been asked to quit"));
	beamApp->PostMessage( B_QUIT_REQUESTED);
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMainWindow::Quit() {
	BM_LOG3( BM_LogMainWindow, BString("MainWindow has quit"));
	inherited::Quit();
}
