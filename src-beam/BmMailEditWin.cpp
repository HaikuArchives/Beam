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
#include <FilePanel.h>
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
#include "BmCheckControl.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmGuiUtil.h"
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
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"

/********************************************************************************\
	BmMailEditWin
\********************************************************************************/

float BmMailEditWin::nNextXPos = 300;
float BmMailEditWin::nNextYPos = 100;
BmMailEditWin::BmEditWinMap BmMailEditWin::nEditWinMap;

/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
#define BM_BCC_ADDED 			'bMYa'
#define BM_CC_ADDED 				'bMYb'
#define BM_CHARSET_SELECTED	'bMYc'
#define BM_FROM_ADDED 			'bMYd'
#define BM_FROM_SET	 			'bMYe'
#define BM_SHOWDETAILS1			'bMYf'
#define BM_SMTP_SELECTED		'bMYg'
#define BM_TO_ADDED 				'bMYh'
#define BM_EDIT_HEADER_DONE	'bMYi'
#define BM_SHOWDETAILS2			'bMYj'
#define BM_SHOWDETAILS3			'bMYk'
#define BM_SIGNATURE_SELECTED	'bMYl'

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMailRef* mailRef) {
	BmEditWinMap::iterator pos = nEditWinMap.find( mailRef->EntryRef());
	if (pos != nEditWinMap.end())
		return pos->second;
	BmMailEditWin* win = new BmMailEditWin( mailRef, NULL);
	win->ReadStateInfo();
	nEditWinMap[mailRef->EntryRef()] = win;
	return win;
}

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMail* mail) {
	BmMailRef* mailRef = mail->MailRef();
	if (mailRef) {
		BmEditWinMap::iterator pos = nEditWinMap.find( mailRef->EntryRef());
		if (pos != nEditWinMap.end())
			return pos->second;
	}
	BmMailEditWin* win = new BmMailEditWin( NULL, mail);
	win->ReadStateInfo();
	if (mailRef)
		nEditWinMap[mailRef->EntryRef()] = win;
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::BmMailEditWin( BmMailRef* mailRef, BmMail* mail)
	:	inherited( "MailEditWin", BRect(50,50,800,600), "Edit Mail", 
					  ThePrefs->GetBool( "UseDocumentResizer", false) 
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails1( false)
	,	mShowDetails2( false)
	,	mShowDetails3( false)
	,	mModified( false)
	,	mHasNeverBeenSaved( mail ? mail->MailRef() == NULL : false)
	,	mAttachPanel( NULL)
{
	CreateGUI();
	mMailView->AddFilter( new BmMsgFilter( mSubjectControl, B_KEY_DOWN));
	if (mail)
		EditMail( mail);
	else
		EditMail( mailRef);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::~BmMailEditWin() {
	delete mAttachPanel;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		BmMailRef* mailRef = mail->MailRef();
		if (mailRef)
			nEditWinMap.erase(mailRef->EntryRef());
	}
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailEditWin::BmMsgFilter::Filter( BMessage* msg, BHandler** handler) {
	if (msg->what == B_KEY_DOWN) {
		BString bytes = msg->FindString( "bytes");
		int32 modifiers = msg->FindInt32( "modifiers");
		if (bytes.Length() && bytes[0]==B_TAB && modifiers & B_SHIFT_KEY) {
			mShiftTabToControl->MakeFocus( true);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
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
				mSmtpControl = new BmMenuControl( "SMTP-Server:", new BPopUpMenu( ""), 0.4),
				0
			),
			new HGroup(
				mShowDetails1Button = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_SHOWDETAILS1), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mToControl = new BmTextControl( "To:", true),
				mCharsetControl = new BmMenuControl( "Charset:", new BPopUpMenu( ""), 0.4),
				0
			),
			mDetails1Group = new VGroup(
				new HGroup(
					mShowDetails2Button = 
					new MPictureButton( minimax( 16,16,16,16), 
											  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
											  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
											  new BMessage( BM_SHOWDETAILS2), this, B_TWO_STATE_BUTTON),
					new Space(minimax(4,-1,4,-1)),
					mCcControl = new BmTextControl( "Cc:", true),
					mReplyToControl = new BmTextControl( "Reply-To:", false),
					0
				),
				mDetails2Group = new HGroup(
					new Space(minimax(20,-1,20,-1)),
					mBccControl = new BmTextControl( "Bcc:", true),
					mSenderControl = new BmTextControl( "Sender:", false),
					0
				),
				0
			),
			mSubjectGroup = new HGroup(
				mShowDetails3Button = 
				new MPictureButton( minimax( 16,16,16,16), 
										  TheResources->CreatePictureFor( &TheResources->mRightArrow, 16, 16), 
										  TheResources->CreatePictureFor( &TheResources->mDownArrow, 16, 16), 
										  new BMessage( BM_SHOWDETAILS3), this, B_TWO_STATE_BUTTON),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", false),
				0
			),
			mDetails3Group = new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mSignatureControl = new BmMenuControl( "Signature:", new BPopUpMenu( "")),
				new Space(minimax(20,-1,20,-1)),
				mEditHeaderControl = new BmCheckControl( "Edit Headers Before Send", 1, false),
				new Space(),
				0
			),
			mSeparator = new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);

	float divider = mToControl->Divider();
	divider = MAX( divider, mSubjectControl->Divider());
	divider = MAX( divider, mFromControl->Divider());
	divider = MAX( divider, mCcControl->Divider());
	divider = MAX( divider, mBccControl->Divider());
	divider = MAX( divider, mReplyToControl->Divider());
	divider = MAX( divider, mSenderControl->Divider());
	divider = MAX( divider, mSignatureControl->Divider());
	mToControl->SetDivider( divider);
	mSubjectControl->SetDivider( divider);
	mFromControl->SetDivider( divider);
	mCcControl->SetDivider( divider);
	mBccControl->SetDivider( divider);
	mReplyToControl->SetDivider( divider);
	mSenderControl->SetDivider( divider);
	mSignatureControl->SetDivider( divider);

	divider = MAX( 0, mSmtpControl->Divider());
	divider = MAX( divider, mCharsetControl->Divider());
	mSmtpControl->SetDivider( divider);
	mCharsetControl->SetDivider( divider);
	mEditHeaderControl->ct_mpm = mSmtpControl->ct_mpm;

	mShowDetails1Button->SetFlags( mShowDetails1Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails2Button->SetFlags( mShowDetails2Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails3Button->SetFlags( mShowDetails3Button->Flags() & (0xFFFFFFFF^B_NAVIGABLE));

	// initially, the detail-parts are hidden:
	if (!mShowDetails1)
		mDetails1Group->RemoveSelf();
	if (!mShowDetails2)
		mDetails2Group->RemoveSelf();
	if (!mShowDetails3)
		mDetails3Group->RemoveSelf();

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
	// add all encodings to menu:
	for( int i=0; BM_Encodings[i].charset; ++i) {
		mCharsetControl->Menu()->AddItem( new BMenuItem( BM_Encodings[i].charset, new BMessage(BM_CHARSET_SELECTED)));
	}
	// add all signatures to signature menu:
	mSignatureControl->Menu()->AddItem( new BMenuItem( "<none>", new BMessage( BM_SIGNATURE_SELECTED)));
	for( iter = TheSignatureList->begin(); iter != TheSignatureList->end(); ++iter) {
		BmSignature* sig = dynamic_cast< BmSignature*>( iter->second.Get());
		mSignatureControl->Menu()->AddItem( new BMenuItem( sig->Key().String(), new BMessage( BM_SIGNATURE_SELECTED)));
	}

	mSaveButton->SetEnabled( mModified);
	mMailView->SetModificationMessage( new BMessage( BM_TEXTFIELD_MODIFIED));
	mMailView->BodyPartView()->StartWatching( this, BM_NTFY_LISTCONTROLLER_MODIFIED);

	// temporarily disabled:
	mPeopleButton->SetEnabled( false);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
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
		BRect scrFrame = bmApp->ScreenFrame();
		frame.bottom = MIN( frame.bottom, scrFrame.bottom-5);
		frame.right = MIN( frame.right, scrFrame.right-5);
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
	menu->AddItem( CreateMenuItem( "Save", BMM_SAVE, "SaveMail"));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), bmApp);
	menubar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( CreateMenuItem( "Undo", B_UNDO));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Cut", B_CUT));
	menu->AddItem( CreateMenuItem( "Copy", B_COPY));
	menu->AddItem( CreateMenuItem( "Paste", B_PASTE));
	menu->AddItem( CreateMenuItem( "Select All", B_SELECT_ALL));
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Find...", BMM_FIND));
	menu->AddItem( CreateMenuItem( "Find Next", BMM_FIND_NEXT));
	menubar->AddItem( menu);

	// Network
	menu = new BMenu( "Network");
	menu->AddItem( CreateMenuItem( "Send Mail Now", BMM_SEND_NOW));
	menu->AddItem( CreateMenuItem( "Send Mail Later", BMM_SEND_LATER));
	menu->AddSeparatorItem();
	menubar->AddItem( menu);

	// Message
	menu = new BMenu( "Message");
	menu->AddItem( CreateMenuItem( "New Message", BMM_NEW_MAIL));
	menubar->AddItem( menu);

	// temporary deactivations:
	menubar->FindItem( BMM_FIND)->SetEnabled( false);
	menubar->FindItem( BMM_FIND_NEXT)->SetEnabled( false);

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
			case BM_SHOWDETAILS1: {
				int32 newVal = (mShowDetails1 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails1Button->SetValue( newVal);
				mShowDetails1 = newVal != B_CONTROL_OFF;
				if (mShowDetails1)
					mOuterGroup->AddChild( mDetails1Group, mSubjectGroup);
				else
					mDetails1Group->RemoveSelf();
				RecalcSize();
				break;
			}
			case BM_SHOWDETAILS2: {
				int32 newVal = (mShowDetails2 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails2Button->SetValue( newVal);
				mShowDetails2 = newVal != B_CONTROL_OFF;
				if (mShowDetails2)
					mDetails1Group->AddChild( mDetails2Group);
				else
					mDetails2Group->RemoveSelf();
				RecalcSize();
				break;
			}
			case BM_SHOWDETAILS3: {
				int32 newVal = (mShowDetails3 ? B_CONTROL_OFF : B_CONTROL_ON);
				mShowDetails3Button->SetValue( newVal);
				mShowDetails3 = newVal != B_CONTROL_OFF;
				if (mShowDetails3)
					mOuterGroup->AddChild( mDetails3Group, mSeparator);
				else
					mDetails3Group->RemoveSelf();
				RecalcSize();
				break;
			}
			case BMM_SEND_LATER:
			case BMM_SEND_NOW: {
				BM_LOG2( BM_LogMailEditWin, BString("Asked to send mail"));
				if (!SaveMail())
					break;
				BM_LOG2( BM_LogMailEditWin, "...mail was saved");
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (!mail)
					break;
				if (mail->IsFieldEmpty(BM_FIELD_FROM)) {
					BM_SHOWERR("Please enter at least one address into the <FROM> field before sending this mail, thank you.");
					break;
				}
				if (mail->IsFieldEmpty(BM_FIELD_TO) && mail->IsFieldEmpty(BM_FIELD_CC)
				&& mail->IsFieldEmpty(BM_FIELD_BCC)) {
					BM_SHOWERR("Please enter at least one address into the\n\t<TO>,<CC> or <BCC>\nfield before sending this mail, thank you.");
					break;
				}
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
							break;
						} else {
							BM_LOG2( BM_LogMailEditWin, "...marking mail as pending");
							mail->MarkAs( BM_MAIL_STATUS_PENDING);
							smtpAcc->mMailVect.push_back( mail);
							BM_LOG2( BM_LogMailEditWin, "...passing mail to smtp-account");
							TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
						}
					} else {
						ShowAlertWithType( "Before you can send this mail, you have to select the SMTP-Account to use for sending it.",
												 B_INFO_ALERT);
						break;
					}
				} else 
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
				PostMessage( B_QUIT_REQUESTED);
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
				SaveMail();
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
						mSmtpControl->MarkItem( acc->SMTPAccount().String());
						// update signature:
						BString sigName = acc->SignatureName();
						mMailView->SetSignatureByName( sigName);
						if (sigName.Length())
							mSignatureControl->MarkItem( sigName.String());
						else
							mSignatureControl->MarkItem( "<none>");
					}
				}
				break;
			}
			case BMM_ATTACH: {
				entry_ref attachRef;
				if (msg->FindRef( "refs", 0, &attachRef) != B_OK) {
					// first step, let user select files to attach:
					if (!mAttachPanel) {
						mAttachPanel = new BFilePanel( B_OPEN_PANEL, new BMessenger(this), NULL,
																 B_FILE_NODE, true, msg);
					}
					mAttachPanel->Show();
				} else {
					// second step, attach selected files to mail:
					mMailView->BodyPartView()->AddAttachment( msg);
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
			case BM_SIGNATURE_SELECTED: {
				// exactly, no break here...
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					BString sigName = item->Label();
					if (sigName=="<none>")
						mMailView->SetSignatureByName( "");
					else
						mMailView->SetSignatureByName( sigName);
				}
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
	BeginLife()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::BeginLife() {
}

/*------------------------------------------------------------------------------*\
	Show()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::Show() {
	if (!Looper()->IsLocked()) {
		// showing living window, we bring it to front:
		LockLooper();
		if (IsMinimized())
			Minimize( false);
		inherited::Hide();
		inherited::Show();
		UnlockLooper();
	} else
		inherited::Show();
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
		if (mail->IsRedirect()) {
			mBccControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_RESENT_BCC).String());
			mCcControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_RESENT_CC).String());
			mFromControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_RESENT_FROM).String());
			mSenderControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_RESENT_SENDER).String());
			mToControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_RESENT_TO).String());
		} else {
			mBccControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_BCC).String());
			mCcControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_CC).String());
			mFromControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_FROM).String());
			mSenderControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_SENDER).String());
			mToControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_TO).String());
			mReplyToControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_REPLY_TO).String());
		}
		mSubjectControl->SetTextSilently( mail->GetStrippedFieldVal( BM_FIELD_SUBJECT).String());
		// mark corresponding SMTP-account (if any):
		BString smtpAccount = mail->AccountName();
		mSmtpControl->MarkItem( smtpAccount.String());
		// mark signature of current mail as selected:
		BString sigName = mail->SignatureName();
		if (sigName.Length())
			mSignatureControl->MarkItem( sigName.String());
		else
			mSignatureControl->MarkItem( "<none>");
		// mark corresponding charset:
		mCharsetControl->MarkItem( EncodingToCharset( mail->DefaultEncoding()).String());
		// try to set convenient focus:
		if (!mFromControl->TextView()->TextLength())
			mFromControl->MakeFocus( true);
		else if (!mToControl->TextView()->TextLength())
			mToControl->MakeFocus( true);
		else if (!mSubjectControl->TextView()->TextLength())
			mSubjectControl->MakeFocus( true);
		else
			mMailView->MakeFocus( true);
		// now make certain fields visible if they contain values:
		if (BString(mCcControl->Text()).Length()) {
			mShowDetails1Button->SetValue( 1);
			mShowDetails1Button->Invoke();
		}
		if (BString(mBccControl->Text()).Length()) {
			mShowDetails2Button->SetValue( 1);
			mShowDetails2Button->Invoke();
		}
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
		mail->AccountName( smtpAccount);
		if (mail->IsRedirect()) {
			mail->SetFieldVal( BM_FIELD_RESENT_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_RESENT_TO, mToControl->Text());
		} else {
			mail->SetFieldVal( BM_FIELD_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_TO, mToControl->Text());
			mail->SetFieldVal( BM_FIELD_REPLY_TO, mReplyToControl->Text());
		}
		mail->SetFieldVal( BM_FIELD_SUBJECT, mSubjectControl->Text());
/* 
	use the following line if mail-date should be bumped whenever the mail
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
bool BmMailEditWin::SaveMail() {
	if (!mModified && !mHasNeverBeenSaved)
		return true;
	if (!CreateMailFromFields())
		return false;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail && mail->Store()) {
		mHasNeverBeenSaved = false;
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
				return SaveMail();
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
