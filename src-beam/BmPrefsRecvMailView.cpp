/*
	BmPrefsRecvMailView.cpp
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

#include <Alert.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include <liblayout/HGroup.h>
#include <liblayout/LayeredGroup.h>
#include <liblayout/MButton.h>
#include <liblayout/MPopup.h>
#include <liblayout/MStringView.h>
#include <liblayout/Space.h>
#include <liblayout/VGroup.h>

#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmCheckControl.h"
#include "BmFilter.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmPopper.h"
#include "BmPrefs.h"
#include "BmPrefsRecvMailView.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmRecvAccItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_CHECK,
	COL_DEFAULT,
	COL_DELETE,
	COL_REAL_NAME,
	COL_MAIL_ADDR,
	COL_MAIL_ALIASES,
	COL_BITBUCKET,
	COL_SIGNATURE,
	COL_FILTER,
	COL_SERVER,
	COL_PORT,
	COL_CHECK_INTERVAL,
	COL_AUTH_METHOD,
	COL_USER,
	COL_PWD,
	COL_SMTP_ACC
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvAccItem::BmRecvAccItem( const BmString& key, BmListModelItem* _item)
	:	inherited( key, _item, false)
{
	UpdateView( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvAccItem::~BmRecvAccItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRecvAccItem::UpdateView( BmUpdFlags flags) {
	inherited::UpdateView( flags);
	BmRecvAcc* acc = ModelItem();
	if (flags & UPD_ALL) {
		BmListColumn cols[] = {
			{ acc->Key().String(),						false },
			{ acc->CheckMail() ? "*" : "",			false },
			{ acc->MarkedAsDefault() ? "*" : "",	false },
			{ acc->DeleteMailFromServer() ? "*" : "",	false },
			{ acc->RealName().String(),				false },
			{ acc->MailAddr().String(),				false },
			{ acc->MailAliases().String(),			false },
			{ acc->MarkedAsBitBucket() ? "*" : "",	false },
			{ acc->SignatureName().String(),			false },
			{ acc->FilterName().String(),				false },
			{ acc->POPServer().String(),				false },
			{ acc->PortNrString().String(),			true  },
			{ acc->CheckIntervalString().String(),	true  },
			{ acc->AuthMethod().String(),				false },
			{ acc->Username().String(),				false },
			{ acc->PwdStoredOnDisk() ? "*****":"",	false },
			{ acc->SMTPAccount().String(),			false },
			{ NULL, false }
		};
		SetTextCols( 0, cols);
	}
}



/********************************************************************************\
	BmRecvAccView
\********************************************************************************/

