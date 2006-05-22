/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <cctype>

#include <FilePanel.h>
#include <MenuItem.h>
#include <MessageFilter.h>

#include "BmString.h"

#include <HGroup.h>
#include <VGroup.h>
#include <MBorder.h>
#include <MMenuBar.h>
#include <Space.h>

#include "TextEntryAlert.h"

#include "BeamApp.h"
#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmBodyPartView.h"
#include "BmCheckControl.h"
#include "BmDataModel.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFolder.h"
#include "BmMailEditWin.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmMsgTypes.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmTextControl.h"
#include "BmMailAddrCompleter.h"
#include "BmToolbarButton.h"

/*------------------------------------------------------------------------------*\
	SelectEmailForPerson( emails)
		-	selects one of the given emails of a person
\*------------------------------------------------------------------------------*/
BmString SelectEmailForPerson( const BmStringVect& emails) 
{
	if (emails.empty())
		return BM_DEFAULT_STRING;
	// maybe TODO: ask user which of the given addresses should be used.
	// However, this would be a major usability drawback, as this method is
	// triggered by selecting one or more people files and opening them with / 
	// dragging them over Beam. That's why we currently select the first mail
	// automatically:
	return emails[0];
}

/*------------------------------------------------------------------------------*\
	NoteOutboundAddresses()
		-	
\*------------------------------------------------------------------------------*/
static void NoteOutboundAddresses( const BmAddressList& toList,
											  const BmAddressList& ccList,
											  const BmAddressList& bccList) 
{
	BmAddrList::const_iterator iter;
	for( iter=toList.begin(); iter != toList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		ThePeopleList->AddAsKnownAddress(iter->AddrSpec());
	}
	for( iter=ccList.begin(); iter != ccList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		ThePeopleList->AddAsKnownAddress(iter->AddrSpec());
	}
	for( iter=bccList.begin(); iter != bccList.end(); ++iter) {
		if (!iter->HasAddrSpec())
			// empty group-addresses have no real address-specification 
			// (like 'Undisclosed-Recipients: ;'), we filter those:
			continue;
		ThePeopleList->AddAsKnownAddress(iter->AddrSpec());
	}
}

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

#include <map>
typedef map< BmString, BmMailEditWin*> BmEditWinMap;
static BmEditWinMap nEditWinMap;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
class BmShiftTabMsgFilter : public BMessageFilter {
public:
	BmShiftTabMsgFilter( BControl* stControl, uint32 cmd)
		: 	BMessageFilter( B_ANY_DELIVERY, B_ANY_SOURCE, cmd) 
		,	mShiftTabToControl( stControl)
	{
	}
	filter_result Filter( BMessage* msg, BHandler** handler);
private:
	BControl* mShiftTabToControl;
};

BMessageFilter* CreateShiftTabMsgFilter(BControl* stControl, uint32 cmd)
{
	return new BmShiftTabMsgFilter(stControl, cmd);
}



/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
class BmPeopleDropMsgFilter : public BMessageFilter {
public:
	BmPeopleDropMsgFilter(uint32 cmd)
		: 	BMessageFilter( B_DROPPED_DELIVERY, B_ANY_SOURCE, cmd) 
	{
	}
	filter_result Filter( BMessage* msg, BHandler** handler);
};

BMessageFilter* CreatePeopleDropMsgFilter(uint32 cmd)
{
	return new BmPeopleDropMsgFilter(cmd);
}

