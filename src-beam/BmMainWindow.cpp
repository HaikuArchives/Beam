/*
	BmMainWindow.cpp
		$Id$
*/

#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "CLVColumn.h"

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
	,	mMailFolderList( NULL)
{
	MView* mOuterGroup = 
		new VGroup(
			new HGroup(
				minimax( -1, -1, 1E5, -1),
				new MButton( "Lass gut sein...", new BMessage(B_QUIT_REQUESTED), be_app, minimax(-1,-1,-1,-1)),
				new Space(),
				0
			),
			new HGroup(
				MVPTR(CreateMailFolderView( minimax( 0, 300, 1E5, 1E5, 0.3), 120, 300)),
				new MSplitter(),
				MVPTR(CreateMailRefView( minimax( 400, 300, 1E5, 1E5, 1.0), 400, 300)),
				0
			),
			0
		);
	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

BmMainWindow::~BmMainWindow() {
	delete mMailFolderList;
}

void BmMainWindow::BeginLife() {
	nIsAlive = true;
	try {
		mMailFolderList = BmMailFolderList::CreateInstance( new BmFolderListInfo( this, &IsAlive));
	} catch(...) {
		nIsAlive = false;
		throw;
	}
}

CLVContainerView* BmMainWindow::CreateMailFolderView( minimax minmax, int32 width, 
																		int32 height) {
	mMailFolderView = BmMailFolderView::CreateInstance( minmax, width, height);
	return mMailFolderView->ContainerView();
}

CLVContainerView* BmMainWindow::CreateMailRefView( minimax minmax, int32 width, 
																	int32 height) {
	mMailRefView = BmMailRefView::CreateInstance( minmax, width, height);
	return mMailRefView->ContainerView();
}

void BmMainWindow::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_FOLDER_ADD:
				mMailFolderView->AddFolder( msg);
				break;
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
