/*
	BmMainWindow.cpp
		$Id$
*/

#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

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
	BView* v1 = CreateMailFolderView( BRect( 0, 0, 120, 300));
	BView* v2 = CreateMailRefView( BRect( 150, 0, 400, 300));
	AddChild( v1);
	AddChild( v2);
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

CLVContainerView* BmMainWindow::CreateMailFolderView( BRect rect) {
	mMailFolderView = BmMailFolderView::CreateInstance( rect);
	bmApp->MailFolderView = mMailFolderView;
	return mMailFolderView->ContainerView();
}

CLVContainerView* BmMainWindow::CreateMailRefView( BRect rect) {
	mMailRefView = BmMailRefView::CreateInstance( rect);
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
