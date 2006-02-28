/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Alert.h>
#include <FilePanel.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <HGroup.h>
#include <LayeredGroup.h>
#include <MButton.h>
#include <MPopup.h>
#include <MStringView.h>
#include <Space.h>
#include <VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmNetEndpointRoster.h"
#include "BmRecvAccount.h"
#include "BmPrefs.h"
#include "BmPrefsSendMailView.h"
#include "BmPrefsWin.h"
#include "BmRosterBase.h"
#include "BmSmtp.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmSendAccItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_SERVER,
	COL_ENCRYPTION_TYPE,
	COL_AUTH_METHOD,
	COL_USER,
	COL_PWD,
	COL_DOMAIN,
	COL_PORT
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccItem::BmSendAccItem( ColumnListView* lv, 
										BmListModelItem* _item)
	:	inherited( lv, _item, false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccItem::~BmSendAccItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmSendAccItem::UpdateView( BmUpdFlags flags, bool redraw,
										  uint32 updColBitmap) {
	BmSendAcc* acc = ModelItem();
	if (flags & UPD_ALL) {
		const char* cols[] = {
			acc->Key().String(),
			acc->SMTPServer().String(),
			acc->EncryptionType().String(),
			acc->AuthMethod().String(),
			acc->Username().String(),
			acc->PwdStoredOnDisk() ? "*****":"",
			acc->DomainToAnnounce().String(),
			acc->PortNrString().String(),
			NULL
		};
		SetTextCols( 0, cols);
		updColBitmap = 0xFFFFFFFF;
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}



/********************************************************************************\
	BmSendAccView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccView::BmSendAccView( int32 width, int32 height)
	:	inherited( BRect(0,0,width-1,height-1), "Beam_SendAccView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true)
{
	int32 flags = CLV_SORT_KEYABLE;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;

	AddColumn( new CLVColumn( "Account", 80.0, flags, 50.0));
	AddColumn( new CLVColumn( "Server", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Encryption", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Auth-Method", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "User", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Pwd", 50.0, flags, 40.0));
	AddColumn( new CLVColumn( "Domain", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Port", 40.0, 0, 40.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmSendAccView::~BmSendAccView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSendAccView::CreateListViewItem( BmListModelItem* item,
																	BMessage*) {
	return new BmSendAccItem( this, item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmSendAccView::AddModelItem( BmListModelItem* item) {
	BmListViewItem* viewItem = inherited::AddModelItem( item);
	if (viewItem) {
		Select( IndexOf(viewItem));
		ScrollToSelection();
	}
	return viewItem;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmSendAccView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("SendAccView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsSendMailView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSendMailView::BmPrefsSendMailView() 
	:	inherited( "Sending Mail-Accounts (SMTP)")
	,	mClientCertPanel( NULL)
{
	MView* view = 
		new VGroup(
			new BetterScrollView( 
				minimax(400,60,1E5,1E5), 
				mAccListView = new BmSendAccView( 400, 80),
				BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
				| BM_SV_CAPTION,
				"99 accounts"
			),
			new HGroup(
				mAddButton = new MButton( "Add Account", 
												  new BMessage(BM_ADD_ACCOUNT), 
												  this),
				mRemoveButton = new MButton( "Remove Account", 
													  new BMessage( BM_REMOVE_ACCOUNT), 
													  this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Account Info",
					new VGroup(
						mAccountControl = new BmTextControl( 
							"Account name:", false, 0, 25
						),
						new Space( minimax(0,5,0,5)),
						new HGroup( 
							mServerControl = new BmTextControl( "SMTP-Server / Port:"),
							mPortControl = new BmTextControl( "", false, 0, 8),
							0
						),
						new Space( minimax(0,5,0,5)),
						mEncryptionControl = new BmMenuControl( 
							"Encryption:", 
							new BPopUpMenu("")
						),
						new HGroup(
							mAuthControl = new BmMenuControl( 
								"Auth-method:", 
								new BPopUpMenu("")
							),
							mCheckAndSuggestButton = new MButton(
								"Check and Suggest", 
								new BMessage(BM_CHECK_AND_SUGGEST), 
								this, minimax(-1,-1,-1,-1)
							),
							0
						),
						mPopControl = new BmMenuControl( 
							"POP3-Account:", 
							new BmMenuController( 
								"POP3-Account:", 
								this, 
								new BMessage( BM_POP_SELECTED),
								&BmGuiRosterBase::RebuildRecvAccountMenu, 
								BM_MC_LABEL_FROM_MARKED | BM_MC_ADD_NONE_ITEM
							)
						),
						new HGroup( 
							mLoginControl = new BmTextControl( "User / Pwd:"),
							mPwdControl = new BmTextControl( "", false, 0, 8),
							0
						),
						new HGroup( 
							mClientCertControl 
								= new BmTextControl("Client Certificate:"),
							mSelectClientCertButton 
								= new MButton( 
									"Select"B_UTF8_ELLIPSIS, 
									new BMessage(BM_CLIENT_CERT_SELECTED), 
									this
								),
							0
						),
						new Space( minimax(0,5,0,5)),
						mDomainControl = new BmTextControl( "Domain to announce:"),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Options",
						new VGroup(
							mStorePwdControl = new BmCheckControl( 
								"Store password on disk (UNSAFE!)", 
								new BMessage(BM_PWD_STORED_CHANGED), 
								this
							),
							new Space( minimax(250,0,250,0)),
							0
						)
					),
					new Space(),
					0
				),
				0
			),
			new Space(minimax(0,0,1E5,1E5,0.5)),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	mPwdControl->TextView()->HideTyping( true);

	BmDividable::DivideSame(
		mAccountControl,
		mDomainControl,
		mLoginControl,
		mClientCertControl,
		mServerControl,
		mEncryptionControl,
		mAuthControl,
		mPopControl,
		NULL
	);

	mPortControl->SetDivider( 15);
	mPortControl->ct_mpm.weight = 0.4;
	mPwdControl->SetDivider( 15);
	mPwdControl->ct_mpm.weight = 0.4;
	mSelectClientCertButton->ct_mpm = minimax(-1,-1,-1,-1,0);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsSendMailView::~BmPrefsSendMailView() {
	delete mClientCertPanel;
	TheBubbleHelper->SetHelp( mAccListView, NULL);
	TheBubbleHelper->SetHelp( mAccountControl, NULL);
	TheBubbleHelper->SetHelp( mDomainControl, NULL);
	TheBubbleHelper->SetHelp( mLoginControl, NULL);
	TheBubbleHelper->SetHelp( mPwdControl, NULL);
	TheBubbleHelper->SetHelp( mServerControl, NULL);
	TheBubbleHelper->SetHelp( mPortControl, NULL);
	TheBubbleHelper->SetHelp( mEncryptionControl, NULL);
	TheBubbleHelper->SetHelp( mAuthControl, NULL);
	TheBubbleHelper->SetHelp( mPopControl, NULL);
	TheBubbleHelper->SetHelp( mCheckAndSuggestButton, NULL);
	TheBubbleHelper->SetHelp( mClientCertControl, NULL);
	TheBubbleHelper->SetHelp( mSelectClientCertButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mAccListView, 
		"This listview shows every SMTP-account you have defined."
	);
	TheBubbleHelper->SetHelp( 
		mAccountControl, 
		"Here you can enter a name for this SMTP-account.\n"
		"This name is used to identify this account in Beam."
	);
	TheBubbleHelper->SetHelp( 
		mDomainControl, 
		"Some SMTP-Servers check the domain announced by the client\n"
		"at session-start. This check will fail if the domain of the client-pc\n"
		"differs from the real internet-domain (usually the case if\n"
		"the client-pc has no permanent connection to the internet).\n\n"
		"If the SMTP-server rejects connections, you should try to enter\n"
		"your dial-in-provider's domain into this field, otherwise leave\n"
		"the field empty."
	);
	TheBubbleHelper->SetHelp( 
		mLoginControl, 
		"Here you can enter the username which \n"
		"will be used during authentication."
	);
	TheBubbleHelper->SetHelp( 
		mPwdControl, 
		"Here you can enter the password which \n"
		"will be used during authentication.\n"
		"(You can only edit this field if you \n"
		"checked 'Store Password on Disk')."
	);
	TheBubbleHelper->SetHelp( 
		mClientCertControl, 
		"Some servers require the user to authenticate\n"
		"with a PKCS#12-client-certificate (as part of the SSL/TLS-\n"
		"handshake). Here you can specify the file that contains\n"
		"the client certificate."
	);
	TheBubbleHelper->SetHelp( 
		mSelectClientCertButton, 
		"Pressing this button allows you to select the\n"
		"PKCS#12-file that is going to be used as the client\n"
		"certificate in order to authenticate to the server."
	);
	TheBubbleHelper->SetHelp( 
		mStorePwdControl, 
		"Checking this allows Beam to store the given \n"
		"password unsafely on disk.\n"
		"If you uncheck this, Beam will ask you for the password\n"
		"everytime you use this account."
	);
	TheBubbleHelper->SetHelp( 
		mServerControl, 
		"Please enter the full name of the SMTP-server \n"
		"into this field (e.g. 'mail.xxx.org')."
	);
	TheBubbleHelper->SetHelp( 
		mPortControl, 
		"Please enter the SMTP-port of the server \n"
		"into this field (usually 25)."
	);
	static BmString encrHelp;
	if (TheNetEndpointRoster->SupportsEncryption()) {
		encrHelp 
			= 		"Here you can select the type of encryption to use:\n"
					"<none>	- no encryption.";
		if (TheNetEndpointRoster->SupportsEncryptionType(BmSmtpAccount::ENCR_TLS))
			encrHelp 
				<< "\n"
				<<	"<auto>		- means the STARTTLS method will be used if available.\n"
					"STARTTLS	- TLS (Transport Layer Security) encryption on\n"
					"		  the standard SMTP-port (usually 25).\n"
					"TLS		- TLS (Transport Layer Security) encryption on\n"
					"		  a special SMTPS-port (usually 465).";
		if (TheNetEndpointRoster->SupportsEncryptionType(BmSmtpAccount::ENCR_SSL))
			encrHelp 
				<< "\n"
				<< "SSL		- SSL (Secure Socket Layer) encryption on\n"
					"		  a special SMTPS-port (usually 465).";
	} else {
		encrHelp = "Encryption is not available,\n"
					  "no addon could be loaded";
	}
	TheBubbleHelper->SetHelp( 
		mEncryptionControl, encrHelp.String()
	);
	TheBubbleHelper->SetHelp( 
		mAuthControl, 
		"Here you can select the authentication type to use:\n"
		"<auto>		- means the best (safest) available mode is used automatically.\n"
		"DIGEST-MD5	- is safe, neither password nor user are sent in cleartext.\n"
		"CRAM-MD5		- is safe, neither password nor user are sent in cleartext.\n"
		"PLAIN		- is a simple auth-mode which sends passwords in cleartext\n"
		"LOGIN		- is another simple auth-mode that sends passwords in cleartext\n"
		"SMTP-AFTER-POP	- does SMTP-authentication via the use of a POP3-server.\n"
		"<none> 		- is the simplest mode, no authentication at all."
	);
	TheBubbleHelper->SetHelp( 
		mPopControl, 
		"Here you can select the POP3-account that shall be used\n"
		"when authenticating via SMTP-AFTER-POP."
	);
	TheBubbleHelper->SetHelp( 
		mCheckAndSuggestButton, 
		"When you click here, Beam will connect to the SMTP-server,\n"
		"check which authentication types it supports and select\n"
		"the most secure."
	);

	mAccountControl->SetTarget( this);
	mDomainControl->SetTarget( this);
	mLoginControl->SetTarget( this);
	mPortControl->SetTarget( this);
	mPwdControl->SetTarget( this);
	mServerControl->SetTarget( this);
	mStorePwdControl->SetTarget( this);

	AddItemToMenu( mEncryptionControl->Menu(), 
						new BMenuItem( BM_NoItemLabel.String(), 
											new BMessage(BM_ENCRYPTION_SELECTED)), 
						this);
	if (TheNetEndpointRoster->SupportsEncryptionType(BmSmtpAccount::ENCR_TLS)) {
		AddItemToMenu( mEncryptionControl->Menu(), 
							new BMenuItem( BmSmtpAccount::ENCR_AUTO, 
												new BMessage(BM_ENCRYPTION_SELECTED)), 
							this);
		AddItemToMenu( mEncryptionControl->Menu(), 
							new BMenuItem( BmSmtpAccount::ENCR_STARTTLS, 
												new BMessage(BM_ENCRYPTION_SELECTED)), 
							this);
		AddItemToMenu( mEncryptionControl->Menu(), 
							new BMenuItem( BmSmtpAccount::ENCR_TLS, 
												new BMessage(BM_ENCRYPTION_SELECTED)), 
							this);
	}
	if (TheNetEndpointRoster->SupportsEncryptionType(BmSmtpAccount::ENCR_SSL)) {
		AddItemToMenu( mEncryptionControl->Menu(), 
							new BMenuItem( BmSmtpAccount::ENCR_SSL, 
												new BMessage(BM_ENCRYPTION_SELECTED)), 
							this);
	}

	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_AUTO, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_DIGEST_MD5, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_CRAM_MD5, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_PLAIN, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_LOGIN, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_SMTP_AFTER_POP, 
											new BMessage(BM_AUTH_SELECTED)),
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmSmtpAccount::AUTH_NONE, 
											new BMessage(BM_AUTH_SELECTED)), 
						this);

	mAccListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mAccListView->SetTarget( this);
	mAccListView->StartJob( TheSmtpAccountList.Get());
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::Activated() {
	inherited::Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsSendMailView::SanityCheck() {
	return DoSanityCheck( TheSmtpAccountList.Get(), Name());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::WriteStateInfo() {
	mAccListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::SaveData() {
	TheSmtpAccountList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::UndoChanges() {
	TheSmtpAccountList->Cleanup();
	TheSmtpAccountList->StartJobInThisThread();
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::EncryptionSelected() {
	if (!mCurrAcc)
		return;
	BMenuItem* item = mEncryptionControl->Menu()->FindMarked();
	BmString encryptionType;
	if (item && BM_NoItemLabel != item->Label()) {
		encryptionType = item->Label();
		mCurrAcc->EncryptionType( encryptionType);
	} else {
		mCurrAcc->EncryptionType( "");
	}
	BmString currPort = mPortControl->Text();
	if (encryptionType == BmSmtpAccount::ENCR_TLS 
	|| encryptionType == BmSmtpAccount::ENCR_SSL) {
		if (currPort == mCurrAcc->DefaultPort(false))
			// auto-switch to encrypted default-port:
			mPortControl->SetText(mCurrAcc->DefaultPort(true));
	} else {
		if (currPort == mCurrAcc->DefaultPort(true))
			// auto-switch to unencrypted default-port:
			mPortControl->SetText(mCurrAcc->DefaultPort(false));
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::AuthTypeSelected() {
	if (!mCurrAcc)
		return;
	BMenuItem* item = mAuthControl->Menu()->FindMarked();
	if (item)
		mCurrAcc->AuthMethod( item->Label());
	else
		mCurrAcc->AuthMethod( "");
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mAccListView->CurrentSelection( 0);
				ShowAccount( index);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				if (mCurrAcc) {
					BView* srcView = NULL;
					msg->FindPointer( "source", (void**)&srcView);
					BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
					if ( source == mAccountControl) {
						BmString oldName = mCurrAcc->Name();
						BmString newName = mAccountControl->Text();
						TheSmtpAccountList->RenameItem( oldName, newName);
						BmModelItemMap::const_iterator iter;
						// update any links to this smtp-account:
						BAutolock lock( TheIdentityList->ModelLocker());
						for( 	iter = TheIdentityList->begin(); 
								iter != TheIdentityList->end(); ++iter) {
							BmIdentity* ident 
								= dynamic_cast<BmIdentity*>( iter->second.Get());
							if (ident && ident->SMTPAccount()==oldName)
								ident->SMTPAccount( newName);
						}
					} else if ( source == mDomainControl)
						mCurrAcc->DomainToAnnounce( mDomainControl->Text());
					else if ( source == mLoginControl)
						mCurrAcc->Username( mLoginControl->Text());
					else if ( source == mPortControl)
						mCurrAcc->PortNr( atoi(mPortControl->Text()));
					else if ( source == mPwdControl)
						mCurrAcc->Password( mPwdControl->Text());
					else if ( source == mServerControl)
						mCurrAcc->SMTPServer( mServerControl->Text());
					else if ( source == mClientCertControl)
						mCurrAcc->ClientCertificate( mClientCertControl->Text());
					NoticeChange();
					UpdateState();
				}
				break;
			}
			case BM_PWD_STORED_CHANGED: {
				if (mCurrAcc) {
					mCurrAcc->PwdStoredOnDisk( mStorePwdControl->Value());
					NoticeChange();
				}
				UpdateState();
				break;
			}
			case BM_ENCRYPTION_SELECTED: {
				EncryptionSelected();
				NoticeChange();
				UpdateState();
				break;
			}
			case BM_AUTH_SELECTED: {
				AuthTypeSelected();
				NoticeChange();
				UpdateState();
				break;
			}
			case BM_CHECK_AND_SUGGEST: {
				if (mCurrAcc) {
					// ToDo: this should be made asynchronous (using messages):
					BmRef<BmSmtp> smtp( new BmSmtp( mCurrAcc->Key(), 
															  mCurrAcc.Get()));
					smtp->StartJobInThisThread( BmSmtp::BM_CHECK_CAPABILITIES_JOB);
					BmString suggestedAuthType = smtp->SuggestAuthType();
					if (suggestedAuthType.Length())
						mAuthControl->MarkItem( suggestedAuthType.String());
					else
						mAuthControl->MarkItem( BM_NoItemLabel.String());
					AuthTypeSelected();
					if (smtp->SupportsTLS())
						mEncryptionControl->MarkItem( BmSmtpAccount::ENCR_STARTTLS);
					else
						mEncryptionControl->MarkItem( BM_NoItemLabel.String());
					EncryptionSelected();
					NoticeChange();
					UpdateState();
				}
				break;
			}
			case BM_CLIENT_CERT_SELECTED: {
				entry_ref certRef;
				if (msg->FindRef( "refs", 0, &certRef) != B_OK) {
					// first step, let user select new certificate:
					if (!mClientCertPanel) {
						BmString certPath = TheNetEndpointRoster->GetCertPath();
						entry_ref eref;
						status_t err = get_ref_for_path(certPath.String(), &eref);
						mClientCertPanel = new BFilePanel( 
							B_OPEN_PANEL, new BMessenger(this), 
							err == B_OK ? &eref : NULL, B_FILE_NODE, false, msg
						);
					}
					mClientCertPanel->Show();
				} else {
					// second step, set data accordingly:
					mCurrAcc->ClientCertificate(certRef.name);
					mClientCertControl->SetText(certRef.name);
					NoticeChange();
					UpdateState();
				}
				break;
			}
			case BM_POP_SELECTED: {
				BMenuItem* item = mPopControl->Menu()->FindMarked();
				if (item && BM_NoItemLabel != item->Label())
					mCurrAcc->AccForSmtpAfterPop( item->Label());
				else
					mCurrAcc->AccForSmtpAfterPop( "");
				NoticeChange();
				UpdateState();
				break;
			}
			case BM_ADD_ACCOUNT: {
				BmString key( "new account");
				for( int32 i=1; TheSmtpAccountList->FindItemByKey( key); ++i) {
					key = BmString("new account_")<<i;
				}
				TheSmtpAccountList->AddItemToList( new BmSmtpAccount( key.String(), 
															  TheSmtpAccountList.Get()));
				mAccountControl->MakeFocus( true);
				mAccountControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_ACCOUNT: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( 
						"Remove Mail-Account", 
					 	(BmString("Are you sure about removing the account <") 
					 		<< mCurrAcc->Name() << ">?").String(),
						"Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
						B_WARNING_ALERT
					);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_ACCOUNT), 
													 BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheSmtpAccountList->RemoveItemFromList( mCurrAcc.Get());
						mCurrAcc = NULL;
						NoticeChange();
					}
				}
				break;
			}
			case BM_COMPLAIN_ABOUT_FIELD: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					BmString complaint;
					complaint = msg->FindString( MSG_COMPLAINT);
					// first step, tell user about complaint:
					BAlert* alert = new BAlert( "Sanity Check Failed", 
														 complaint.String(),
													 	 "OK", NULL, NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 0, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(*msg), BMessenger( this)));
					BmSmtpAccount* acc=NULL;
					msg->FindPointer( MSG_ITEM, (void**)&acc);
					BmListViewItem* accItem = mAccListView->FindViewItemFor( acc);
					if (accItem)
						mAccListView->Select( mAccListView->IndexOf( accItem));
				} else {
					// second step, set corresponding focus:
					BmString fieldName;
					fieldName = msg->FindString( MSG_FIELD_NAME);
					if (fieldName.ICompare( "username")==0)
						mLoginControl->MakeFocus( true);
					else if (fieldName.ICompare( "smtpserver")==0)
						mServerControl->MakeFocus( true);
					else if (fieldName.ICompare( "pop-account")==0)
						mPopControl->MakeFocus( true);
					else if (fieldName.ICompare( "portnr")==0)
						mPortControl->MakeFocus( true);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::ShowAccount( int32 selection) {
	if (selection == -1) {
		mCurrAcc = NULL;
		mAccountControl->SetTextSilently( "");
		mDomainControl->SetTextSilently( "");
		mLoginControl->SetTextSilently( "");
		mPortControl->SetTextSilently( "");
		mPwdControl->SetTextSilently( "");
		mServerControl->SetTextSilently( "");
		mEncryptionControl->ClearMark();
		mAuthControl->ClearMark();
		mPopControl->ClearMark();
		mStorePwdControl->SetValue( 0);
		mClientCertControl->SetTextSilently( "");
	} else {
		BmSendAccItem* accItem 
			= dynamic_cast<BmSendAccItem*>(mAccListView->ItemAt( selection));
		if (accItem) {
			if  (mCurrAcc != accItem->ModelItem()) {
				mCurrAcc = dynamic_cast<BmSmtpAccount*>(accItem->ModelItem());
				if (mCurrAcc) {
					mAccountControl->SetTextSilently( mCurrAcc->Name().String());
					mDomainControl->SetTextSilently( 
						mCurrAcc->DomainToAnnounce().String());
					mLoginControl->SetTextSilently( mCurrAcc->Username().String());
					mPortControl->SetTextSilently( 
						mCurrAcc->PortNrString().String());
					mPwdControl->SetTextSilently( mCurrAcc->Password().String());
					mServerControl->SetTextSilently( 
						mCurrAcc->SMTPServer().String());
					mEncryptionControl->MarkItem( 
						(mCurrAcc->EncryptionType().Length() 
						&& TheNetEndpointRoster->SupportsEncryption())
								? mCurrAcc->EncryptionType().String()
								: BM_NoItemLabel.String()
					);
					mAuthControl->MarkItem( mCurrAcc->AuthMethod().Length() 
														? mCurrAcc->AuthMethod().String()
														: BM_NoItemLabel.String());
					mPopControl->MarkItem( 
						mCurrAcc->AccForSmtpAfterPop().Length()
							? mCurrAcc->AccForSmtpAfterPop().String()
							: BM_NoItemLabel.String());
					mStorePwdControl->SetValue( mCurrAcc->PwdStoredOnDisk());
					mClientCertControl->SetTextSilently( 
						mCurrAcc->ClientCertificate().String()
					);
				}
			}
		} else
			mCurrAcc = NULL;
	}
	UpdateState();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsSendMailView::UpdateState() {
	bool accSelected = mCurrAcc ? true : false;

	mAccountControl->SetEnabled( accSelected);
	mDomainControl->SetEnabled( accSelected);
	mPortControl->SetEnabled( accSelected);
	mServerControl->SetEnabled( accSelected);
	if (TheNetEndpointRoster->SupportsEncryption())
		mEncryptionControl->SetEnabled( accSelected);
	else
		mEncryptionControl->SetEnabled( false);
	mAuthControl->SetEnabled( accSelected);
	mCheckAndSuggestButton->SetEnabled( accSelected);
	mRemoveButton->SetEnabled( accSelected);
	mStorePwdControl->SetEnabled( accSelected);
	mClientCertControl->SetEnabled( accSelected);
	mSelectClientCertButton->SetEnabled( accSelected);

	if (!accSelected) {
		mPopControl->SetEnabled( false);
		mLoginControl->SetEnabled( false);
		mPwdControl->SetEnabled( false);
	} else {
		mPopControl->SetEnabled( mCurrAcc->NeedsAuthViaPopServer());
		mLoginControl->SetEnabled( mCurrAcc->AuthMethod().Length() 
											&& !mCurrAcc->NeedsAuthViaPopServer());
		bool pwdEnabled = mCurrAcc->PwdStoredOnDisk() 
								&& mCurrAcc->AuthMethod().Length()
								&& !mCurrAcc->NeedsAuthViaPopServer();
		mPwdControl->SetEnabled( pwdEnabled);
		if (!pwdEnabled)
			mPwdControl->SetTextSilently("");
	}
}
