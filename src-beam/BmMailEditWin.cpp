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
#include "BmString.h"

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
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMailHeader.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMailEditWin.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmTextControl.h"
#include "BmToolbarButton.h"
#include "BmUtil.h"
#include "BmPeople.h"


/*------------------------------------------------------------------------------*\
	types of messages handled by a BmMailEditWin:
\*------------------------------------------------------------------------------*/
enum {
	BM_TO_CLEAR 			= 'bMYa',
	BM_TO_ADDED				= 'bMYb',
	BM_CC_CLEAR 			= 'bMYc',
	BM_CC_ADDED				= 'bMYd',
	BM_BCC_CLEAR 			= 'bMYe',
	BM_BCC_ADDED			= 'bMYf',
	BM_CHARSET_SELECTED	= 'bMYg',
	BM_FROM_SET	 			= 'bMYi',
	BM_SMTP_SELECTED		= 'bMYj',
	BM_EDIT_HEADER_DONE	= 'bMYk',
	BM_SHOWDETAILS1		= 'bMYl',
	BM_SHOWDETAILS2		= 'bMYm',
	BM_SHOWDETAILS3		= 'bMYn',
	BM_SIGNATURE_SELECTED= 'bMYo'
};



/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
static void RebuildPeopleMenu( BmMenuControllerBase* peopleMenu) {
	BMenuItem* old;
	while( (old = peopleMenu->RemoveItem( (int32)0)) != NULL)
		delete old;
	
	BMessage* msgTempl = peopleMenu->MsgTemplate();
	// add all adresses to menu and a menu-entry for clearing the field:
	ThePeopleList->AddPeopleToMenu( peopleMenu, *msgTempl,
											  BmListModel::MSG_ITEMKEY);
	peopleMenu->AddSeparatorItem();
	BMessage* clearMsg;
	if (msgTempl->what == BM_TO_ADDED)
		clearMsg = new BMessage( BM_TO_CLEAR);
	else if (msgTempl->what == BM_CC_ADDED)
		clearMsg = new BMessage( BM_CC_CLEAR);
	else
		clearMsg = new BMessage( BM_BCC_CLEAR);
	peopleMenu->AddItem( new BMenuItem( "<Clear Field>", clearMsg));
}




/********************************************************************************\
	BmMailEditWin
\********************************************************************************/

float BmMailEditWin::nNextXPos = 300;
float BmMailEditWin::nNextYPos = 100;
BmMailEditWin::BmEditWinMap BmMailEditWin::nEditWinMap;

const char* const BmMailEditWin::MSG_CONTROL = 	"ctrl";
const char* const BmMailEditWin::MSG_ADDRESS = 	"addr";

