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


#include <FilePanel.h>
#include <MenuItem.h>
#include <MessageFilter.h>

#ifdef BEOS_VERSION_6 	// Zeta	
	#include <interface/EMailCompleter.h>
#endif

#include "BmString.h"

#include <HGroup.h>
#include <VGroup.h>
#include <MBorder.h>
#include <MMenuBar.h>
#include <Space.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmMailEditWin.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmToolbarButton.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmRosterBase.h"
#include "BmTextControl.h"

#include "BmLogHandler.h"

/********************************************************************************\
	BmMailEditWin
\********************************************************************************/

float BmMailEditWin::nNextXPos = 300;
float BmMailEditWin::nNextYPos = 100;

const char* const BmMailEditWin::MSG_CONTROL = 	"ctrl";
const char* const BmMailEditWin::MSG_ADDRESS = 	"addr";

const char* const BmMailEditWin::MSG_DETAIL1 = 	"det1";
const char* const BmMailEditWin::MSG_DETAIL2 = 	"det2";
const char* const BmMailEditWin::MSG_DETAIL3 = 	"det3";

// stuff living in BmMailEditWinPart2.cpp
BmMailEditWin* FindMailEditWinFor( const BmString& key);
void AddMailEditWin( const BmString& key, BmMailEditWin* win);
void RemoveMailEditWin( BmMailEditWin* win);
BMessageFilter* CreateShiftTabMsgFilter(BControl* stControl, uint32 cmd);
BMessageFilter* CreatePeopleDropMsgFilter(uint32 cmd);

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMailRef* mailRef) {
	BmString key = BmString() << mailRef->NodeRef().node;
	BmMailEditWin* win = FindMailEditWinFor( key);
	if (!win) {
		win = new BmMailEditWin( mailRef, NULL);
		win->ReadStateInfo();
		AddMailEditWin( key, win);
	}
	return win;
}

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creates a new mail-edit window
		-	initialiazes the window's dimensions by reading its archive-file (if any)