/*------------------------------------------------------------------------------*\
	Filter()
		-	
\*------------------------------------------------------------------------------*/
filter_result BmShiftTabMsgFilter::Filter( BMessage* msg, BHandler**) {
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
filter_result BmPeopleDropMsgFilter::Filter( BMessage* msg, 
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
				BmStringVect emails;
				ThePeopleList->GetEmailsFromPeopleFile( eref, emails);
				BmString email = SelectEmailForPerson( emails);
				win->AddAddressToTextControl( 
					dynamic_cast< BmTextControl*>( cntrl), email
				);
				res = B_SKIP_MESSAGE;
			}
		}
	}
	return res;
}


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
	new BmMailAddressCompleter( mToControl);
	new BmMailAddressCompleter( mCcControl);
	new BmMailAddressCompleter( mBccControl);
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
		BRect scrFrame = beamApp->ScreenFrame();
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
	BmToolbarButton::CalcMaxSize( width, height, "Send", true);
	BmToolbarButton::CalcMaxSize( width, height, "Save");
	BmToolbarButton::CalcMaxSize( width, height, "New");
	BmToolbarButton::CalcMaxSize( width, height, "Attach"B_UTF8_ELLIPSIS);

	BBitmap* rightArrow = TheResources->IconByName("Expander_Right")->bitmap;
	BBitmap* downArrow = TheResources->IconByName("Expander_Down")->bitmap;
	
	mOuterGroup = 
		new VGroup(
			minimax( 200, 300, 1E5, 1E5),
			CreateMenu(),
			mToolbar = new BmToolbar(
				new HGroup(
					minimax( -1, -1, 1E5, -1),
					mSendButton 
						= new BmToolbarButton( 
									"Send", 
									width, height,
									new BMessage(BMM_SEND_NOW), this, 
									"Send mail now", true
								),
					mSaveButton 
						= new BmToolbarButton( 
									"Save", 
									width, height,
									new BMessage(BMM_SAVE), this, 
									"Save mail as draft (for later use)"
								),
					mNewButton 
						= new BmToolbarButton( 
									"New", 
									width, height,
									new BMessage(BMM_NEW_MAIL), this, 
									"Compose a new mail message"
								),
					mAttachButton 
						= new BmToolbarButton( 
									"Attach"B_UTF8_ELLIPSIS, 
									width, height,
									new BMessage(BMM_ATTACH), this, 
									"Attach a file to this mail",
									false, "Attachment"
								),
					new BmToolbarSpace(),
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
												 &BmGuiRosterBase::RebuildIdentityMenu,
												 BM_MC_RADIO_MODE)
				),
				mCharsetControl = new BmMenuControl( 
					"Charset:", 
					new BmMenuController( "", this, 
												 new BMessage( BM_CHARSET_SELECTED), 
												 &BmGuiRosterBase::RebuildCharsetMenu, 
												 BM_MC_LABEL_FROM_MARKED),
					0.5
				),
				0
			),
			new HGroup(
				mShowDetails1Button = 
					new MPictureButton( 
						minimax( 16,16,16,16), 
						TheResources->CreatePictureFor( rightArrow, 16, 16), 
						TheResources->CreatePictureFor( downArrow, 16, 16), 
						new BMessage( BM_SHOWDETAILS1), this, B_TWO_STATE_BUTTON
					),
				new Space(minimax(4,-1,4,-1)),
				mToControl = new BmTextControl( 
					"To:", 
					new BmMenuController( "To:", this, 
												 new BMessage( BM_TO_ADDED), 
												 &BmGuiRosterBase::RebuildPeopleMenu)
				),
				0
			),
			mDetails1Group = new VGroup(
				new HGroup(
					mShowDetails2Button = 
						new MPictureButton( minimax( 16,16,16,16), 
							TheResources->CreatePictureFor( rightArrow, 16, 16),
							TheResources->CreatePictureFor( downArrow, 16, 16), 
							new BMessage( BM_SHOWDETAILS2), this, B_TWO_STATE_BUTTON
						),
					new Space(minimax(4,-1,4,-1)),
					mCcControl = new BmTextControl( 
						"Cc:", 
						new BmMenuController( "Cc:", this, 
													 new BMessage( BM_CC_ADDED), 
													 &BmGuiRosterBase::RebuildPeopleMenu)
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
													 &BmGuiRosterBase::RebuildPeopleMenu)
					),
					mSenderControl = new BmTextControl( "Sender:", false),
					0
				),
				0
			),
			mSubjectGroup = new HGroup(
				mShowDetails3Button = 
					new MPictureButton( minimax( 16,16,16,16), 
						TheResources->CreatePictureFor( rightArrow, 16, 16), 
						TheResources->CreatePictureFor( downArrow, 16, 16), 
						new BMessage( BM_SHOWDETAILS3), this, B_TWO_STATE_BUTTON
					),
				new Space(minimax(4,-1,4,-1)),
				mSubjectControl = new BmTextControl( "Subject:", false),
				0
			),
			mDetails3Group = new VGroup(
				new HGroup(
					new Space(minimax(20,-1,20,-1)),
					mSignatureControl = new BmMenuControl( 
						"Signature:", 
						new BmMenuController( 
							"Signature:", this, 
							new BMessage( BM_SIGNATURE_SELECTED), 
							&BmGuiRosterBase::RebuildSignatureMenu, 
							BM_MC_ADD_NONE_ITEM | BM_MC_LABEL_FROM_MARKED
						)
					),
					new Space(minimax(20,-1,20,-1)),
					mSmtpControl = new BmMenuControl( 
						"SMTP-Server:", 
						new BmMenuController( "", this, 
													 new BMessage( BM_SMTP_SELECTED), 
													 &BmGuiRosterBase::RebuildSmtpAccountMenu, 
													 BM_MC_LABEL_FROM_MARKED),
						0.5
					),
					0
				),
				new HGroup(
					new Space(minimax(20,-1,20,-1)),
					mFileIntoControl = new BmMenuControl( 
						"Target Folder:",
						new BmMenuControllerBase( 
							"out", this, 
							new BMessage( BM_FILEINTO_SELECTED), 
							&BmGuiRosterBase::RebuildFolderMenu
						),
						2.0
					),
					new Space(minimax(20,-1,20,-1)),
					mEditHeaderControl = new BmCheckControl( "Edit Headers", 1, false),
					0
				),
				0
			),
			mSeparator = new Space(minimax(-1,4,-1,4)),
			new BmMailViewContainer(
				minimax(200,200,1E5,1E5),
				mMailView = BmMailView::CreateInstance( BRect(0,0,400,200), true)
			),
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
												& ~B_NAVIGABLE);
	mShowDetails2Button->SetFlags( mShowDetails2Button->Flags() 
												& ~B_NAVIGABLE);
	mShowDetails3Button->SetFlags( mShowDetails3Button->Flags()
												& ~B_NAVIGABLE);

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

	AddChild( dynamic_cast<BView*>(mOuterGroup));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::BeginLife() {
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

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailEditWin* FindMailEditWinFor( const BmString& key)
{
	BmEditWinMap::iterator pos = nEditWinMap.find( key);
	if (pos != nEditWinMap.end())
		return pos->second;
	else
		return NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void AddMailEditWin( const BmString& key, BmMailEditWin* win)
{
	nEditWinMap[key] = win;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void RemoveMailEditWin( BmMailEditWin* win)
{
	// we remove ourselves from the existing edit-windows map in a safe manner
	// (such as to not depend on the current key corresponding to the key
	// that was used when we were inserted into the map):
	BmEditWinMap::iterator iter;
	for( iter = nEditWinMap.begin(); iter != nEditWinMap.end(); ++iter) {
		if (iter->second == win) {
			// remove ourselves and quit:
			nEditWinMap.erase( iter);
			return;
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::RebuildPeopleMenu( BmMenuControllerBase* peopleMenu) {
	BmMailEditWin* editWin 
		= dynamic_cast< BmMailEditWin*>( peopleMenu->MsgTarget());
	if (!editWin)
		return;

	BMessage* msgTempl = peopleMenu->MsgTemplate();
	// add all adresses to menu and a menu-entry for clearing the field:
	ThePeopleList->AddPeopleToMenu( peopleMenu, *msgTempl,
											  BmListModel::MSG_ITEMKEY);

	peopleMenu->AddSeparatorItem();
	BMenu* removeMenu = NULL;
	BmRef<BmMail> currMail = editWin->CurrMail();
	if (currMail) {
		BmAddressList addrList;
		if (msgTempl->what == BM_TO_ADDED)
			addrList.Set( editWin->mToControl->Text());
		else if (msgTempl->what == BM_CC_ADDED)
			addrList.Set( editWin->mCcControl->Text());
		else
			addrList.Set( editWin->mBccControl->Text());
		BmAddrList::const_iterator iter;
		for( iter = addrList.begin(); iter != addrList.end(); ++iter) {
			if (!removeMenu) {
				BFont font;
				peopleMenu->GetFont( &font);
				removeMenu = new BMenu( "<Remove>");
				removeMenu->SetFont( &font);
			}
			BMessage* removeMsg;
			if (msgTempl->what == BM_TO_ADDED)
				removeMsg = new BMessage( BM_TO_REMOVE);
			else if (msgTempl->what == BM_CC_ADDED)
				removeMsg = new BMessage( BM_CC_REMOVE);
			else
				removeMsg = new BMessage( BM_BCC_REMOVE);
			removeMsg->AddString( MSG_ADDRESS, (*iter).AddrString().String());
			removeMenu->AddItem( new BMenuItem( (*iter).AddrString().String(), 
															removeMsg));
		}
		if (removeMenu)
			peopleMenu->AddItem( removeMenu);
	}
	// add menu-entry for clearing the field:
	BMessage* clearMsg;
	if (msgTempl->what == BM_TO_ADDED)
		clearMsg = new BMessage( BM_TO_CLEAR);
	else if (msgTempl->what == BM_CC_ADDED)
		clearMsg = new BMessage( BM_CC_CLEAR);
	else
		clearMsg = new BMessage( BM_BCC_CLEAR);
	peopleMenu->AddItem( new BMenuItem( "<Clear Field>", clearMsg));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailEditWin::EditHeaders( )
{
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (!mail)
		return false;
	if (!CreateMailFromFields( false))
		return false;
	// allow user to edit mail-header before we send it:
	BRect screen( beamApp->ScreenFrame());
	float w=600, h=400;
	BRect alertFrame( (screen.Width()-w)/2,
							(screen.Height()-h)/2,
							(screen.Width()+w)/2,
							(screen.Height()+h)/2);
	BmString headerStr;
	headerStr.ConvertLinebreaksToLF( 
		&mail->Header()->HeaderString()
	);
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
	int32 choice = alert->Go( headerStr);
	if (choice == 1) {
		mail->SetNewHeader( headerStr);
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SendMail( bool sendNow)
{
	if (mHasBeenSent)
		return;		// mail has already been sent, we do not repeat!
	BM_LOG2( BM_LogGui, BmString("MailEditWin: Asked to send mail"));
	if (!SaveMail( true))
		return;
	BM_LOG2( BM_LogGui, "MailEditWin: ...mail was saved");
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (!mail)
		return;
	if (mail->IsFieldEmpty( mail->IsRedirect() 
										? BM_FIELD_RESENT_FROM 
										: BM_FIELD_FROM)) {
		BM_SHOWERR("Please enter at least one address into the <FROM> "
					  "field before sending this mail, thank you.");
		return;
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
		return;
	}
	BmRef<BmListModelItem> smtpRef 
		= TheSmtpAccountList->FindItemByKey( mail->AccountName());
	BmSmtpAccount* smtpAcc 
		= dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
	if (!smtpAcc) {
		ShowAlertWithType( "Before you can send this mail, "
								 "you have to select the SMTP-Account "
								 "to use for sending it.",
								 B_INFO_ALERT);
		return;
	}
	mail->Send( sendNow);
	mHasBeenSent = true;
	PostMessage( B_QUIT_REQUESTED);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::HandleFromSet( const BmString& from) {
	BmRef<BmListModelItem> identRef 
		= TheIdentityList->FindItemByKey( from);
	BmIdentity* ident 
		= dynamic_cast< BmIdentity*>( identRef.Get()); 
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (!ident || !mail)
		return;
	TheIdentityList->CurrIdentity( ident);
	BmString fromAddr = ident->GetFromAddress();
	mail->SetupFromIdentityAndRecvAddr( ident, fromAddr);
	mMailView->SetSignatureByName( mail->SignatureName());
	SetFieldsFromMail( mail.Get(), true);
}

/*------------------------------------------------------------------------------*\
	SetFieldFromMail()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SetFieldsFromMail( BmMail* mail, bool onlyIdentityFields) 
{
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
			if (!onlyIdentityFields)
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
			if (!onlyIdentityFields)
				mToControl->SetTextSilently( 
								mail->GetFieldVal( BM_FIELD_TO).String());
			mReplyToControl->SetTextSilently( 
							mail->GetFieldVal( BM_FIELD_REPLY_TO).String());
			fromAddrSpec 
				= mail->Header()->GetAddressList( BM_FIELD_FROM)
																.FirstAddress().AddrSpec();
		}
		if (!onlyIdentityFields) {
			mSubjectControl->SetTextSilently( 
								mail->GetFieldVal( BM_FIELD_SUBJECT).String());
			SetTitle((BmString("Edit Mail: ")+mSubjectControl->Text()).String());
			// mark corresponding charset:
			BmString charset = mail->DefaultCharset();
			charset.ToLower();
			mCharsetControl->MenuItem()->SetLabel( charset.String());
			mCharsetControl->MarkItem( charset.String());
			if (ThePrefs->GetBool( "ImportExportTextAsUtf8", true))
				mMailView->BodyPartView()->DefaultCharset( "utf-8");
			else
				mMailView->BodyPartView()->DefaultCharset( charset);
		}

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
			NoteOutboundAddresses(
				mail->Header()->GetAddressList( BM_FIELD_RESENT_TO),
				mail->Header()->GetAddressList( BM_FIELD_RESENT_CC),
				mail->Header()->GetAddressList( BM_FIELD_RESENT_BCC)
			);
		} else {
			mail->SetFieldVal( BM_FIELD_BCC, mBccControl->Text());
			mail->SetFieldVal( BM_FIELD_CC, mCcControl->Text());
			mail->SetFieldVal( BM_FIELD_FROM, mFromControl->Text());
			mail->SetFieldVal( BM_FIELD_SENDER, mSenderControl->Text());
			mail->SetFieldVal( BM_FIELD_TO, mToControl->Text());
			mail->SetFieldVal( BM_FIELD_REPLY_TO, mReplyToControl->Text());
			NoteOutboundAddresses(
				mail->Header()->GetAddressList( BM_FIELD_TO),
				mail->Header()->GetAddressList( BM_FIELD_CC),
				mail->Header()->GetAddressList( BM_FIELD_BCC)
			);
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
			if (mail->DefaultCharset() != charset) {
				// charset has been changed by autodetection, 
				// we select the new one:
				charset = mail->DefaultCharset();
				mCharsetControl->MenuItem()->SetLabel( charset.String());
				mCharsetControl->MarkItem( charset.String());
			}
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
						BMM_PREFERENCES), beamApp);
	menu->AddSeparatorItem();
	menu->AddItem( CreateMenuItem( "Close", B_QUIT_REQUESTED));
	menu->AddSeparatorItem();
	AddItemToMenu( menu, CreateMenuItem( "Quit Beam", B_QUIT_REQUESTED), beamApp);
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
			// set selected folder as default and then start filter-job:
			BMenuItem* labelItem = mFileIntoControl->MenuItem();
			BmString destFolderName 
				= labelItem ? labelItem->Label() : BmMailFolder::OUT_FOLDER_NAME;
			mail->SetDestFolderName( destFolderName);
		} else {
			// drop draft mails into 'draft'-folder:
			if (mail->Status() == BM_MAIL_STATUS_DRAFT)
				mail->SetDestFolderName( BmMailFolder::DRAFT_FOLDER_NAME);
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

/*------------------------------------------------------------------------------*\
	CurrMail()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMailEditWin::CurrMail() const { 
	if (mMailView)
		return mMailView->CurrMail();
	else
		return NULL;
}

