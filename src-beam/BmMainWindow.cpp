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


/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates the app's main window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMainWindow* BmMainWindow::CreateInstance() 
{
	BmMainWindow *win = NULL;
	status_t err;
	BString winFilename;
	BFile winFile;

	// create standard main-window:
	win = new BmMainWindow;
	// try to open state-cache-file...
	winFilename = BString("MainWindow");
	if ((err = winFile.SetTo( bmApp->StateInfoFolder(), winFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, archive file found, we fetch our dimensions from it:
		try {
			BMessage archive;
			(err = archive.Unflatten( &winFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch main-window archive from file\n\t<") << winFilename << ">\n\n Result: " << strerror(err));
			win->Unarchive( &archive);
		} catch (exception &e) {
			BM_SHOWERR( e.what());
		}
	}
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMainWindow::BmMainWindow()
	:	inherited( BRect(50,50,800,600), "Beam 0.x", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mMailFolderView( NULL)
	,	mMailRefView( NULL)
	,	mVertSplitter( NULL)
{
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
				mVertSplitter = new UserResizeSplitView( 
					CreateMailFolderView( minimax(0,100,300,1E5), 120, 100),
					CreateMailRefView( minimax(200,100,1E5,1E5), 400, 100),
					BRect(0, 0, 800, 500), 
					"splitter1", 120, B_VERTICAL, true, true, false, B_FOLLOW_NONE
				),
				0
			),
			0
		);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMainWindow::~BmMainWindow() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddRect( MSG_FRAME, Frame())
						|| archive->AddFloat( MSG_VSPLITTER, mVertSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::Unarchive( BMessage* archive, bool deep=true) {
	BRect frame;
	float vDividerPos;
	status_t ret = archive->FindRect( MSG_FRAME, &frame)
						|| archive->FindFloat( MSG_VSPLITTER, &vDividerPos);
	if (ret == B_OK) {
		MoveTo( frame.LeftTop());
		ResizeTo( frame.Width(), frame.Height());
		mVertSplitter->SetDividerLeftOrTop( vDividerPos);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMainWindow::CreateMailFolderView( minimax minmax, int32 width, int32 height) {
	mMailFolderView = BmMailFolderView::CreateInstance( minmax, width, height);
	bmApp->MailFolderView = mMailFolderView;
	return mMailFolderView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMainWindow::CreateMailRefView( minimax minmax, int32 width, int32 height) {
	mMailRefView = BmMailRefView::CreateInstance( minmax, width, height);
	bmApp->MailRefView = mMailRefView;
	return mMailRefView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
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
	Store();
	beamApp->PostMessage( B_QUIT_REQUESTED);
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMainWindow::Quit() {
	mMailRefView->DetachModel();
	mMailFolderView->DetachModel();
	BM_LOG3( BM_LogMainWindow, BString("MainWindow has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores MainWindow-state inside StateCache-folder:
\*------------------------------------------------------------------------------*/
bool BmMainWindow::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BString filename = BString( "MainWindow");
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive MainWindow-object");
		(err = cacheFile.SetTo( bmApp->StateInfoFolder(), filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store state-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}
