/*
	BmMainWindow.cpp
		$Id$
*/

#include <Autolock.h>
#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "UserResizeSplitView.h"

#include "Beam.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailEditWin.h"
#include "BmMailFolderList.h"
#include "BmMailFolderView.h"
#include "BmMailRefView.h"
#include "BmMailView.h"
#include "BmMainWindow.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmResources.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"


/********************************************************************************\
	BmMainMenuBar
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMainMenuBar()
		-	
\*------------------------------------------------------------------------------*/
BmMainMenuBar::BmMainMenuBar()
	:	inherited()
	,	inheritedController( "PopAccountListController")
	,	mAccountMenu( NULL)
{
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMainMenuBar::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_LISTMODEL_ADD:
			case BM_LISTMODEL_REMOVE: {
				if (!IsMsgFromCurrentModel( msg)) break;
				JobIsDone( true);
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString(ControllerName()) << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMainMenuBar::JobIsDone( bool completed) {
	if (completed && mAccountMenu) {
		BmAutolock lock( DataModel()->ModelLocker());
		lock.IsLocked()	 						|| BM_THROW_RUNTIME( BString() << ControllerName() << ":AddAllModelItems(): Unable to lock model");
		BMenuItem* old;
		while( (old = mAccountMenu->RemoveItem( (int32)0)))
			delete old;
		BmListModel *model = dynamic_cast<BmListModel*>( DataModel());
		BmModelItemMap::const_iterator iter;
		for( iter = model->begin(); iter != model->end(); ++iter) {
			BmListModelItem* item = iter->second.Get();
			BMessage* msg = new BMessage( BMM_CHECK_MAIL);
			msg->AddString( BmPopAccountList::MSG_ITEMKEY, item->Key());
			if (item)
				mAccountMenu->AddItem( new BMenuItem( item->Key().String(), msg));
		}
	}
}



/********************************************************************************\
	BmMainWindow
\********************************************************************************/

BmMainWindow* BmMainWindow::theInstance = NULL;

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

	if (theInstance)
		return theInstance;

	// create standard main-window:
	win = new BmMainWindow;
	// try to open state-cache-file...
	winFilename = BString("MainWindow");
	if ((err = winFile.SetTo( TheResources->StateInfoFolder(), winFilename.String(), B_READ_ONLY)) == B_OK) {
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
	theInstance = win;
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
	TheMailFolderList = BmMailFolderList::CreateInstance();
	ThePopAccountList = BmPopAccountList::CreateInstance();

	MView* mOuterGroup = 
		new VGroup(
			minimax( 600, 200, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mCheckButton = new BmToolbarButton( "Check", 
																	TheResources->IconByName("Button_Check"), 
																	new BMessage(BMM_CHECK_MAIL), this, 
																	"Check for new mail"),
					mNewButton = new BmToolbarButton( "New", 
																 TheResources->IconByName("Button_New"), 
																 new BMessage(BMM_NEW_MAIL), this, 
																 "Compose a new mail message"),
					mReplyButton = new BmToolbarButton( "Reply", 
																	TheResources->IconByName("Button_Reply"), 
																	new BMessage(BMM_REPLY), this, 
																	"Reply to sender only"),
					mReplyAllButton = new BmToolbarButton( "Reply All", 
																		TheResources->IconByName("Button_ReplyAll"), 
																		new BMessage(BMM_REPLY_ALL), this, 
																		"Reply to sender and all recipients"),
					mForwardButton = new BmToolbarButton( "Forward", 
																	  TheResources->IconByName("Button_Forward"), 
																	  new BMessage(BMM_FORWARD), this, 
																	  "Forward mail to somewhere else"),
					mBounceButton = new BmToolbarButton( "Bounce", 
																	 TheResources->IconByName("Button_Bounce"), 
																	 new BMessage(BMM_BOUNCE), this, 
																	 "Redirect message to somewhere else (preserves original sender)"),
					mPrintButton = new BmToolbarButton( "Print", 
																	TheResources->IconByName("Button_Print"), 
																	new BMessage(BMM_PRINT), this, 
																	"Print selected messages(s)"),
					mTrashButton = new BmToolbarButton( "Delete", 
																	TheResources->IconByName("Button_Trash"), 
																	new BMessage(BMM_TRASH), this, 
																	"Move selected messages to Trash"),
					new Space(),
					new BmToolbarButton( "Lass' gut sein", 
												TheResources->IconByName("Person"), 
												new BMessage(B_QUIT_REQUESTED), this),
					new Space(minimax(20,-1,20,-1)),
					0
				)
			),
			new Space(minimax(-1,2,-1,2)),
			new HGroup(
				mVertSplitter = new UserResizeSplitView( 
					CreateMailFolderView( minimax(0,100,300,1E5), 120, 100),
					mHorzSplitter = new UserResizeSplitView( 
						CreateMailRefView( minimax(200,100,1E5,1E5), 400, 200),
						CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
						"hsplitter", 150, B_HORIZONTAL, true, true, false, B_FOLLOW_NONE
					),
					"vsplitter", 120, B_VERTICAL, true, true, false, B_FOLLOW_NONE
				),
				0
			),
			0
		);

	mMailRefView->TeamUpWith( mMailView);
	mMailView->TeamUpWith( mMailRefView);
	MailRefSelectionChanged( 0);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMainWindow::~BmMainWindow() {
	TheMailFolderList = NULL;
	ThePopAccountList = NULL;
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
MMenuBar* BmMainWindow::CreateMenu() {
	mMainMenuBar = new BmMainMenuBar();
	BMenu* menu = NULL;
	// File
	menu = new BMenu( "File");
	menu->AddItem( new BMenuItem( "New Folder", new BMessage( BMM_NEW_MAILFOLDER)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Page Setup...", new BMessage( BMM_PAGE_SETUP)));
	menu->AddItem( new BMenuItem( "Print Message(s)...", new BMessage( BMM_PRINT)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Preferences...", new BMessage( BMM_PREFERENCES)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "About Beam...", new BMessage( B_ABOUT_REQUESTED)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Quit Beam", new BMessage( B_QUIT_REQUESTED), 'Q'));
	mMainMenuBar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( new BMenuItem( "Undo", new BMessage( B_UNDO), 'Z'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Cut", new BMessage( B_CUT), 'X'));
	menu->AddItem( new BMenuItem( "Copy", new BMessage( B_COPY), 'C'));
	menu->AddItem( new BMenuItem( "Paste", new BMessage( B_PASTE), 'V'));
	menu->AddItem( new BMenuItem( "Select All", new BMessage( B_SELECT_ALL), 'A'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Find...", new BMessage( BMM_FIND), 'F'));
	menu->AddItem( new BMenuItem( "Find Messages...", new BMessage( BMM_FIND_MESSAGES), 'F', B_SHIFT_KEY));
	menu->AddItem( new BMenuItem( "Find Next", new BMessage( BMM_FIND_NEXT), 'G'));
	mMainMenuBar->AddItem( menu);

	// Network
	BMenu* accMenu = new BMenu( "Check Mail For");
	mMainMenuBar->SetAccountMenu( accMenu);
	menu = new BMenu( "Network");
	menu->AddItem( new BMenuItem( "Check Mail", new BMessage( BMM_CHECK_MAIL), 'M'));
	menu->AddItem( accMenu);
	menu->AddItem( new BMenuItem( "Check All Accounts", new BMessage( BMM_CHECK_ALL), 'M', B_SHIFT_KEY));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Send Pending Messages", new BMessage( BMM_SEND_PENDING)));
	mMainMenuBar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( new BMenuItem( "New Message", new BMessage( BMM_NEW_MAIL), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Reply", new BMessage( BMM_REPLY), 'R'));
	menu->AddItem( new BMenuItem( "Reply To All", new BMessage( BMM_REPLY_ALL), 'R', B_SHIFT_KEY));
	menu->AddItem( new BMenuItem( "Forward", new BMessage( BMM_FORWARD), 'J'));
//	menu->AddItem( new BMenuItem( "Forward With Attachments", new BMessage( BMM_FORWARD_ATTACHMENTS), 'J', B_SHIFT_KEY));
	menu->AddItem( new BMenuItem( "Bounce (Redirect)", new BMessage( BMM_BOUNCE), 'B'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Apply Filter", new BMessage( BMM_FILTER)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Move To Trash", new BMessage( BMM_TRASH), 'T'));
	mMainMenuBar->AddItem( menu);

	// Help
	menu = new BMenu( "Help");
	mMainMenuBar->AddItem( menu);

	return mMainMenuBar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddRect( MSG_FRAME, Frame())
						|| archive->AddFloat( MSG_VSPLITTER, mVertSplitter->DividerLeftOrTop())
						|| archive->AddFloat( MSG_HSPLITTER, mHorzSplitter->DividerLeftOrTop());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMainWindow::Unarchive( BMessage* archive, bool deep=true) {
	BRect frame;
	float vDividerPos, hDividerPos;
	status_t ret = archive->FindRect( MSG_FRAME, &frame)
						|| archive->FindFloat( MSG_VSPLITTER, &vDividerPos)
						|| archive->FindFloat( MSG_HSPLITTER, &hDividerPos);
	if (ret == B_OK) {
		MoveTo( frame.LeftTop());
		ResizeTo( frame.Width(), frame.Height());
		mVertSplitter->SetPreferredDividerLeftOrTop( vDividerPos);
		mHorzSplitter->SetPreferredDividerLeftOrTop( hDividerPos);
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
		mMailRefView->StartWatching( this, BM_NTFY_MAILREF_SELECTION);
		BM_LOG2( BM_LogMainWindow, BString("MainWindow begins life"));
		mMailFolderView->StartJob( TheMailFolderList.Get());
		mMainMenuBar->StartJob( ThePopAccountList.Get());
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
	return mMailFolderView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmMainWindow::CreateMailRefView( minimax minmax, int32 width, int32 height) {
	mMailRefView = BmMailRefView::CreateInstance( minmax, width, height);
	return mMailRefView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMainWindow::CreateMailView( minimax minmax, BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, false);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_QUIT_REQUESTED: {
				beamApp->PostMessage( B_QUIT_REQUESTED);
				break;
			}
			case B_COPY:
			case B_CUT: 
			case B_PASTE: 
			case B_UNDO: 
			case B_SELECT_ALL: {
				BView* focusView = CurrentFocus();
				if (focusView)
					PostMessage( msg, focusView);
				break;
			}
			case BMM_CHECK_MAIL: {
				const char* key = NULL;
				msg->FindString( BmPopAccountList::MSG_ITEMKEY, &key);
				if (key)
					ThePopAccountList->CheckMailFor( key);
				else
					ThePopAccountList->CheckMail();
				break;
			}
			case BMM_NEW_MAIL: {
				BmMailEditWin* editWin = BmMailEditWin::CreateInstance();
				if (editWin)
					editWin->Show();
				break;
			}
			case B_OBSERVER_NOTICE_CHANGE: {
				switch( msg->FindInt32( B_OBSERVE_WHAT_CHANGE)) {
					case BM_NTFY_MAILREF_SELECTION: {
						MailRefSelectionChanged( msg->FindInt32( BmMailRefView::MSG_MAILS_SELECTED));
					}
				}
				break;
			}
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
	BM_LOG2( BM_LogMainWindow, BString("MainWindow has been asked to quit"));
	Store();
	beamApp->PostMessage( B_QUIT_REQUESTED);
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMainWindow::Quit() {
	mMailView->DetachModel();
	mMailRefView->DetachModel();
	mMailFolderView->DetachModel();
	BM_LOG2( BM_LogMainWindow, BString("MainWindow has quit"));
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
		(err = cacheFile.SetTo( TheResources->StateInfoFolder(), filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store state-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		if (mMailView)
			mMailView->Store();
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	MailRefSelectionChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMainWindow::MailRefSelectionChanged( int32 numSelected) {
	mReplyButton->SetEnabled( numSelected > 0);
	mReplyAllButton->SetEnabled( numSelected > 0);
	mForwardButton->SetEnabled( numSelected > 0);
	mBounceButton->SetEnabled( numSelected > 0);
	mPrintButton->SetEnabled( numSelected * 0 > 0);
	mTrashButton->SetEnabled( numSelected > 0);
}