const char* const BmMailEditWin::MSG_DETAIL1 = 	"det1";
const char* const BmMailEditWin::MSG_DETAIL2 = 	"det2";
const char* const BmMailEditWin::MSG_DETAIL3 = 	"det3";

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
					  ThePrefs->GetBool( "UseDocumentResizer", true)
					  		? B_DOCUMENT_WINDOW_LOOK 
					  		: B_TITLED_WINDOW_LOOK, 
					  B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	,	mShowDetails1( false)
	,	mShowDetails2( false)
	,	mShowDetails3( false)
	,	mPrefsShowDetails1( false)
	,	mPrefsShowDetails2( false)
	,	mPrefsShowDetails3( false)
	,	mModified( false)
	,	mHasNeverBeenSaved( mail ? mail->MailRef() == NULL : false)
	,	mAttachPanel( NULL)
{
	CreateGUI();
	mMailView->AddFilter( new BmShiftTabMsgFilter( mSubjectControl, B_KEY_DOWN));
	mToControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
	mCcControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
	mBccControl->AddFilter( new BmPeopleDropMsgFilter( B_SIMPLE_DATA));
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
filter_result BmMailEditWin::BmShiftTabMsgFilter::Filter( BMessage* msg, 
																			 BHandler**) {
	if (msg->what == B_KEY_DOWN) {
		BmString bytes = msg->FindString( "bytes");
		int32 modifiers = msg->FindInt32( "modifiers");
		if (bytes.Length() && bytes[0]==B_TAB && modifiers & B_SHIFT_KEY) {
			mShiftTabToControl->MakeFocus( true);
			return B_SKIP_MESSAGE;
		}
	}
	return B_DISPATCH_MESSAGE;
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmMailEditWin::BmPeopleDropMsgFilter::Filter( BMessage* msg, 
																				BHandler** handler) {
	filter_result res = B_DISPATCH_MESSAGE;
	BView* cntrl = handler ? dynamic_cast< BView*>( *handler) : NULL;
	if (msg && msg->what == B_SIMPLE_DATA && cntrl) {
		BmMailEditWin* win = dynamic_cast< BmMailEditWin*>( cntrl->Window());
		if (!win)
			return res;
		entry_ref eref;
		for( int32 i=0; msg->FindRef( "refs", i, &eref) == B_OK; ++i) {
			if (CheckMimeType( &eref, "application/x-person")) {
				BNode personNode( &eref);
				if (personNode.InitCheck() != B_OK)
					continue;
				BmString addr;
				BmReadStringAttr( &personNode, "META:email", addr);
				win->AddAddressToTextControl( dynamic_cast< BmTextControl*>( cntrl), 
														addr);
				res = B_SKIP_MESSAGE;
			}
		}
	}
	return res;
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
											TheResources->IconByName("Attachment"));
	BmToolbarButton::CalcMaxSize( width, height, "People",
											TheResources->IconByName("Person"));
	BmToolbarButton::CalcMaxSize( width, height, "Print",
											TheResources->IconByName("Button_Print"));

	mOuterGroup = 
		new VGroup(
			minimax( 200, 300, 1E5, 1E5),
			CreateMenu(),
			new MBorder( M_RAISED_BORDER, 3, NULL,
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
									TheResources->IconByName("Attachment"), 
									width, height,
									new BMessage(BMM_ATTACH), this, 
									"Attach a file to this mail"
								),
					mPeopleButton 
						= new BmToolbarButton( 
									"People", 
									TheResources->IconByName("Person"), 
									width, height,
									new BMessage(BMM_SHOW_PEOPLE), this, 
									"Show people information (addresses)"
								),
					mPrintButton 
						= new BmToolbarButton( 
									"Print", 
									TheResources->IconByName("Button_Print"), 
									width, height,
									new BMessage(BMM_PRINT), this, 
									"Print selected messages(s)"
								),
					new Space(),
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
												 TheIdentityList.Get(),
												 BM_MC_RADIO_MODE)
				),
				mSmtpControl = new BmMenuControl( 
					"SMTP-Server:", 
					new BmMenuController( "", this, 
												 new BMessage( BM_SMTP_SELECTED), 
												 TheSmtpAccountList.Get(), 
												 BM_MC_LABEL_FROM_MARKED),
					0.4
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
												 RebuildPeopleMenu)
				),
				mCharsetControl = new BmMenuControl( 
					"Charset:", 
					new BmMenuController( "", this, 
												 new BMessage( BM_CHARSET_SELECTED), 
												 BmRebuildCharsetMenu, 
												 BM_MC_LABEL_FROM_MARKED),
					0.4
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
													 RebuildPeopleMenu)
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
													 RebuildPeopleMenu)
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
						TheSignatureList.Get(), 
						BM_MC_ADD_NONE_ITEM | BM_MC_LABEL_FROM_MARKED
					)
				),
				new Space(minimax(20,-1,20,-1)),
				mEditHeaderControl = new BmCheckControl( "Edit Headers Before Send", 
																	  1, false),
				new Space(),
				0
			),
			mSeparator = new Space(minimax(-1,4,-1,4)),
			CreateMailView( minimax(200,200,1E5,1E5), BRect(0,0,400,200)),
			0
		);

	mSendButton->AddActionVariation( "Send Now", new BMessage(BMM_SEND_NOW));
	mSendButton->AddActionVariation( "Send Later", new BMessage(BMM_SEND_LATER));

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

	// temporarily disabled:
	mPeopleButton->SetEnabled( false);

	AddChild( dynamic_cast<BView*>(mOuterGroup));
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
	AddItemToMenu( menu, CreateMenuItem( "Preferences...", 
						BMM_PREFERENCES), bmApp);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), bmApp);
	menubar->AddItem( menu);

	// Edit
	menu = new BMenu( "Edit");
	menu->AddItem( CreateMenuItem( "Undo", B_UNDO));
	menu->AddItem( CreateMenuItem( "Redo", B_REDO));
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
BmMailViewContainer* BmMailEditWin::CreateMailView( minimax minmax, 
																	 BRect frame) {
	mMailView = BmMailView::CreateInstance( minmax, frame, true);
	return mMailView->ContainerView();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetDetailsButton( int32 nr, int32 newVal) {
	switch( nr) {
		case 1: {
			if (mShowDetails1 != (newVal == B_CONTROL_ON)) {
				mShowDetails1Button->SetValue( newVal);
				mShowDetails1 = (newVal == B_CONTROL_ON);
				if (mShowDetails1)
					mOuterGroup->AddChild( mDetails1Group, mSubjectGroup);
				else
					mDetails1Group->RemoveSelf();
			}
			break;
		}
		case 2: {
			if (mShowDetails2 != (newVal == B_CONTROL_ON)) {
				mShowDetails2Button->SetValue( newVal);
				mShowDetails2 = (newVal == B_CONTROL_ON);
				if (mShowDetails2)
					mDetails1Group->AddChild( mDetails2Group);
				else
					mDetails2Group->RemoveSelf();
			}
			break;
		}
		case 3: {
			if (mShowDetails3 != (newVal == B_CONTROL_ON)) {
				mShowDetails3Button->SetValue( newVal);
				mShowDetails3 = (newVal == B_CONTROL_ON);
				if (mShowDetails3)
					mOuterGroup->AddChild( mDetails3Group, mSeparator);
				else
					mDetails3Group->RemoveSelf();
			}
			break;
		}
	}
	RecalcSize();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::MessageReceived( BMessage* msg) {
	Regexx rx;
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
				BM_LOG2( BM_LogGui, BmString("MailEditWin: Asked to send mail"));
				if (!SaveMail( true))
					break;
				BM_LOG2( BM_LogGui, "MailEditWin: ...mail was saved");
				BmRef<BmMail> mail = mMailView->CurrMail();
				if (!mail)
					break;
				if (mail->IsFieldEmpty( mail->IsRedirect() 
													? BM_FIELD_RESENT_FROM 
													: BM_FIELD_FROM)) {
					BM_SHOWERR("Please enter at least one address into the <FROM> "
								  "field before sending this mail, thank you.");
					break;
				}
				if (mail->IsFieldEmpty( mail->IsRedirect() 
						? BM_FIELD_RESENT_TO 
						: BM_FIELD_TO) 
				&& mail->IsFieldEmpty( mail->IsRedirect() 
						? BM_FIELD_RESENT_CC 
						: BM_FIELD_CC)
				&& mail->IsFieldEmpty( mail->IsRedirect() 
						? BM_FIELD_RESENT_BCC 
						: BM_FIELD_BCC)) {
					BM_SHOWERR("Please enter at least one address into the\n"
								  "\t<TO>,<CC> or <BCC>\nfield before sending this "
								  "mail, thank you.");
					break;
				}
				if (msg->what == BMM_SEND_NOW) {
					BmRef<BmListModelItem> smtpRef 
						= TheSmtpAccountList->FindItemByKey( mail->AccountName());
					BmSmtpAccount* smtpAcc 
						= dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
					if (smtpAcc) {
						if (mEditHeaderControl->Value()) {
							// allow user to edit mail-header before we send it:
							BRect screen( bmApp->ScreenFrame());
							float w=600, h=400;
							BRect alertFrame( (screen.Width()-w)/2,
													(screen.Height()-h)/2,
													(screen.Width()+w)/2,
													(screen.Height()+h)/2);
							BmString headerStr;
							headerStr.ConvertLinebreaksToLF( 
															&mail->Header()->HeaderString());
							TextEntryAlert* alert = 
								new TextEntryAlert( 
									"Edit Headers", 
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
							alert->Go( 
								new BInvoker( new BMessage( BM_EDIT_HEADER_DONE), 
												  BMessenger( this))
							);
							break;
						} else {
							BM_LOG2( BM_LogGui, 
										"MailEditWin: ...marking mail as pending");
							mail->MarkAs( BM_MAIL_STATUS_PENDING);
							smtpAcc->mMailVect.push_back( mail);
							BM_LOG2( BM_LogGui, 
										"MailEditWin: ...passing mail to smtp-account");
							TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
						}
					} else {
						ShowAlertWithType( "Before you can send this mail, "
												 "you have to select the SMTP-Account "
												 "to use for sending it.",
												 B_INFO_ALERT);
						break;
					}
				} else 
					mail->MarkAs( BM_MAIL_STATUS_PENDING);
				PostMessage( B_QUIT_REQUESTED);
				break;
			}
			case BM_EDIT_HEADER_DONE: {
				// User is done with editing the mail-header. We reconstruct 
				// the mail with the new header and then send it:
				BmRef<BmMail> mail = mMailView->CurrMail();
				int32 result;
				const char* headerStr; 
				if (!mail || msg->FindInt32( "which", &result) != B_OK 
				|| msg->FindString( "entry_text", &headerStr) != B_OK 
				|| result != 1)
					break;
				BmRef<BmListModelItem> smtpRef 
					= TheSmtpAccountList->FindItemByKey( mail->AccountName());
				BmSmtpAccount* smtpAcc 
					= dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
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
				if (item) {
					BmRef<BmListModelItem> identRef 
						= TheIdentityList->FindItemByKey( item->Label());
					BmIdentity* ident 
						= dynamic_cast< BmIdentity*>( identRef.Get()); 
					if (!ident)
						break;
					TheIdentityList->CurrIdentity( ident);
					BmString fromString = ident->GetFromAddress();
					mFromControl->SetText( fromString.String());
					mFromControl->TextView()->Select( fromString.Length(), 
																 fromString.Length());
					mFromControl->TextView()->ScrollToSelection();
					// mark selected identity:
					mFromControl->Menu()->MarkItem( ident->Key().String());
					// select corresponding smtp-account, if any:
					mSmtpControl->MarkItem( ident->SMTPAccount().String());
					// update signature:
					BmString sigName = ident->SignatureName();
					mMailView->SetSignatureByName( sigName);
					if (sigName.Length())
						mSignatureControl->MarkItem( sigName.String());
					else
						mSignatureControl->MarkItem( BM_NoItemLabel.String());
				}
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
		Regexx rx;
		BmString currStr = cntrl->Text();
		BmAddress addr( email);
		BmString mailAddr;
		if (ThePrefs->GetBool( "AddPeopleNameToMailAddr", true))
			mailAddr = addr.AddrString();
		else
			mailAddr = addr.AddrSpec();
		if (rx.exec( currStr, "\\S+"))
			currStr << ", " << mailAddr;
		else
			currStr << mailAddr;
		cntrl->SetText( currStr.String());
		cntrl->TextView()->Select( currStr.Length(), currStr.Length());
		cntrl->TextView()->ScrollToSelection();
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
							// false=>sync (i.e. wait till mail is being displayed)
	BmRef<BmMail> mail = mMailView->CurrMail();
	SetFieldsFromMail( mail.Get());
}

/*------------------------------------------------------------------------------*\
	EditMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::EditMail( BmMail* mail) {
	mMailView->ShowMail( mail, false);
							// false=>sync (i.e. wait till mail is being displayed)
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
		BmString fromAddrSpec;
		if (mail->IsRedirect()) {
			mBccControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_RESENT_BCC).String());
			mCcControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_RESENT_CC).String());
			mFromControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_RESENT_FROM).String());
			mSenderControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_RESENT_SENDER).String());
			mToControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_RESENT_TO).String());
			fromAddrSpec 
				= mail->Header()->GetAddressList( BM_FIELD_RESENT_FROM)
																.FirstAddress().AddrSpec();
		} else {
			mBccControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_BCC).String());
			mCcControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_CC).String());
			mFromControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_FROM).String());
			mSenderControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_SENDER).String());
			mToControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_TO).String());
			mReplyToControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_REPLY_TO).String());
			fromAddrSpec 
				= mail->Header()->GetAddressList( BM_FIELD_FROM)
																.FirstAddress().AddrSpec();
		}
		mSubjectControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_SUBJECT).String());
		SetTitle( (BmString("Edit Mail: ") + mSubjectControl->Text()).String());
		// mark corresponding identity:
		BmRef<BmIdentity> identRef 
			= TheIdentityList->FindIdentityForAddrSpec( fromAddrSpec);
		if (identRef)
			mFromControl->Menu()->MarkItem( identRef->Key().String());
		// mark corresponding SMTP-account (if any):
		BmString smtpAccount = mail->AccountName();
		mSmtpControl->MarkItem( smtpAccount.String());
		// mark signature of current mail as selected:
		BmString sigName = mail->SignatureName();
		if (sigName.Length())
			mSignatureControl->MarkItem( sigName.String());
		else
			mSignatureControl->MarkItem( BM_NoItemLabel.String());
		// mark corresponding charset:
		BmString charset = mail->DefaultCharset();
		charset.ToLower();
		mCharsetControl->MenuItem()->SetLabel( charset.String());
		mCharsetControl->MarkItem( charset.String());
		if (ThePrefs->GetBool( "ImportExportTextAsUtf8", true))
			mMailView->BodyPartView()->DefaultCharset( "utf-8");
		else
			mMailView->BodyPartView()->DefaultCharset( charset);
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
		if (BmString(mCcControl->Text()).Length() 
		|| BmString(mBccControl->Text()).Length()) {
			SetDetailsButton( 1, B_CONTROL_ON);
		}
		if (BmString(mBccControl->Text()).Length()) {
			SetDetailsButton( 2, B_CONTROL_ON);
		}
	}
}

/*------------------------------------------------------------------------------*\
	CreateMailFromFields()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::CreateMailFromFields( bool hardWrapIfNeeded) {
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		BmString editedText;
		mMailView->GetWrappedText( editedText, hardWrapIfNeeded);
		BmString charset = mCharsetControl->MenuItem()->Label();
		if (!charset.Length())
			charset = ThePrefs->GetString( "DefaultCharset");
		BMenuItem* smtpItem = mSmtpControl->Menu()->FindMarked();
		BmString smtpAccount = smtpItem ? smtpItem->Label() : "";
		mail->AccountName( smtpAccount);
		BMenuItem* identItem = mFromControl->Menu()->FindMarked();
		if (identItem)
			mail->IdentityName( identItem->Label());
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
		if (!mail->IsRedirect() 
		&& ThePrefs->GetBool( "SetMailDateWithEverySave", true)) {
			mail->SetFieldVal( BM_FIELD_DATE, 
									 TimeToString( time( NULL), 
														"%a, %d %b %Y %H:%M:%S %z"));
		}
		try {
			bool res = mail->ConstructRawText( editedText, charset, smtpAccount);
			return res;
		} catch( BM_text_error& textErr) {
			if (textErr.posInText >= 0) {
				BTextView* textView;
				int32 end = 1+textErr.posInText;
				if (textErr.context==BM_FIELD_SUBJECT)
					textView = mSubjectControl->TextView();
				else if (textErr.context==BM_FIELD_FROM)
					textView = mFromControl->TextView();
				else if (textErr.context==BM_FIELD_TO)
					textView = mToControl->TextView();
				else if (textErr.context==BM_FIELD_CC) {
					textView = mCcControl->TextView();
					if (!mShowDetails1)
						SetDetailsButton( 1, B_CONTROL_ON);
				} else if (textErr.context==BM_FIELD_BCC) {
					textView = mBccControl->TextView();
					if (!mShowDetails1)
						SetDetailsButton( 1, B_CONTROL_ON);
					if (!mShowDetails2)
						SetDetailsButton( 2, B_CONTROL_ON);
				} else
					textView = mMailView;
				while( IS_WITHIN_UTF8_MULTICHAR( textView->ByteAt( end)))
					end++;
				textView->Select( textErr.posInText, end);
				textView->ScrollToSelection();
				textView->MakeFocus( true);
			}
			ShowAlertWithType( textErr.what(), B_WARNING_ALERT);
			return false;
		}
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	SaveMail( saveForSend)
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::SaveMail( bool saveForSend) {
	if (!saveForSend && !mModified && !mHasNeverBeenSaved)
		return true;
	if (!CreateMailFromFields( saveForSend))
		return false;
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (mail) {
		mail->Outbound( true);				// just to make sure... >:o)
		if (saveForSend) {
			// set 'out'-folder as default and then start filter-job:
			mail->SetDestFoldername( BM_MAIL_FOLDER_OUT);
			mail->ApplyFilter();
		} else {
			// drop draft mails into 'draft'-folder:
			if (mail->Status() == BM_MAIL_STATUS_DRAFT)
				mail->SetDestFoldername( BM_MAIL_FOLDER_DRAFT);
		}
			
		if (mail->Store()) {
			mHasNeverBeenSaved = false;
			mModified = false;
			if (LockLooper()) {
				mSaveButton->SetEnabled( false);
				UnlockLooper();
			}
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	QuitRequested()
		-	standard BeOS-behaviour, we allow a quit
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::QuitRequested() {
	BM_LOG2( BM_LogGui, BmString("MailEditWin has been asked to quit"));
	if (mModified) {
		if (IsMinimized())
			Minimize( false);
		Activate();
		BAlert* alert = new BAlert( "title", 
											 "Save mail as draft before closing?",
											 "Cancel", "Don't Save", "Save",
											 B_WIDTH_AS_USUAL, B_OFFSET_SPACING, 
											 B_WARNING_ALERT);
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
				return SaveMail( false);
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
	BM_LOG2( BM_LogGui, BmString("MailEditWin has quit"));
	inherited::Quit();
}