\*------------------------------------------------------------------------------*/
BmMailEditWin* BmMailEditWin::CreateInstance( BmMail* mail) {
	BmMailRef* mailRef = mail->MailRef();
	BmString key;
	BmMailEditWin* win = NULL;
	if (mailRef) {
		key = BmString() << mailRef->NodeRef().node;
		win = FindMailEditWinFor( key);
		if (win)
			return win;
	}
	win = new BmMailEditWin( NULL, mail);
	win->ReadStateInfo();
	if (mailRef)
		AddMailEditWin( key, win);
	return win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin::BmMailEditWin( BmMailRef* mailRef, BmMail* mail)
	:	inherited( "MailEditWin", BRect(50,50,800,600), "Edit Mail", 
					  ThePrefs->GetBool( "UseDocumentResizer", true)
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mMailView( NULL)
	,	mShowDetails1( false)
	,	mShowDetails2( false)
	,	mShowDetails3( false)
	,	mPrefsShowDetails1( false)
	,	mPrefsShowDetails2( false)
	,	mPrefsShowDetails3( false)
	,	mModified( false)
	,	mHasNeverBeenSaved( mail ? mail->MailRef() == NULL : false)
	,	mHasBeenSent( false)
	,	mAttachPanel( NULL)
{
	CreateGUI();
	mMailView->AddFilter( CreateShiftTabMsgFilter( mSubjectControl, B_KEY_DOWN));
	mToControl->AddFilter( CreatePeopleDropMsgFilter( B_SIMPLE_DATA));
	mCcControl->AddFilter( CreatePeopleDropMsgFilter( B_SIMPLE_DATA));
	mBccControl->AddFilter( CreatePeopleDropMsgFilter( B_SIMPLE_DATA));
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
	RemoveMailEditWin( this);
	// now manually delete all sub-views that are not currently added to the
	// window:
	if (!mShowDetails1)
		delete mDetails1Group;
	if (!mShowDetails2)
		delete mDetails2Group;
	if (!mShowDetails3)
		delete mDetails3Group;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailEditWin::ArchiveState( BMessage* archive) const {
	inherited::ArchiveState( archive);
	status_t ret = archive->AddBool( MSG_DETAIL1, mPrefsShowDetails1)
						|| archive->AddBool( MSG_DETAIL2, mPrefsShowDetails2)
						|| archive->AddBool( MSG_DETAIL3, mPrefsShowDetails3);
	return ret;
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
		archive->FindBool( MSG_DETAIL1, &mPrefsShowDetails1);
		if (mPrefsShowDetails1)
			SetDetailsButton( 1, B_CONTROL_ON);
		archive->FindBool( MSG_DETAIL2, &mPrefsShowDetails2);
		if (mPrefsShowDetails2)
			SetDetailsButton( 2, B_CONTROL_ON);
		archive->FindBool( MSG_DETAIL3, &mPrefsShowDetails3);
		if (mPrefsShowDetails3)
			SetDetailsButton( 3, B_CONTROL_ON);
		WriteStateInfo();
	} else {
		MoveTo( BPoint( nNextXPos, nNextYPos));
		ResizeTo( 400, 400);
		WriteStateInfo();
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	CreateGUI()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::CreateGUI() {
	// Get maximum button size
	float width=0, height=0;
	BmToolbarButton::CalcMaxSize( width, height, "Send",		
										   TheResources->IconByName("Button_Send"));
	BmToolbarButton::CalcMaxSize( width, height, "Save",		
											TheResources->IconByName("Button_Save"));
	BmToolbarButton::CalcMaxSize( width, height, "New",			
											TheResources->IconByName("Button_New"));
	BmToolbarButton::CalcMaxSize( width, height, "Attach",
											TheResources->IconByName("Button_Attachment"));

	mOuterGroup = 
		new VGroup(
			minimax( 200, 300, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 1, NULL,
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mSendButton 
						= new BmToolbarButton( 
									"Send", 
									TheResources->IconByName("Button_Send"), 
									width, height,
									new BMessage(BMM_SEND_NOW), this, 
									"Send mail now", true
								),
					mSaveButton 
						= new BmToolbarButton( 
									"Save", 
									TheResources->IconByName("Button_Save"), 
									width, height,
									new BMessage(BMM_SAVE), this, 
									"Save mail as draft (for later use)"
								),
					mNewButton 
						= new BmToolbarButton( 
									"New", 
									TheResources->IconByName("Button_New"), 
									width, height,
									new BMessage(BMM_NEW_MAIL), this, 
									"Compose a new mail message"
								),
					mAttachButton 
						= new BmToolbarButton( 
									"Attach", 
									TheResources->IconByName("Button_Attachment"), 
									width, height,
									new BMessage(BMM_ATTACH), this, 
									"Attach a file to this mail"
								),
					new ToolbarSpace(),
					0
				)
			),
			new Space(minimax(-1,4,-1,4)),
			new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mFromControl = new BmTextControl( 
					"From:", 
					new BmMenuController( "From:", this, 
												 new BMessage( BM_FROM_SET), 
												 &BmRosterBase::RebuildIdentityMenu,
												 BM_MC_RADIO_MODE)
				),
				mSmtpControl = new BmMenuControl( 
					"SMTP-Server:", 
					new BmMenuController( "", this, 
												 new BMessage( BM_SMTP_SELECTED), 
												 &BmRosterBase::RebuildSmtpAccountMenu, 
												 BM_MC_LABEL_FROM_MARKED),
					0.5
				),
				0
			),
			new HGroup(
				mShowDetails1Button = 
					new MPictureButton( 
						minimax( 16,16,16,16), 
						TheResources->CreatePictureFor( &TheResources->mRightArrow, 
																  16, 16), 
						TheResources->CreatePictureFor( &TheResources->mDownArrow, 
																  16, 16), 
						new BMessage( BM_SHOWDETAILS1), this, B_TWO_STATE_BUTTON
					),
				new Space(minimax(4,-1,4,-1)),
				mToControl = new BmTextControl( 
					"To:", 
					new BmMenuController( "To:", this, 
												 new BMessage( BM_TO_ADDED), 
												 &BmRosterBase::RebuildPeopleMenu)
				),
				mCharsetControl = new BmMenuControl( 
					"Charset:", 
					new BmMenuController( "", this, 
												 new BMessage( BM_CHARSET_SELECTED), 
												 &BmRosterBase::RebuildCharsetMenu, 
												 BM_MC_LABEL_FROM_MARKED),
					0.5
				),
				0
			),
			mDetails1Group = new VGroup(
				new HGroup(
					mShowDetails2Button = 
						new MPictureButton( minimax( 16,16,16,16), 
							TheResources->CreatePictureFor( &TheResources->mRightArrow, 
																	  16, 16), 
							TheResources->CreatePictureFor( &TheResources->mDownArrow,
																	  16, 16), 
							new BMessage( BM_SHOWDETAILS2), this, B_TWO_STATE_BUTTON
						),
					new Space(minimax(4,-1,4,-1)),
					mCcControl = new BmTextControl( 
						"Cc:", 
						new BmMenuController( "Cc:", this, 
													 new BMessage( BM_CC_ADDED), 
													 &BmRosterBase::RebuildPeopleMenu)
					),
					mReplyToControl = new BmTextControl( "Reply-To:", false),
					0
				),
				mDetails2Group = new HGroup(
					new Space(minimax(20,-1,20,-1)),
					mBccControl = new BmTextControl( 
						"Bcc:", 
						new BmMenuController( "Bcc:", this, 
													 new BMessage( BM_BCC_ADDED), 
													 &BmRosterBase::RebuildPeopleMenu)
					),
					mSenderControl = new BmTextControl( "Sender:", false),
					0
				),
				0
			),
			mSubjectGroup = new HGroup(
				mShowDetails3Button = 
					new MPictureButton( minimax( 16,16,16,16), 
						TheResources->CreatePictureFor( &TheResources->mRightArrow,
																  16, 16), 
						TheResources->CreatePictureFor( &TheResources->mDownArrow, 
																  16, 16), 
						new BMessage( BM_SHOWDETAILS3), this, B_TWO_STATE_BUTTON
					),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", false),
				0
			),
			mDetails3Group = new HGroup(
				new Space(minimax(20,-1,20,-1)),
				mSignatureControl = new BmMenuControl( 
					"Signature:", 
					new BmMenuController( 
						"Signature:", this, 
						new BMessage( BM_SIGNATURE_SELECTED), 
						&BmRosterBase::RebuildSignatureMenu, 
						BM_MC_ADD_NONE_ITEM | BM_MC_LABEL_FROM_MARKED
					)
				),
				new Space(minimax(20,-1,20,-1)),
				mFileIntoControl = new BmMenuControl( 
					"Target Folder:",
					new BmMenuControllerBase( 
						"out", this, 
						new BMessage( BM_FILEINTO_SELECTED), 
						&BmRosterBase::RebuildFolderMenu
					),
					3.0
				),
				new Space(minimax(20,-1,20,-1)),
				mEditHeaderControl = new BmCheckControl( "Edit Headers Before Send", 
																	  1, false),
				0
			),
			mSeparator = new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);

	mSendButton->AddActionVariation( "Send Now", new BMessage(BMM_SEND_NOW));
	mSendButton->AddActionVariation( "Send Later", new BMessage(BMM_SEND_LATER));

	BmDividable::DivideSame(
		mToControl,
		mSubjectControl,
		mFromControl,
		mCcControl,
		mBccControl,
		mReplyToControl,
		mSenderControl,
		mSignatureControl,
		NULL
	);

	BmDividable::DivideSame(
		mSmtpControl,
		mCharsetControl,
		NULL
	);
	mCharsetControl->ct_mpm = mSmtpControl->ct_mpm;

	mShowDetails1Button->SetFlags( mShowDetails1Button->Flags() 
												& (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails2Button->SetFlags( mShowDetails2Button->Flags() 
												& (0xFFFFFFFF^B_NAVIGABLE));
	mShowDetails3Button->SetFlags( mShowDetails3Button->Flags()
												& (0xFFFFFFFF^B_NAVIGABLE));

	// initially, the detail-parts are hidden:
	mDetails2Group->RemoveSelf();
	mDetails1Group->RemoveSelf();
	mDetails3Group->RemoveSelf();

	mSaveButton->SetEnabled( mModified);
	mMailView->SetModificationMessage( new BMessage( BM_TEXTFIELD_MODIFIED));

	// watch changes to bodypartview in order to be set the
	// changed-flag accordingly:	
	mMailView->BodyPartView()->StartWatching( this, 
															BM_NTFY_LISTCONTROLLER_MODIFIED);

#ifdef BEOS_VERSION_6 	// Zeta	
	mToControl->TextView()->AddFilter(new BEMailCompleter);
	mCcControl->TextView()->AddFilter(new BEMailCompleter);
	mBccControl->TextView()->AddFilter(new BEMailCompleter);
#endif

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailViewContainer* BmMailEditWin::CreateMailView( minimax minmax, 
																	 BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, true);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::MessageReceived( BMessage* msg) {
	int32 newVal;
	try {
		switch( msg->what) {
			case BM_SHOWDETAILS1: {
				newVal = mShowDetails1 ? B_CONTROL_OFF : B_CONTROL_ON;
				SetDetailsButton( 1, newVal);
				mPrefsShowDetails1 = (newVal == B_CONTROL_ON);
				break;
			}
			case BM_SHOWDETAILS2: {
				newVal = mShowDetails2 ? B_CONTROL_OFF : B_CONTROL_ON;
				SetDetailsButton( 2, newVal);
				mPrefsShowDetails2 = (newVal == B_CONTROL_ON);
				break;
			}
			case BM_SHOWDETAILS3: {
				newVal = mShowDetails3 ? B_CONTROL_OFF : B_CONTROL_ON;
				SetDetailsButton( 3, newVal);
				mPrefsShowDetails3 = (newVal == B_CONTROL_ON);
				break;
			}
			case BMM_SEND_LATER:
			case BMM_SEND_NOW: {
				if (mEditHeaderControl->Value()) {
					if (!EditHeaders())
						break;
				}
				SendMail( msg->what == BMM_SEND_NOW);
				break;
			}
			case BMM_SAVE: {
				SaveMail( false);
				break;
			}
			case BM_TO_ADDED: {
				AddAddressToTextControl( 
					mToControl, 
					msg->FindString( BmListModel::MSG_ITEMKEY)
				);
				break;
			}
			case BM_CC_ADDED: {
				AddAddressToTextControl( 
					mCcControl, 
					msg->FindString( BmListModel::MSG_ITEMKEY)
				);
				break;
			}
			case BM_BCC_ADDED: {
				AddAddressToTextControl( 
					mBccControl, 
					msg->FindString( BmListModel::MSG_ITEMKEY)
				);
				break;
			}
			case BM_TO_REMOVE: {
				RemoveAddressFromTextControl( 
					mToControl, 
					msg->FindString( MSG_ADDRESS)
				);
				break;
			}
			case BM_CC_REMOVE: {
				RemoveAddressFromTextControl( 
					mCcControl, 
					msg->FindString( MSG_ADDRESS)
				);
				break;
			}
			case BM_BCC_REMOVE: {
				RemoveAddressFromTextControl( 
					mBccControl, 
					msg->FindString( MSG_ADDRESS)
				);
				break;
			}
			case BM_TO_CLEAR: {
				mToControl->SetText( "");
				mToControl->TextView()->Select( 0, 0);
				mToControl->TextView()->ScrollToSelection();
				break;
			}
			case BM_CC_CLEAR: {
				mCcControl->SetText( "");
				mCcControl->TextView()->Select( 0, 0);
				mCcControl->TextView()->ScrollToSelection();
				break;
			}
			case BM_BCC_CLEAR: {
				mBccControl->SetText( "");
				mBccControl->TextView()->Select( 0, 0);
				mBccControl->TextView()->ScrollToSelection();
				break;
			}
			case BM_FROM_SET: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item)
					HandleFromSet(item->Label());
				break;
			}
			case BMM_ATTACH: {
				entry_ref attachRef;
				if (msg->FindRef( "refs", 0, &attachRef) != B_OK) {
					// first step, let user select files to attach:
					if (!mAttachPanel) {
						mAttachPanel = new BFilePanel( B_OPEN_PANEL, 
																 new BMessenger(this), NULL,
																 B_FILE_NODE, true, msg);
					}
					mAttachPanel->Show();
				} else {
					// second step, attach selected files to mail:
					mMailView->BodyPartView()->AddAttachment( msg);
				}
				break;
			}
			case BM_SIGNATURE_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					BmString sigName = item->Label();
					if (sigName==BM_NoItemLabel)
						mMailView->SetSignatureByName( "");
					else
						mMailView->SetSignatureByName( sigName);
					mModified = true;
					mSaveButton->SetEnabled( true);
				}
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				BControl* source;
				if (msg->FindPointer( "source", (void**)&source)==B_OK
				&& source==mSubjectControl) {
					SetTitle( (BmString("Edit Mail: ") 
								 + mSubjectControl->Text()).String());
				}
				mModified = true;
				mSaveButton->SetEnabled( true);
				break;
			}
			case BM_CHARSET_SELECTED: {
				BMenuItem* item = NULL;
				msg->FindPointer( "source", (void**)&item);
				if (item) {
					mCharsetControl->ClearMark();
					mCharsetControl->MenuItem()->SetLabel( item->Label());
					item->SetMarked( true);
					mModified = true;
					mSaveButton->SetEnabled( true);
					// need to tell BodyPartView about new charset:
					if (ThePrefs->GetBool( "ImportExportTextAsUtf8", true))
						mMailView->BodyPartView()->DefaultCharset( "utf-8");
					else
						mMailView->BodyPartView()->DefaultCharset( item->Label());
				}
				break;
			}
			case BM_SMTP_SELECTED:
			case B_OBSERVER_NOTICE_CHANGE: {
				mModified = true;
				mSaveButton->SetEnabled( true);
				break;
			}
			case BMM_NEW_MAIL: {
				be_app_messenger.SendMessage( msg);
				break;
			}
			case BM_FILEINTO_SELECTED: {
				BView* srcView = NULL;
				msg->FindPointer( "source", (void**)&srcView);
				BMenuItem* item = dynamic_cast<BMenuItem*>( srcView);
				if (item) {
					BMenuItem* currItem = item;
					BMenu* currMenu = item->Menu();
					BmString path;
					while( currMenu && currItem 
					&& currItem!=mFileIntoControl->MenuItem()) {
						if (!path.Length())
							path.Prepend( BmString(currItem->Label()));
						else
							path.Prepend( BmString(currItem->Label()) << "/");
						currItem = currMenu->Superitem();
						currMenu = currMenu->Supermenu();
					}
					mFileIntoControl->ClearMark();
					item->SetMarked( true);
					mFileIntoControl->MenuItem()->SetLabel( path.String());
				} else {
					mFileIntoControl->ClearMark();
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailEditWin: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddAddressToTextControl( BmTextControl* cntrl, email)
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::AddAddressToTextControl( BmTextControl* cntrl, 
															const BmString& email) {
	if (cntrl) {
		BmString currStr = cntrl->Text();
		currStr.Trim();
		BmAddress addr( email);
		BmString mailAddr;
		if (ThePrefs->GetBool( "AddPeopleNameToMailAddr", true))
			mailAddr = addr.AddrString();
		else
			mailAddr = addr.AddrSpec();
		
		if (currStr.Length() > 0)
			currStr << ", " << mailAddr;
		else
			currStr << mailAddr;
		cntrl->SetText( currStr.String());
		cntrl->TextView()->Select( currStr.Length(), currStr.Length());
		cntrl->TextView()->ScrollToSelection();
	}
}

/*------------------------------------------------------------------------------*\
	RemoveAddressFromTextControl( BmTextControl* cntrl, email)
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::RemoveAddressFromTextControl( BmTextControl* cntrl, 
																  const BmString& email) {
	if (cntrl) {
		BmAddressList addr( cntrl->Text());
		addr.Remove( email);
		BmString currStr( addr.AddrString());
		cntrl->SetText( currStr.String());
		cntrl->TextView()->Select( currStr.Length(), currStr.Length());
		cntrl->TextView()->ScrollToSelection();
	}
}