BmRecvAccView* BmRecvAccView::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvAccView* BmRecvAccView::CreateInstance( minimax minmax, int32 width, int32 height) {
	if (theInstance)
		return theInstance;
	else 
		return theInstance = new BmRecvAccView( minmax, width, height);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvAccView::BmRecvAccView( minimax minmax, int32 width, int32 height)
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_RecvAccView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true, true, false)
{
	int32 flags = 0;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;

	Initialize( BRect( 0,0,width-1,height-1),
					B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
					B_FOLLOW_TOP_BOTTOM, true, true, true, B_FANCY_BORDER);

	AddColumn( new CLVColumn( "Account", 80.0, CLV_SORT_KEYABLE|flags, 50.0));
	AddColumn( new CLVColumn( "C", 20.0, CLV_SORT_KEYABLE|flags, 20.0, "(C)heck?"));
	AddColumn( new CLVColumn( "D", 20.0, CLV_SORT_KEYABLE|flags, 20.0, "(D)efault?"));
	AddColumn( new CLVColumn( "R", 20.0, CLV_SORT_KEYABLE|flags, 20.0, "(R)emove Mails from Server?"));
	AddColumn( new CLVColumn( "Real Name", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Mailaddress", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Aliases", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "F", 20.0, CLV_SORT_KEYABLE|flags, 20.0, "(F)allback Account?"));
	AddColumn( new CLVColumn( "Signature", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Filter", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Server", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Port", 40.0, flags, 40.0));
	AddColumn( new CLVColumn( "Interval", 40.0, flags, 40.0));
	AddColumn( new CLVColumn( "Auth-Method", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "User", 80.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "Pwd", 50.0, CLV_SORT_KEYABLE|flags, 40.0));
	AddColumn( new CLVColumn( "SMTP-Account", 80.0, CLV_SORT_KEYABLE|flags, 40.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvAccView::~BmRecvAccView() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmRecvAccView::CreateListViewItem( BmListModelItem* item,
																	BMessage*) {
	return new BmRecvAccItem( item->Key(), item);
}

/*------------------------------------------------------------------------------*\
	CreateContainer()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmRecvAccView::CreateContainer( bool horizontal, bool vertical, 
												  				  bool scroll_view_corner, 
												  				  border_style border, 
																  uint32 ResizingMode, 
																  uint32 flags) 
{
	return new BmCLVContainerView( fMinMax, this, ResizingMode, flags, horizontal, 
											 vertical, scroll_view_corner, border, mShowCaption,
											 mShowBusyView, 
											 be_plain_font->StringWidth(" 99 accounts "));
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmRecvAccView::AddModelItem( BmListModelItem* item) {
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
void BmRecvAccView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("RecvAccView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsRecvMailView
\********************************************************************************/

const BmString BmPrefsRecvMailView::nEmptyItemLabel("<none>");

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsRecvMailView::BmPrefsRecvMailView() 
	:	inherited( "Receiving Mail-Accounts (POP3)")
{
	MView* view = 
		new VGroup(
			CreateAccListView( minimax(400,100,1E5,1E5), 400, 100),
			new HGroup(
				mAddButton = new MButton("Add Account", new BMessage(BM_ADD_ACCOUNT), this),
				mRemoveButton = new MButton("Remove Account", new BMessage( BM_REMOVE_ACCOUNT), this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Account Info",
					new VGroup(
						mAccountControl = new BmTextControl( "Account name:", false, 0, 25),
						mRealNameControl = new BmTextControl( "Real name:"),
						mMailAddrControl = new BmTextControl( "Mail address:"),
						mAliasesControl = new BmTextControl( "Aliases:"),
						new Space( minimax(0,5,0,5)),
						new HGroup( 
							mServerControl = new BmTextControl( "POP-Server:"),
							mPortControl = new BmTextControl( "", false, 0, 8),
							0
						),
						new HGroup( 
							mAuthControl = new BmMenuControl( "Auth-method:", new BPopUpMenu("")),
							mCheckAndSuggestButton = new MButton("Check and Suggest", new BMessage(BM_CHECK_AND_SUGGEST), this, minimax(-1,-1,-1,-1)),
							0
						),
						new HGroup( 
							mLoginControl = new BmTextControl( "User/Pwd:"),
							mPwdControl = new BmTextControl( "", false, 0, 8),
							0
						),
						new Space( minimax(0,5,0,5)),
						mSignatureControl = new BmMenuControl( "Signature:", new BPopUpMenu("")),
						new Space( minimax(0,5,0,5)),
						mFilterControl = new BmMenuControl( "Filter:", new BPopUpMenu("")),
						new Space( minimax(0,5,0,5)),
						mSmtpControl = new BmMenuControl( "SMTP-account:", new BPopUpMenu("")),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Options",
						new VGroup(
							mCheckAccountControl = new BmCheckControl( "Include in Manual Check", 
																					 new BMessage(BM_CHECK_MAIL_CHANGED), 
																					 this),
							new HGroup(
								mCheckEveryControl = new BmCheckControl( "Check automatically every", 
																						 new BMessage(BM_CHECK_EVERY_CHANGED), 
																						 this),
								mCheckIntervalControl = new BmTextControl( "", false, 4),
								mMinutesLabel = new MStringView( "minutes"),
								new Space(),
								0
							),
							mRemoveMailControl = new BmCheckControl( "Remove mails from server", 
																				  new BMessage(BM_REMOVE_MAIL_CHANGED), 
																				  this),
							new Space( minimax(0,10,0,10)),
							mIsDefaultControl = new BmCheckControl( "Use this as default account", 
																				 new BMessage(BM_IS_DEFAULT_CHANGED), 
																				 this),
							mIsBucketControl = new BmCheckControl( "This is a catch-all account", 
																				new BMessage(BM_IS_BUCKET_CHANGED), 
																				this),
							new Space( minimax(0,10,0,10)),
							mStorePwdControl = new BmCheckControl( "Store password on disk (UNSAFE!)", 
																				new BMessage(BM_PWD_STORED_CHANGED), 
																				this),
							0
						)
					),
					new Space(),
					0
				),
				0
			),
			0
		);
	mGroupView->AddChild( dynamic_cast<BView*>(view));
	
	mPwdControl->TextView()->HideTyping( true);
	
	float divider = mAccountControl->Divider();
	divider = MAX( divider, mAliasesControl->Divider());
	divider = MAX( divider, mLoginControl->Divider());
	divider = MAX( divider, mMailAddrControl->Divider());
	divider = MAX( divider, mRealNameControl->Divider());
	divider = MAX( divider, mServerControl->Divider());
	divider = MAX( divider, mAuthControl->Divider());
	divider = MAX( divider, mSignatureControl->Divider());
	divider = MAX( divider, mSmtpControl->Divider());
	divider = MAX( divider, mFilterControl->Divider());
	mAccountControl->SetDivider( divider);
	mAliasesControl->SetDivider( divider);
	mLoginControl->SetDivider( divider);
	mMailAddrControl->SetDivider( divider);
	mRealNameControl->SetDivider( divider);
	mServerControl->SetDivider( divider);
	mAuthControl->SetDivider( divider);
	mSignatureControl->SetDivider( divider);
	mSmtpControl->SetDivider( divider);
	mFilterControl->SetDivider( divider);

	mPortControl->SetDivider( 15);
	mPortControl->ct_mpm.weight = 0.4;
	mPwdControl->SetDivider( 15);
	mPwdControl->ct_mpm.weight = 0.4;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsRecvMailView::~BmPrefsRecvMailView() {
	TheBubbleHelper.SetHelp( mAccListView, NULL);
	TheBubbleHelper.SetHelp( mAccountControl, NULL);
	TheBubbleHelper.SetHelp( mAliasesControl, NULL);
	TheBubbleHelper.SetHelp( mLoginControl, NULL);
	TheBubbleHelper.SetHelp( mMailAddrControl, NULL);
	TheBubbleHelper.SetHelp( mPwdControl, NULL);
	TheBubbleHelper.SetHelp( mRealNameControl, NULL);
	TheBubbleHelper.SetHelp( mServerControl, NULL);
	TheBubbleHelper.SetHelp( mPortControl, NULL);
	TheBubbleHelper.SetHelp( mCheckAccountControl, NULL);
	TheBubbleHelper.SetHelp( mCheckEveryControl, NULL);
	TheBubbleHelper.SetHelp( mCheckIntervalControl, NULL);
	TheBubbleHelper.SetHelp( mIsBucketControl, NULL);
	TheBubbleHelper.SetHelp( mIsDefaultControl, NULL);
	TheBubbleHelper.SetHelp( mRemoveMailControl, NULL);
	TheBubbleHelper.SetHelp( mStorePwdControl, NULL);
	TheBubbleHelper.SetHelp( mAuthControl, NULL);
	TheBubbleHelper.SetHelp( mSignatureControl, NULL);
	TheBubbleHelper.SetHelp( mSmtpControl, NULL);
	TheBubbleHelper.SetHelp( mFilterControl, NULL);
	TheBubbleHelper.SetHelp( mCheckAndSuggestButton, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper.SetHelp( mAccListView, "This listview shows every POP3-account you have defined.");
	TheBubbleHelper.SetHelp( mAccountControl, "Here you can enter a name for this POP3-account.\nThis name is used to identify this account in Beam.");
	TheBubbleHelper.SetHelp( mAliasesControl, "Some email-providers allow definition of aliases for mail-accounts.\n\
In this case mails addressed to any of the aliases will be delivered\n\
to the real account. If you have defined such aliases, you can enter them here.\n\n\
(This information is used by Beam when trying to determine the mail-account to use\n\
when replying to mails that were addressed to one of the aliases).");
	TheBubbleHelper.SetHelp( mLoginControl, "Here you can enter the username which \nwill be used during authentication.");
	TheBubbleHelper.SetHelp( mMailAddrControl, "Here you can define the mail-address this account will use.\nBeam creates the mail-address automatically, \nbut you can override this by specifying the address here.");
	TheBubbleHelper.SetHelp( mPwdControl, "Here you can enter the password which \nwill be used during authentication.\n(You can only edit this field if you checked 'Store Password on Disk').");
	TheBubbleHelper.SetHelp( mRealNameControl, "Please enter your real name here (e.g. 'Bob Meyer').");
	TheBubbleHelper.SetHelp( mServerControl, "Please enter the full name of the POP3-server \ninto this field (e.g. 'pop.xxx.org').");
	TheBubbleHelper.SetHelp( mPortControl, "Please enter the POP3-port of the server \ninto this field (usually 110).");
	TheBubbleHelper.SetHelp( mCheckAccountControl, "Check this if you want to check this account \nwhen pressing the 'Check'-button.");
	TheBubbleHelper.SetHelp( mCheckEveryControl, "Check this if you want to check this account \nin regular intervals.");
	TheBubbleHelper.SetHelp( mCheckIntervalControl, "Here you can enter the interval (in minutes)\n between automatic checks.");
	TheBubbleHelper.SetHelp( mIsBucketControl, "Check this if this account is a catch-all account, \n\
i.e. if this account receives all undeliverable mails\n\
for a specific domain.\n\
(Beam uses this information when trying to determine\n\
the account to use when replying to mails).");
	TheBubbleHelper.SetHelp( mIsDefaultControl, "Checking this makes Beam use this account \nas the default account.");
	TheBubbleHelper.SetHelp( mRemoveMailControl, "Checking this makes Beam remove each mail \nfrom the server after retrieving them.");
	TheBubbleHelper.SetHelp( mStorePwdControl, "Checking this allows Beam to store the given \n\
password unsafely on disk.\n\
If you uncheck this, Beam will ask you for the password\n\
everytime you use this account.");
	TheBubbleHelper.SetHelp( mAuthControl, "Here you can select the authentication type to use:\n\
POP3  -  is the standard mode, which sends passwords in cleartext\n\
APOP  -  is a somewhat safer mode, passwords are encrypted.");
	TheBubbleHelper.SetHelp( mSignatureControl, "Here you can select the signature to be used \n\
for every mail sent from this account.");
	TheBubbleHelper.SetHelp( mSmtpControl, "Here you can select the SMTP-account that shall be used\n\
to send mails from this account.");
	TheBubbleHelper.SetHelp( mFilterControl, "Here you can select the mail-filter to be used \n\
for every mail received through this account.");
	TheBubbleHelper.SetHelp( mCheckAndSuggestButton, "When you click here, Beam will connect to the POP-server,\n\
check which authentication types it supports and select\n\
the most secure.");

	mAccountControl->SetTarget( this);
	mAliasesControl->SetTarget( this);
	mLoginControl->SetTarget( this);
	mMailAddrControl->SetTarget( this);
	mPortControl->SetTarget( this);
	mPwdControl->SetTarget( this);
	mRealNameControl->SetTarget( this);
	mServerControl->SetTarget( this);
	mCheckAccountControl->SetTarget( this);
	mCheckEveryControl->SetTarget( this);
	mCheckIntervalControl->SetTarget( this);
	mIsBucketControl->SetTarget( this);
	mIsDefaultControl->SetTarget( this);
	mRemoveMailControl->SetTarget( this);
	mStorePwdControl->SetTarget( this);

	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmPopAccount::AUTH_POP3, new BMessage(BM_AUTH_SELECTED)), 
						this);
	AddItemToMenu( mAuthControl->Menu(), 
						new BMenuItem( BmPopAccount::AUTH_APOP, new BMessage(BM_AUTH_SELECTED)), 
						this);

	mAccListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mAccListView->SetTarget( this);
	mAccListView->StartJob( ThePopAccountList.Get(), false);
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::Activated() {
	inherited::Activated();

	// update all entries of SMTP-account-menu:
	BMenuItem* item;
	while( (item = mSmtpControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	AddItemToMenu( mSmtpControl->Menu(), 
					   new BMenuItem( nEmptyItemLabel.String(), new BMessage( BM_SMTP_SELECTED)), this);
	BmModelItemMap::const_iterator iter;
	for( iter = TheSmtpAccountList->begin(); iter != TheSmtpAccountList->end(); ++iter) {
		BmSmtpAccount* acc = dynamic_cast< BmSmtpAccount*>( iter->second.Get());
		AddItemToMenu( mSmtpControl->Menu(), 
							new BMenuItem( acc->Key().String(), new BMessage( BM_SMTP_SELECTED)), 
							this);
	}

	// update all entries of signature-menu:
	while( (item = mSignatureControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	AddItemToMenu( mSignatureControl->Menu(), 
					   new BMenuItem( nEmptyItemLabel.String(), new BMessage( BM_SIGNATURE_SELECTED)), this);
	for( iter = TheSignatureList->begin(); iter != TheSignatureList->end(); ++iter) {
		BmSignature* sig = dynamic_cast< BmSignature*>( iter->second.Get());
		AddItemToMenu( mSignatureControl->Menu(), 
							new BMenuItem( sig->Key().String(), new BMessage( BM_SIGNATURE_SELECTED)), 
							this);
	}

	// update all entries of filter-menu:
	while( (item = mFilterControl->Menu()->RemoveItem( (int32)0))!=NULL)
		delete item;
	AddItemToMenu( mFilterControl->Menu(), 
					   new BMenuItem( nEmptyItemLabel.String(), new BMessage( BM_FILTER_SELECTED)), this);
	AddItemToMenu( mFilterControl->Menu(), 
					   new BMenuItem( BM_DefaultItemLabel.String(), new BMessage( BM_FILTER_SELECTED)), this);
	for( iter = TheInboundFilterList->begin(); iter != TheInboundFilterList->end(); ++iter) {
		BmFilter* filter = dynamic_cast< BmFilter*>( iter->second.Get());
		AddItemToMenu( mFilterControl->Menu(), 
							new BMenuItem( filter->Key().String(), new BMessage( BM_FILTER_SELECTED)), 
							this);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::WriteStateInfo() {
	mAccListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsRecvMailView::SanityCheck() {
	if (!InitDone())
		return true;
	BmString complaint, fieldName;
	BMessage msg( BM_COMPLAIN_ABOUT_FIELD);
	BmModelItemMap::const_iterator iter;
	for( iter = ThePopAccountList->begin(); iter != ThePopAccountList->end(); ++iter) {
		BmPopAccount* acc = dynamic_cast<BmPopAccount*>( iter->second.Get());
		if (acc && !acc->SanityCheck( complaint, fieldName)) {
			msg.AddPointer( MSG_ITEM, (void*)acc);
			msg.AddString( MSG_COMPLAINT, complaint.String());
			if (fieldName.Length())
				msg.AddString( MSG_FIELD_NAME, fieldName.String());
			Looper()->PostMessage( &msg, this);
			return false;
		}
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::SaveData() {
	ThePopAccountList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::UndoChanges() {
	ThePopAccountList->ResetToSaved();
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::MessageReceived( BMessage* msg) {
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
						ThePopAccountList->RenameItem( mCurrAcc->Name(), mAccountControl->Text());
					} else if ( source == mAliasesControl)
						mCurrAcc->MailAliases( mAliasesControl->Text());
					else if ( source == mLoginControl)
						mCurrAcc->Username( mLoginControl->Text());
					else if ( source == mMailAddrControl)
						mCurrAcc->MailAddr( mMailAddrControl->Text());
					else if ( source == mPortControl)
						mCurrAcc->PortNr( atoi(mPortControl->Text()));
					else if ( source == mPwdControl)
						mCurrAcc->Password( mPwdControl->Text());
					else if ( source == mRealNameControl)
						mCurrAcc->RealName( mRealNameControl->Text());
					else if ( source == mServerControl)
						mCurrAcc->POPServer( mServerControl->Text());
					else if ( source == mCheckIntervalControl)
						mCurrAcc->CheckInterval( MAX( 0,atoi(mCheckIntervalControl->Text())));
					NoticeChange();
				}
				break;
			}
			case BM_CHECK_MAIL_CHANGED: {
				if (mCurrAcc) {
					mCurrAcc->CheckMail( mCheckAccountControl->Value());
					NoticeChange();
				}
				break;
			}
			case BM_CHECK_EVERY_CHANGED: {
				bool val = mCheckEveryControl->Value();
				mCheckIntervalControl->SetEnabled( val);
				if (!val) {
					mCheckIntervalControl->SetTextSilently( "");
					if (mCurrAcc)
						mCurrAcc->CheckInterval( 0);
				}
				mMinutesLabel->SetHighColor( val ? Black : BeInactiveGrey);
				mMinutesLabel->Invalidate();
				NoticeChange();
				break;
			}
			case BM_REMOVE_MAIL_CHANGED: {
				if (mCurrAcc)
					mCurrAcc->DeleteMailFromServer( mRemoveMailControl->Value());
				NoticeChange();
				break;
			}
			case BM_IS_BUCKET_CHANGED: {
				if (mCurrAcc)
					mCurrAcc->MarkedAsBitBucket( mIsBucketControl->Value());
				NoticeChange();
				break;
			}
			case BM_IS_DEFAULT_CHANGED: {
				if (mCurrAcc) {
					bool val = mIsDefaultControl->Value();
					if (val)
						ThePopAccountList->SetDefaultAccount( mCurrAcc->Key());
					else
						mCurrAcc->MarkedAsDefault( false);
					NoticeChange();
				}
				break;
			}
			case BM_PWD_STORED_CHANGED: {
				bool val = mStorePwdControl->Value();
				mPwdControl->SetEnabled( val);
				if (!val)
					mPwdControl->SetText("");
				if (mCurrAcc)
					mCurrAcc->PwdStoredOnDisk( val);
				NoticeChange();
				break;
			}
			case BM_CHECK_AND_SUGGEST: {
				if (mCurrAcc) {
					BmRef<BmPopper> popper( new BmPopper( mCurrAcc->Key(), mCurrAcc.Get()));
					popper->StartJobInThisThread( BmPopper::BM_CHECK_AUTH_TYPES_JOB);
					BmString suggestedAuthType = popper->SuggestAuthType();
					mAuthControl->MarkItem( suggestedAuthType.String());
					NoticeChange();
				}
				// yes, no break here, we want to proceed with updating the auth-menu...
			}
			case BM_AUTH_SELECTED: {
				BMenuItem* item = mAuthControl->Menu()->FindMarked();
				if (item)
					mCurrAcc->AuthMethod( item->Label());
				else
					mCurrAcc->AuthMethod( "");
				NoticeChange();
				break;
			}
			case BM_SMTP_SELECTED: {
				BMenuItem* item = mSmtpControl->Menu()->FindMarked();
				if (item && nEmptyItemLabel != item->Label())
					mCurrAcc->SMTPAccount( item->Label());
				else
					mCurrAcc->SMTPAccount( "");
				NoticeChange();
				break;
			}
			case BM_SIGNATURE_SELECTED: {
				BMenuItem* item = mSignatureControl->Menu()->FindMarked();
				if (item && nEmptyItemLabel != item->Label())
					mCurrAcc->SignatureName( item->Label());
				else
					mCurrAcc->SignatureName( "");
				NoticeChange();
				break;
			}
			case BM_FILTER_SELECTED: {
				BMenuItem* item = mFilterControl->Menu()->FindMarked();
				if (item && nEmptyItemLabel != item->Label())
					mCurrAcc->FilterName( item->Label());
				else
					mCurrAcc->FilterName( "");
				NoticeChange();
				break;
			}
			case BM_ADD_ACCOUNT: {
				BmString key( "new account");
				for( int32 i=1; ThePopAccountList->FindItemByKey( key); ++i) {
					key = BmString("new account_")<<i;
				}
				ThePopAccountList->AddItemToList( new BmPopAccount( key.String(), ThePopAccountList.Get()));
				mAccountControl->MakeFocus( true);
				mAccountControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_ACCOUNT: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Mail-Account", 
														 (BmString("Are you sure about removing the account <") << mCurrAcc->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_ACCOUNT), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						ThePopAccountList->RemoveItemFromList( mCurrAcc.Get());
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
					BmPopAccount* acc=NULL;
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
					else if (fieldName.ICompare( "popserver")==0)
						mServerControl->MakeFocus( true);
					else if (fieldName.ICompare( "portnr")==0)
						mPortControl->MakeFocus( true);
					else if (fieldName.ICompare( "checkinterval")==0)
						mCheckIntervalControl->MakeFocus( true);
					else if (fieldName.ICompare( "pwdstoredondisk")==0)
						mStorePwdControl->MakeFocus( true);
					else if (fieldName.ICompare( "authmethod")==0)
						mAuthControl->MakeFocus( true);
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("PrefsView_") << Name() << ":\n\t" << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::ShowAccount( int32 selection) {
	bool enabled = (selection != -1);
	mAccountControl->SetEnabled( enabled);
	mAliasesControl->SetEnabled( enabled);
	mLoginControl->SetEnabled( enabled);
	mMailAddrControl->SetEnabled( enabled);
	mPortControl->SetEnabled( enabled);
	mRealNameControl->SetEnabled( enabled);
	mServerControl->SetEnabled( enabled);
	mAuthControl->SetEnabled( enabled);
	mSignatureControl->SetEnabled( enabled);
	mFilterControl->SetEnabled( enabled);
	mSmtpControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	mCheckAccountControl->SetEnabled( enabled);
	mCheckEveryControl->SetEnabled( enabled);
	mCheckAndSuggestButton->SetEnabled( enabled);
	mRemoveMailControl->SetEnabled( enabled);
	mIsDefaultControl->SetEnabled( enabled);
	mIsBucketControl->SetEnabled( enabled);
	mStorePwdControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrAcc = NULL;
		mAccountControl->SetTextSilently( "");
		mAliasesControl->SetTextSilently( "");
		mLoginControl->SetTextSilently( "");
		mMailAddrControl->SetTextSilently( "");
		mPortControl->SetTextSilently( "");
		mPwdControl->SetTextSilently( "");
		mRealNameControl->SetTextSilently( "");
		mServerControl->SetTextSilently( "");
		mCheckIntervalControl->SetTextSilently( "");
		mAuthControl->ClearMark();
		mSignatureControl->ClearMark();
		mFilterControl->ClearMark();
		mSmtpControl->ClearMark();
		mCheckAccountControl->SetValue( 0);
		mCheckEveryControl->SetValue( 0);
		mRemoveMailControl->SetValue( 0);
		mIsDefaultControl->SetValue( 0);
		mIsBucketControl->SetValue( 0);
		mStorePwdControl->SetValue( 0);
		mPwdControl->SetEnabled( false);
		mCheckIntervalControl->SetEnabled( false);
		mMinutesLabel->SetHighColor( BeInactiveGrey);
	} else {
		BmRecvAccItem* accItem = dynamic_cast<BmRecvAccItem*>(mAccListView->ItemAt( selection));
		if (accItem) {
			mCurrAcc = dynamic_cast<BmPopAccount*>(accItem->ModelItem());
			if (mCurrAcc) {
				mAccountControl->SetTextSilently( mCurrAcc->Name().String());
				mAliasesControl->SetTextSilently( mCurrAcc->MailAliases().String());
				mLoginControl->SetTextSilently( mCurrAcc->Username().String());
				mMailAddrControl->SetTextSilently( mCurrAcc->MailAddr().String());
				mPortControl->SetTextSilently( mCurrAcc->PortNrString().String());
				mPwdControl->SetTextSilently( mCurrAcc->Password().String());
				mRealNameControl->SetTextSilently( mCurrAcc->RealName().String());
				mServerControl->SetTextSilently( mCurrAcc->POPServer().String());
				mAuthControl->MarkItem( mCurrAcc->AuthMethod().String());
				mSignatureControl->MarkItem( mCurrAcc->SignatureName().Length() 
															? mCurrAcc->SignatureName().String()
															: nEmptyItemLabel.String());
				mFilterControl->MarkItem( mCurrAcc->FilterName().Length() 
													? mCurrAcc->FilterName().String()
													: nEmptyItemLabel.String());
				mSmtpControl->MarkItem( mCurrAcc->SMTPAccount().Length() 
													? mCurrAcc->SMTPAccount().String()
													: nEmptyItemLabel.String());
				mCheckAccountControl->SetValue( mCurrAcc->CheckMail());
				mCheckEveryControl->SetValue( mCurrAcc->CheckInterval()>0 ? 1 : 0);
				mCheckIntervalControl->SetTextSilently( mCurrAcc->CheckIntervalString().String());
				mRemoveMailControl->SetValue( mCurrAcc->DeleteMailFromServer());
				mIsDefaultControl->SetValue( mCurrAcc->MarkedAsDefault());
				mIsBucketControl->SetValue( mCurrAcc->MarkedAsBitBucket());
				mStorePwdControl->SetValue( mCurrAcc->PwdStoredOnDisk());
				mPwdControl->SetEnabled( mCurrAcc->PwdStoredOnDisk());
				mCheckIntervalControl->SetEnabled( mCurrAcc->CheckInterval()>0);
				mMinutesLabel->SetHighColor( mCurrAcc->CheckInterval()>0 ? Black : BeInactiveGrey);
			}
		} else
			mCurrAcc = NULL;
	}
	mMinutesLabel->Invalidate();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsRecvMailView::CreateAccListView( minimax minmax, int32 width, int32 height) {
	mAccListView = BmRecvAccView::CreateInstance( minmax, width, height);
	mAccListView->ClickSetsFocus( true);
	return mAccListView->ContainerView();
}
