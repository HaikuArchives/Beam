/*
	BmMailEditWin.cpp
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


#include <File.h>
#include <InterfaceKit.h>
#include <Message.h>
#include <String.h>

#include <layout-all.h>

#include "regexx.hh"
	using namespace regexx;

#include "PrefilledBitmap.h"
#include "TextEntryAlert.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailHeader.h"
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

float BmMailEditWin::nNextXPos = 300;
float BmMailEditWin::nNextYPos = 100;

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
#define BM_BCC_ADDED 			'bMYa'
#define BM_CC_ADDED 				'bMYb'
#define BM_CHARSET_SELECTED	'bMYc'
#define BM_FROM_ADDED 			'bMYd'
#define BM_FROM_SET	 			'bMYe'
#define BM_SHOWDETAILS			'bMYf'
#define BM_SMTP_SELECTED		'bMYg'
#define BM_TO_ADDED 				'bMYh'
#define BM_EDIT_HEADER_DONE	'bMYi'

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMailRef* mailRef) {
	BmMailEditWin* win = new BmMailEditWin( mailRef, NULL);
	win->ReadStateInfo();
	return win;
}

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMail* mail) {
	BmMailEditWin* win = new BmMailEditWin( NULL, mail);
	win->ReadStateInfo();
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::BmMailEditWin( BmMailRef* mailRef, BmMail* mail)
	:	inherited( "MailEditWin", BRect(50,50,800,600), "Edit Mail", B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails( false)
	,	mModified( false)
{
	CreateGUI();
	if (mail)
		EditMail( mail);
	else
		EditMail( mailRef);
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
				mEditHeaderControl = new MCheckBox( "Edit Headers Before Send", 1, false),
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
	mEditHeaderControl->RemoveSelf();
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
		mCharsetControl->Menu()->AddItem( new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_CHARSET_SELECTED)));
	}
	// mark default charset:
	item = mCharsetControl->Menu()->FindItem( EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding")).String());
	if (item)
		item->SetMarked( true);

	mSaveButton->SetEnabled( false);
	mMailView->SetModificationMessage( new BMessage( BM_TEXTFIELD_MODIFIED));
	mMailView->BodyPartView()->StartWatching( this, BM_NTFY_LISTCONTROLLER_MODIFIED);

	// temporarily disabled:
	mAttachButton->SetEnabled( false);
	mPeopleButton->SetEnabled( false);
	mPrintButton->SetEnabled( false);

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
status_t BmMailEditWin::UnarchiveState( BMessage* archive) {
	status_t ret = inherited::UnarchiveState( archive);
	if (ret == B_OK) {
		BRect frame = Frame();
		if (nNextXPos != frame.left || nNextYPos != frame.top) {
			nNextXPos = frame.left;
			nNextYPos = frame.top;
		} else {
			nNextXPos += 10;
			nNextYPos += 16;
			if (nNextYPos > 300) {
				nNextXPos = 300;
				nNextYPos = 100;
			}
		}
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( frame.Width(), frame.Height());
		WriteStateInfo();
	} else {
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( 400, 400);
		WriteStateInfo();
	}
	return ret;
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
					mBccControl->Parent()->AddChild( mEditHeaderControl);
					mReplyToControl->ReattachToParent();
					mSenderControl->ReattachToParent();
				} else {
					mCcControl->DetachFromParent();
					mBccControl->DetachFromParent();
					mEditHeaderControl->RemoveSelf();
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
				if (msg->what == BMM_SEND_NOW) {
					BmRef<BmListModelItem> smtpRef = TheSmtpAccountList->FindItemByKey( mail->AccountName());
					BmSmtpAccount* smtpAcc = dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
					if (smtpAcc) {
						if (mEditHeaderControl->Value()) {
							// allow user to edit mail-header before we send it:
							BRect screen( bmApp->ScreenFrame());
							float w=600, h=400;
							BRect alertFrame( (screen.Width()-w)/2,(screen.Height()-h)/2,
													(screen.Width()+w)/2,(screen.Height()+h)/2);
							BString headerStr;
							ConvertLinebreaksToLF( mail->Header()->HeaderString(), headerStr);
							TextEntryAlert* alert = 
								new TextEntryAlert( "Edit Headers", 
														  "Please edit the mail-headers below:",
														  headerStr.String(),
														  "Cancel",
														  "OK, Send Message",
														  false, 80, 20, B_WIDTH_FROM_LABEL, true,
														  &alertFrame
								);
							alert->SetShortcut( B_ESCAPE, 0);
							alert->TextEntryView()->DisallowChar( 27);
							alert->TextEntryView()->SetFontAndColor( be_fixed_font);
							alert->Go( new BInvoker( new BMessage( BM_EDIT_HEADER_DONE), BMessenger( this)));
						} else {
							mail->MarkAs( BM_MAIL_STATUS_PENDING);
							smtpAcc->mMailVect.push_back( mail);
							TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
							PostMessage( B_QUIT_REQUESTED);
						}
					}
				} else 
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
				break;
			}
			case BM_EDIT_HEADER_DONE: {
				// User is done with editing the mail-header. We reconstruct the mail with
				// the new header and then send it:
				BmRef<BmMail> mail = mMailView->CurrMail();
				int32 result;
				const char* headerStr; 
				if (!mail || msg->FindInt32( "which", &result) != B_OK 
				|| msg->FindString( "entry_text", &headerStr) != B_OK || result != 1)
					break;
				BmRef<BmListModelItem> smtpRef = TheSmtpAccountList->FindItemByKey( mail->AccountName());
				BmSmtpAccount* smtpAcc = dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
				if (smtpAcc) {
					mail->SetNewHeader( headerStr);
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
					smtpAcc->mMailVect.push_back( mail);
					TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
					PostMessage( B_QUIT_REQUESTED);
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
					BmRef<BmListModelItem> accRef = ThePopAccountList->FindItemByKey( item->Label());
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
			case BM_CHARSET_SELECTED:
			case BM_SMTP_SELECTED:
			case B_OBSERVER_NOTICE_CHANGE:
			case BM_TEXTFIELD_MODIFIED: {
				mModified = true;
				mSaveButton->SetEnabled( true);
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
			case BMM_NEW_MAIL: {
				be_app_messenger.SendMessage( msg);
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
BmRef<BmMail> BmMailEditWin::CurrMail() const { 
	return mMailView->CurrMail();
}

/*------------------------------------------------------------------------------*\
	SetFieldFromMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetFieldsFromMail( BmMail* mail) {
	if (mail) {
		mBccControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_BCC).String());
		mCcControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_CC).String());
		mFromControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_FROM).String());
		mSenderControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_SENDER).String());
		mSubjectControl->SetTextSilently( mail->GetFieldVal( BM_FIELD_SUBJECT).String());
		mToControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_TO).String());
		mReplyToControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_REPLY_TO).String());
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
	} else
		return false;
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
		mModified = false;
		mSaveButton->SetEnabled( false);
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
		if (IsMinimized())
			Minimize( false);
		Activate();
		BAlert* alert = new BAlert( "title", "Save mail as draft before closing?",
											 "Cancel", "Don't Save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut( 0, B_ESCAPE);
		int32 result = alert->Go();
		switch( result) {
			case 0:
				return false;
			case 1: {
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (mail) {
					// reset mail to original values:
					mail->ResyncFromDisk();
				}
				break;
			}
			case 2:
				return SaveAndReloadMail();
		}
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
