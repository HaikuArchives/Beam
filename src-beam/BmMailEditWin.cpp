/*
	BmMailEditWin.cpp
		$Id$
*/

#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "PrefilledBitmap.h"

#include "Beam.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailRef.h"
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
	,	mRawMode( false)
	,	mModified( false)
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
			minimax( 200, 300, 1E5, 1E5),
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

	// add all smtp-accounts to smtp menu:
	for( iter = TheSmtpAccountList->begin(); iter != TheSmtpAccountList->end(); ++iter) {
		BmSmtpAccount* acc = dynamic_cast< BmSmtpAccount*>( iter->second.Get());
		mSmtpControl->Menu()->AddItem( new BMenuItem( acc->Key().String(), new BMessage( BM_SMTP_SELECTED)));
	}
	// mark default smtp-account:
	BString defaultSmtp = ThePrefs->GetString( "DefaultSmtpAccount");
	BMenuItem* item = NULL;
	if (defaultSmtp.Length())
		item = mSmtpControl->Menu()->FindItem( defaultSmtp.String());
	else 
		item = mSmtpControl->Menu()->ItemAt( 0);
	if (item)
		item->SetMarked( true);

	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		mCharsetControl->Menu()->AddItem( new BMenuItem( BM_Encodings[i].charset, new BMessage('bmyy')));
	}
	// mark default charset:
	item = mCharsetControl->Menu()->FindItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
	if (item)
		item->SetMarked( true);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
	
	mMailView->AddFilter( new BmMailViewFilter( BM_MAILVIEW_SHOWRAW, this));
	mMailView->AddFilter( new BmMailViewFilter( BM_MAILVIEW_SHOWCOOKED, this));
	mMailView->AddFilter( new BmMailViewFilter( B_SIMPLE_DATA, this));
	mMailView->BodyPartView()->AddFilter( new BmMailViewFilter( B_SIMPLE_DATA, this));
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
	menu->AddItem( new BMenuItem( "Save", new BMessage( BMM_SAVE), 'S'));
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
	menu->AddItem( new BMenuItem( "Send Mail Now", new BMessage( BMM_SEND_NOW), 'E'));
	menu->AddItem( new BMenuItem( "Send Mail Later", new BMessage( BMM_SEND_LATER), 'E', B_SHIFT_KEY));
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
				} else {
					mCcControl->DetachFromParent();
					mBccControl->DetachFromParent();
					mReplyToControl->DetachFromParent();
					mSenderControl->DetachFromParent();
				}
				RecalcSize();
				break;
			}
			case BMM_SEND_LATER:
			case BMM_SEND_NOW: {
				if (!SaveAndReloadMail())
					break;
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (!mail)
					break;
				mail->MarkAs( BM_MAIL_STATUS_PENDING);
				if (msg->what == BMM_SEND_NOW) {
					BmListModelItemRef smtpRef	= TheSmtpAccountList->FindItemByKey( mail->AccountName());
					BmSmtpAccount* smtpAcc = dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
					if (smtpAcc) {
						smtpAcc->mMailVect.push_back( mail);
						smtpAcc->SendQueuedMail();
						Quit();
					}
				}
				break;
			}
			case BMM_SAVE: {
				SaveAndReloadMail();
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
					if (msg->what == BM_FROM_SET) {
						// select corresponding smtp-account, if any:
						BMenuItem* item = mSmtpControl->Menu()->FindItem( acc->SMTPAccount().String());
						if (item)
							item->SetMarked( true);
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
			case BM_FIELD_MODIFIED: {
				mModified = true;
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
	EditMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::EditMail( BmMailRef* ref) {
	mMailView->ShowMail( ref, false);
							// false=>synchronize (i.e. wait till mail is being displayed)
	BmRef<BmMail> mail = mMailView->CurrMail();
	SetFieldsFromMail( mail.Get());
}

/*------------------------------------------------------------------------------*\
	EditMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::EditMail( BmMail* mail) {
	mMailView->ShowMail( mail, false);
							// false=>synchronize (i.e. wait till mail is being displayed)
	SetFieldsFromMail( mail);
}

/*------------------------------------------------------------------------------*\
	CurrMail()
		-	
\*------------------------------------------------------------------------------*/
BmMail* BmMailEditWin::CurrMail() const { 
	return mMailView ? mMailView->CurrMail().Get() : NULL; 
}

/*------------------------------------------------------------------------------*\
	SetFieldFromMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetFieldsFromMail( BmMail* mail) {
	if (mail) {
		mBccControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_BCC).String());
		mCcControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_CC).String());
		mFromControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_FROM).String());
		mSenderControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_SENDER).String());
		mSubjectControl->SetText( mail->GetFieldVal( BM_FIELD_SUBJECT).String());
		mToControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_TO).String());
		mReplyToControl->SetText( mail->GetStrippedFieldVal( BM_FIELD_REPLY_TO).String());
		// mark corresponding SMTP-account (if any):
		BMenuItem* item = NULL;
		BString smtpAccount = mail->AccountName();
		if (smtpAccount.Length())
			item = mSmtpControl->Menu()->FindItem( smtpAccount.String());
		else 
			item = mSmtpControl->Menu()->ItemAt( 0);
		if (item)
		item->SetMarked( true);
		// mark corresponding charset:
		item = mCharsetControl->Menu()->FindItem( EncodingToCharset( mail->DefaultEncoding()).String());
		if (item)
			item->SetMarked( true);
		mModified = false;
	}
}

/*------------------------------------------------------------------------------*\
	CreateMailFromFields()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::CreateMailFromFields() {
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		BString editedText;
		BString convertedText;
		mMailView->GetWrappedText( editedText);
		BMenuItem* charsetItem = mCharsetControl->Menu()->FindMarked();
		int32 encoding = charsetItem 
								? CharsetToEncoding( charsetItem->Label())
						 		: ThePrefs->GetInt("DefaultEncoding");
		ConvertFromUTF8( encoding, editedText, convertedText);
		BMenuItem* smtpItem = mSmtpControl->Menu()->FindMarked();
		BString smtpAccount = smtpItem ? smtpItem->Label() : "";
		if (mRawMode) {
			// N.B.: In raw mode the user is editing the raw message
			// 		text which may contain characters from different 
			//			character-encodings. Since we have just converted the
			//			raw message text from UTF8 to the single selected encoding,
			//			we may have lost some characters on the way.
			// 		This does not apply to quoted-printable- or base64-encoded
			//			MIME-parts, but it will cause problems when using 8-bit-mime.
			//			So we should probably forbid combining 8-bit-mime and 
			//			raw-message-editing. 
			//			Currently, Beam does not use 8-bit-mime, but we have to be
			//			careful here, should we add support for it later...
			mail->SetTo( convertedText, smtpAccount);
			return mail->InitCheck() == B_OK;
		} else {
			mail->SetFieldVal( BM_FIELD_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_SUBJECT, mSubjectControl->Text());
			mail->SetFieldVal( BM_FIELD_TO, mToControl->Text());
			mail->SetFieldVal( BM_FIELD_REPLY_TO, mReplyToControl->Text());
/* use the following line if mail-date should be bumped whenever the mail
	has been edited:
			mail->SetFieldVal( BM_FIELD_DATE, TimeToString( time( NULL), 
																			"%a, %d %b %Y %H:%M:%S %z"));
*/
			return mail->ConstructRawText( convertedText, encoding, smtpAccount);
		}
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	SetEditMode()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetEditMode( int32 mode) {
	mRawMode = (mode == BM_MAILVIEW_SHOWRAW);
	bool enabled = (mRawMode == false);
	mAttachButton->SetEnabled( enabled);
	mPeopleButton->SetEnabled( enabled);
	mPrintButton->SetEnabled( enabled);
	mBccControl->SetEnabled( enabled);
	mCcControl->SetEnabled( enabled);
	mFromControl->SetEnabled( enabled);
	mReplyToControl->SetEnabled( enabled);
	mSenderControl->SetEnabled( enabled);
	mSubjectControl->SetEnabled( enabled);
	mToControl->SetEnabled( enabled);
}

/*------------------------------------------------------------------------------*\
	SaveAndReloadMail()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::SaveAndReloadMail() {
	if (!CreateMailFromFields())
		return false;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail && mail->Store()) {
		EditMail( mail.Get());
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::QuitRequested() {
	BM_LOG2( BM_LogMailEditWin, BString("MailEditWin has been asked to quit"));
	if (mModified) {
		BAlert* alert = new BAlert( "title", "Save mail as draft before closing?",
											 "Cancel", "Don't Save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut( 0, B_ESCAPE);
		int32 result = alert->Go();
		if (result == 0)
			return false;
		if (result == 2)
			return SaveAndReloadMail();
	}
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
BmMailViewFilter::BmMailViewFilter( int32 msgCmd, BmMailEditWin* editWin)
	:	inherited( msgCmd)
	,	mEditWin( editWin) 
{
}	

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailViewFilter::Filter( BMessage* msg, BHandler** target) {
	switch( msg->what) {
		case BM_MAILVIEW_SHOWRAW:
		case BM_MAILVIEW_SHOWCOOKED: {
			if (mEditWin && mEditWin->SaveAndReloadMail()) {
				mEditWin->SetEditMode( msg->what);
				return B_DISPATCH_MESSAGE;
			}
			break;
		}
		case B_SIMPLE_DATA: {
			if (mEditWin && !mEditWin->IsInRawMode()) {
				return B_DISPATCH_MESSAGE;
			}
		}
	}
	return B_SKIP_MESSAGE;
}
