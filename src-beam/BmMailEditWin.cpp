/*
	BmMailEditWin.cpp
		$Id$
*/

#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include <regexx.hh>
	using namespace regexx;

#include "PrefilledBitmap.h"

#include "Beam.h"
#include "BmBasics.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailView.h"
#include "BmMailEditWin.h"
#include "BmMenuControl.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSmtpAccount.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailEditWin
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
#define BM_BCC_ADDED 		'bMYa'
#define BM_CC_ADDED 			'bMYb'
#define BM_FROM_ADDED 		'bMYc'
#define BM_TO_ADDED 			'bMYd'
#define BM_SMTP_SELECTED	'bMYe'
#define BM_SHOWDETAILS		'bMYf'
#define BM_FROM_SET	 		'bMYg'

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance() 
{
	BmMailEditWin* win = new BmMailEditWin;
	win->ReadStateInfo();
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::BmMailEditWin()
	:	inherited( "MailEditWin", BRect(50,50,800,600), "Edit Mail", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails( false)
{
	CreateGUI();
	mMailView->ShowMail( new BmMail( true));
}

/*------------------------------------------------------------------------------*\
	CreateGUI()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::CreateGUI() {
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
				mFromControl = new BmTextControl( "From:", true),
				mSmtpControl = new BmMenuControl( "SMTP-Server:", new BPopUpMenu( "")),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mToControl = new BmTextControl( "To:", true),
				0
			),
			new HGroup(
				mShowDetailsButton = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_SHOWDETAILS), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", false),
				mCharsetControl = new BmMenuControl( "Charset:", new BPopUpMenu( "")),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mCcControl = new BmTextControl( "Cc:", true),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mBccControl = new BmTextControl( "Bcc:", true),
				0
			),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mReplyToControl = new BmTextControl( "Reply-To:", false),
				mSenderControl = new BmTextControl( "Sender:", false),
				0
			),
/*
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				0
			),
*/
			new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);
		
	float divider = mToControl->Divider();
	divider = MAX( divider, mSubjectControl->Divider());
	divider = MAX( divider, mFromControl->Divider());
	divider = MAX( divider, mCcControl->Divider());
	divider = MAX( divider, mBccControl->Divider());
	divider = MAX( divider, mReplyToControl->Divider());
	mToControl->SetDivider( divider);
	mSubjectControl->SetDivider( divider);
	mFromControl->SetDivider( divider);
	mCcControl->SetDivider( divider);
	mBccControl->SetDivider( divider);
	mReplyToControl->SetDivider( divider);

	divider = MAX( 0, mSmtpControl->Divider());
	divider = MAX( divider, mCharsetControl->Divider());
	mSmtpControl->SetDivider( divider-5);
	mCharsetControl->SetDivider( divider-5);

	mShowDetailsButton->SetFlags( mShowDetailsButton->Flags() & (0xFFFFFFFF^B_NAVIGABLE));

	// initially, the detail-parts are hidden:
	mCcControl->DetachFromParent();
	mBccControl->DetachFromParent();
	mReplyToControl->DetachFromParent();
	mSenderControl->DetachFromParent();

	// add all popaccounts to from menu twice (one for single-address mode, one for adding addresses):
	mFromControl->Menu()->SetLabelFromMarked( false);
	BmModelItemMap::const_iterator iter;
	BMenu* subMenu = new BMenu( "Add...");
	for( iter = ThePopAccountList->begin(); iter != ThePopAccountList->end(); ++iter) {
		BmPopAccount* acc = dynamic_cast< BmPopAccount*>( iter->second.Get());
		mFromControl->Menu()->AddItem( new BMenuItem( acc->Key().String(), new BMessage( BM_FROM_SET)));
		subMenu->AddItem( new BMenuItem( acc->Key().String(), new BMessage( BM_FROM_ADDED)));
	}
	mFromControl->Menu()->AddItem( subMenu);

	// add all smtpaccounts to smtp menu:
	for( iter = TheSmtpAccountList->begin(); iter != TheSmtpAccountList->end(); ++iter) {
		BmSmtpAccount* acc = dynamic_cast< BmSmtpAccount*>( iter->second.Get());
		mSmtpControl->Menu()->AddItem( new BMenuItem( acc->Key().String(), new BMessage( BM_SMTP_SELECTED)));
	}

	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		mCharsetControl->Menu()->AddItem( new BMenuItem( BM_Encodings[i].charset, new BMessage('bmyy')));
	}
	// mark default charset:
	BMenuItem* item = mCharsetControl->Menu()->FindItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
	if (item)
		item->SetMarked( true);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
	
	mMailView->AddFilter( new BmMailViewFilter( this));
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
			case BM_SHOWDETAILS: {
				int32 newVal = (mShowDetails ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetailsButton->SetValue( newVal);
				mShowDetails = newVal != B_CONTROL_OFF;
				if (mShowDetails) {
					mCcControl->ReattachToParent();
					mBccControl->ReattachToParent();
					mReplyToControl->ReattachToParent();
					mSenderControl->ReattachToParent();
//					mCharsetControl->ReattachToParent();
//					mSmtpControl->ReattachToParent();
				} else {
					mCcControl->DetachFromParent();
					mBccControl->DetachFromParent();
					mReplyToControl->DetachFromParent();
					mSenderControl->DetachFromParent();
//					mCharsetControl->DetachFromParent();
//					mSmtpControl->DetachFromParent();
				}
				RecalcSize();
				break;
			}
			case BMM_SEND_NOW: {
				UpdateMailFields();
				break;
			}
			case BMM_SAVE: {
				UpdateMailFields();
				BmRef<BmMail> mail = mMailView->CurrMail();
				mail->Store();
				break;
			}
			case BM_FROM_SET:
			case BM_FROM_ADDED: {
				Regexx rx;
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					BmListModelItemRef accRef = ThePopAccountList->FindItemByKey( item->Label());
					BmPopAccount* acc = dynamic_cast< BmPopAccount*>( accRef.Get()); 
					if (acc) {
						BString fromString = msg->what == BM_FROM_ADDED ? mFromControl->Text() : "";
						if (rx.exec( fromString, "\\S+")) {
							fromString << ", " << acc->GetFromAddress();
						} else {
							fromString << acc->GetFromAddress();
						}
						mFromControl->SetText( fromString.String());
						mFromControl->TextView()->Select( fromString.Length(), fromString.Length());
						mFromControl->TextView()->ScrollToSelection();
					}
				}
				break;
			}
			case BM_BCC_ADDED: {
				break;
			}
			case BM_CC_ADDED: {
				break;
			}
			case BM_TO_ADDED: {
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
	UpdateMailFields()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::UpdateMailFields() {
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		mail->SetFieldVal( BM_FIELD_BCC, mBccControl->Text());
		mail->SetFieldVal( BM_FIELD_CC, mCcControl->Text());
		mail->SetFieldVal( BM_FIELD_FROM, mFromControl->Text());
		mail->SetFieldVal( BM_FIELD_SUBJECT, mSubjectControl->Text());
		mail->SetFieldVal( BM_FIELD_TO, mToControl->Text());
		mail->SetFieldVal( BM_FIELD_REPLY_TO, mReplyToControl->Text());
		BMenuItem* charsetItem = mCharsetControl->Menu()->FindMarked();
		int32 encoding = charsetItem ? CharsetToEncoding( charsetItem->Label()) 
											  : ThePrefs->GetInt( "DefaultEncoding");
		BString editedText = mMailView->Text();
		BMenuItem* smtpItem = mSmtpControl->Menu()->FindMarked();
		BString smtpAccount = smtpItem ? smtpItem->Label() : "";
		mail->ConstructRawText( editedText, encoding, smtpAccount);
	}
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::QuitRequested() {
	BM_LOG2( BM_LogMailEditWin, BString("MailEditWin has been asked to quit"));
	return true;
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	standard BeOS-behaviour, we quit
\*------------------------------------------------------------------------------*/
void BmMailEditWin::Quit() {
	mMailView->WriteStateInfo();
	mMailView->DetachModel();
	BM_LOG2( BM_LogMailEditWin, BString("MailEditWin has quit"));
	inherited::Quit();
}



/********************************************************************************\
	BmMailViewFilter
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMailViewFilter()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewFilter::BmMailViewFilter( BmMailEditWin* editWin)
	:	inherited( BM_MAILVIEW_SHOWRAW)
	,	mEditWin( editWin) 
{
}	

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailViewFilter::Filter( BMessage* msg, BHandler** target) {
/*
	if (mEditWin)
		mEditWin->UpdateMailFields();
*/
	return B_DISPATCH_MESSAGE;
}
