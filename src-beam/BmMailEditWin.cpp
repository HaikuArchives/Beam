/*
	BmMailEditWin.cpp
		$Id$
*/

#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "PrefilledBitmap.h"

#include "Beam.h"
#include "BmBasics.h"
#include "BmEncoding.h"
#include "BmLogHandler.h"
#include "BmMailView.h"
#include "BmMailEditWin.h"
#include "BmMenuControl.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmResources.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"


/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance() 
{
	BmMailEditWin *win = NULL;
	status_t err;
	BString winFilename;
	BFile winFile;

	// create standard main-window:
	win = new BmMailEditWin;
	// try to open state-cache-file...
	winFilename = BString("MailEditWin");
	if ((err = winFile.SetTo( TheResources->StateInfoFolder(), winFilename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, archive file found, we fetch our dimensions from it:
		try {
			BMessage archive;
			(err = archive.Unflatten( &winFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch window archive from file\n\t<") << winFilename << ">\n\n Result: " << strerror(err));
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
BmMailEditWin::BmMailEditWin()
	:	inherited( BRect(50,50,800,600), "Edit Mail", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails( false)
{
	mOuterGroup = 
		new VGroup(
			minimax( 500, 400, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mSendButton = new BmToolbarButton( "Send", 
																  TheResources->IconByName("Button_Send"), 
																  new BMessage(BMM_SEND_NOW), this, 
																  "Send mail now"),
					mSaveButton = new BmToolbarButton( "Save", 
																	TheResources->IconByName("Button_Save"), 
																	new BMessage(BMM_SAVE), this, 
																	"Save mail as draft (for later use)"),
					mNewButton = new BmToolbarButton( "New", 
																 TheResources->IconByName("Button_New"), 
																 new BMessage(BMM_NEW_MAIL), this, 
																 "Compose a new mail message"),
					mAttachButton = new BmToolbarButton( "Attach", 
																	 TheResources->IconByName("Attachment"), 
																	 new BMessage(BMM_ATTACH), this, 
																	 "Attach a file to this mail"),
					mPeopleButton = new BmToolbarButton( "People", 
																	 TheResources->IconByName("Person"), 
																	 new BMessage(BMM_SHOW_PEOPLE), this, 
																	 "Show people information (addresses)"),
					mPrintButton = new BmToolbarButton( "Print", 
																	TheResources->IconByName("Button_Print"), 
																	new BMessage(BMM_PRINT), this, 
																	"Print selected messages(s)"),
					new Space(),
					0
				)
			),
			new Space(minimax(-1,4,-1,4)),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mFromControl = new BmMenuControl( "From:", BM_FIELD_FROM, new BPopUpMenu( "Mail Accounts")),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mToControl = new BmTextControl( "To:", BM_FIELD_TO, true),
				0
			),
			new HGroup(
				mShowDetailsButton = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_MAILEDIT_SHOWDETAILS), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", BM_FIELD_SUBJECT, false),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mCcControl = new BmTextControl( "Cc:", BM_FIELD_CC, true),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mBccControl = new BmTextControl( "Bcc:", BM_FIELD_BCC, true),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mEncodingControl = new BmMenuControl( "Encoding:", BM_FIELD_CONTENT_TRANSFER_ENCODING, new BPopUpMenu( "Encoding:")),
				0
			),
			new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);
		
	float divider = mToControl->Divider();
	divider = max( divider, mSubjectControl->Divider());
	divider = max( divider, mFromControl->Divider());
	divider = max( divider, mCcControl->Divider());
	divider = max( divider, mBccControl->Divider());
	mToControl->SetDivider( divider);
	mSubjectControl->SetDivider( divider);
	mFromControl->SetDivider( divider);
	mCcControl->SetDivider( divider);
	mBccControl->SetDivider( divider);
	mEncodingControl->SetDivider( divider);
	mShowDetailsButton->SetFlags( mShowDetailsButton->Flags() & (0xFFFFFFFF^B_NAVIGABLE));

	// initially, the detail-parts are hidden:
	mCcControl->DetachFromParent();
	mBccControl->DetachFromParent();
	mEncodingControl->DetachFromParent();

	// add all popaccounts to menu:
	BmModelItemMap::const_iterator iter;
	for( iter = ThePopAccountList->begin(); iter != ThePopAccountList->end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		mFromControl->Menu()->AddItem( new BMenuItem( acc->Key().String(), new BMessage('bmyy')));
	}

	// add all encodings to menu:
	for( const char **enc = BmEncoding::BM_Encodings; *enc; ++enc) {
		mEncodingControl->Menu()->AddItem( new BMenuItem( *enc, new BMessage('bmyy')));
	}

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::~BmMailEditWin() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
MMenuBar* BmMailEditWin::CreateMenu() {
	MMenuBar* menubar = new MMenuBar();
	BMenu* menu = NULL;
//	BMenuItem* item = NULL;
	// File
	menu = new BMenu( "File");
	menu->AddItem( new BMenuItem( "Open...", new BMessage( BMM_OPEN), 'O'));
	menu->AddItem( new BMenuItem( "Save...", new BMessage( BMM_SAVE), 'S'));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Page Setup...", new BMessage( BMM_PAGE_SETUP)));
	menu->AddItem( new BMenuItem( "Print Message(s)...", new BMessage( BMM_PRINT)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Preferences...", new BMessage( BMM_PREFERENCES)));
	menu->AddSeparatorItem();
	menu->AddItem( new BMenuItem( "Quit Beam", new BMessage( B_QUIT_REQUESTED), 'Q'));
	menubar->AddItem( menu);

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
	menu->AddItem( new BMenuItem( "Find Next", new BMessage( BMM_FIND_NEXT), 'G'));
	menubar->AddItem( menu);

	// Network
	menu = new BMenu( "Network");
	menu->AddItem( new BMenuItem( "Send Mail", new BMessage( BMM_SEND_NOW), 'E'));
	menu->AddSeparatorItem();
	menubar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( new BMenuItem( "New Message", new BMessage( BMM_NEW_MAIL), 'N'));
	menubar->AddItem( menu);

	return menubar;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailEditWin::Archive( BMessage* archive, bool deep=true) const {
	status_t ret = archive->AddRect( MSG_FRAME, Frame());
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailEditWin::Unarchive( BMessage* archive, bool deep=true) {
	BRect frame;
	status_t ret = archive->FindRect( MSG_FRAME, &frame);
	if (ret == B_OK) {
		MoveTo( frame.LeftTop());
		ResizeTo( frame.Width(), frame.Height());
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMailEditWin::CreateMailView( minimax minmax, BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, true);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_MAILEDIT_SHOWDETAILS: {
				int32 newVal = (mShowDetails ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetailsButton->SetValue( newVal);
				mShowDetails = newVal != B_CONTROL_OFF;
				if (mShowDetails) {
					mCcControl->ReattachToParent();
					mBccControl->ReattachToParent();
					mEncodingControl->ReattachToParent();
				} else {
					mCcControl->DetachFromParent();
					mBccControl->DetachFromParent();
					mEncodingControl->DetachFromParent();
				}
				RecalcSize();
				break;
			}
			case BM_TEXT_CHANGED: {
				BTextControl* control = NULL;
				msg->FindPointer( "source", (void**)&control);
				if (control) {
					BmRef<BmMail> mail = mMailView->CurrMail();
					if (mail) {
						mail->SetFieldVal( control->Name(), control->Text());
					}
				}
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
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("MailEditWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::QuitRequested() {
	BM_LOG2( BM_LogMailEditWin, BString("MailEditWin has been asked to quit"));
	Store();
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMailEditWin::Quit() {
	mMailView->DetachModel();
	BM_LOG2( BM_LogMailEditWin, BString("MailEditWin has quit"));
	inherited::Quit();
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores MailEditWin-state inside StateCache-folder:
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BString filename = BString( "MailEditWin");
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive MailEditWin-object");
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
