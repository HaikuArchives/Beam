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

#include <layout-all.h>

#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
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
BmRecvAccItem::BmRecvAccItem( BString key, BmListModelItem* _item)
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
			{ acc->POPServer().String(),				false },
			{ acc->PortNrString().String(),			true  },
			{ acc->CheckIntervalString().String(),	true  },
			{ acc->AuthMethod().String(),				false },
			{ acc->Username().String(),				false },
			{ acc->PwdStoredOnDisk() ? "*****":"",	false },
			{ acc->SMTPAccount().String(),			false },
			{ NULL, false }
		};
		SetTextCols( 0, cols, !ThePrefs->GetBool("StripedListView"));
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
																		BMessage* archive) {
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
		BM_SHOWERR( BString("RecvAccView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsRecvMailView
\********************************************************************************/

const BString BmPrefsRecvMailView::nEmptyItemLabel("<none>");

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsRecvMailView::BmPrefsRecvMailView() 
	:	inherited( "Receiving Mail-Accounts (POP3)")
{
	MView* view = 
		new VGroup(
			CreateAccListView( minimax(500,100,1E5,1E5), 500, 100),
			new HGroup(
				mAddButton = new MButton("Add Account", new BMessage(BM_ADD_ACCOUNT), this),
				mRemoveButton = new MButton("Remove Account", new BMessage( BM_REMOVE_ACCOUNT), this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Account Info",
					new VGroup(
						mAccountControl = new BmTextControl( "Account name:", false, 0, 30),
						mRealNameControl = new BmTextControl( "Real name:"),
						mMailAddrControl = new BmTextControl( "Mail address:"),
						mAliasesControl = new BmTextControl( "Aliases:"),
						new Space( minimax(0,5,0,5)),
						mServerControl = new BmTextControl( "Server:"),
						mPortControl = new BmTextControl( "Port:"),
						mAuthControl = new BmMenuControl( "Auth-method:", new BPopUpMenu("")),
						mLoginControl = new BmTextControl( "Login:"),
						mPwdControl = new BmTextControl( "Password:"),
						new Space( minimax(0,5,0,5)),
						mSignatureControl = new BmMenuControl( "Signature:", new BPopUpMenu("")),
						new Space( minimax(0,5,0,5)),
						mSmtpControl = new BmMenuControl( "SMTP-account:", new BPopUpMenu("")),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Options",
						new VGroup(
							mCheckAccountControl = new BmCheckControl( "Check mail", 
																					 new BMessage(BM_CHECK_MAIL_CHANGED), 
																					 this),
							new HGroup(
								mCheckEveryControl = new BmCheckControl( "Check every", 
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
							mIsBucketControl = new BmCheckControl( "This is a fallback account", 
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
	divider = MAX( divider, mPortControl->Divider());
	divider = MAX( divider, mPwdControl->Divider());
	divider = MAX( divider, mRealNameControl->Divider());
	divider = MAX( divider, mServerControl->Divider());
	divider = MAX( divider, mAuthControl->Divider());
	divider = MAX( divider, mSignatureControl->Divider());
	divider = MAX( divider, mSmtpControl->Divider());
	mAccountControl->SetDivider( divider);
	mAliasesControl->SetDivider( divider);
	mLoginControl->SetDivider( divider);
	mMailAddrControl->SetDivider( divider);
	mPortControl->SetDivider( divider);
	mPwdControl->SetDivider( divider);
	mRealNameControl->SetDivider( divider);
	mServerControl->SetDivider( divider);
	mAuthControl->SetDivider( divider);
	mSignatureControl->SetDivider( divider);
	mSmtpControl->SetDivider( divider);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsRecvMailView::~BmPrefsRecvMailView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::Initialize() {
	inherited::Initialize();

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
	mAccListView->StartJob( ThePopAccountList.Get());
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
	while( (item = mSmtpControl->Menu()->RemoveItem( (int32)0)))
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
	while( (item = mSignatureControl->Menu()->RemoveItem( (int32)0)))
		delete item;
	AddItemToMenu( mSignatureControl->Menu(), 
					   new BMenuItem( nEmptyItemLabel.String(), new BMessage( BM_SIGNATURE_SELECTED)), this);
	for( iter = TheSignatureList->begin(); iter != TheSignatureList->end(); ++iter) {
		BmSignature* sig = dynamic_cast< BmSignature*>( iter->second.Get());
		AddItemToMenu( mSignatureControl->Menu(), 
							new BMenuItem( sig->Key().String(), new BMessage( BM_SIGNATURE_SELECTED)), 
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
void BmPrefsRecvMailView::SaveData() {
	ThePopAccountList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::UndoChanges() {
	ThePopAccountList->Cleanup();
	ThePopAccountList->StartJobInThisThread();
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
				}
				break;
			}
			case BM_CHECK_MAIL_CHANGED: {
				if (mCurrAcc)
					mCurrAcc->CheckMail( mCheckAccountControl->Value());
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
				break;
			}
			case BM_REMOVE_MAIL_CHANGED: {
				if (mCurrAcc)
					mCurrAcc->DeleteMailFromServer( mRemoveMailControl->Value());
				break;
			}
			case BM_IS_BUCKET_CHANGED: {
				if (mCurrAcc)
					mCurrAcc->MarkedAsBitBucket( mIsBucketControl->Value());
				break;
			}
			case BM_IS_DEFAULT_CHANGED: {
				if (mCurrAcc) {
					bool val = mIsDefaultControl->Value();
					if (val)
						ThePopAccountList->SetDefaultAccount( mCurrAcc->Key());
					else
						mCurrAcc->MarkedAsDefault( false);
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
				break;
			}
			case BM_AUTH_SELECTED: {
				BMenuItem* item = mAuthControl->Menu()->FindMarked();
				if (item)
					mCurrAcc->AuthMethod( item->Label());
				else
					mCurrAcc->AuthMethod( "");
				break;
			}
			case BM_SMTP_SELECTED: {
				BMenuItem* item = mSmtpControl->Menu()->FindMarked();
				if (item && nEmptyItemLabel != item->Label())
					mCurrAcc->SMTPAccount( item->Label());
				else
					mCurrAcc->SMTPAccount( "");
				break;
			}
			case BM_SIGNATURE_SELECTED: {
				BMenuItem* item = mSignatureControl->Menu()->FindMarked();
				if (item && nEmptyItemLabel != item->Label())
					mCurrAcc->SignatureName( item->Label());
				else
					mCurrAcc->SignatureName( "");
				break;
			}
			case BM_ADD_ACCOUNT: {
				BString key( "new account");
				for( int32 i=1; ThePopAccountList->FindItemByKey( key); ++i) {
					key = BString("new account_")<<i;
				}
				ThePopAccountList->AddItemToList( new BmPopAccount( key.String(), ThePopAccountList.Get()));
				mAccountControl->MakeFocus( true);
				mAccountControl->TextView()->SelectAll();
				break;
			}
			case BM_REMOVE_ACCOUNT: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert = new BAlert( "Remove Mail-Account", 
														 (BString("Are you sure about removing the account <") << mCurrAcc->Name() << ">?").String(),
													 	 "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
													 	 B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_ACCOUNT), BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						ThePopAccountList->RemoveItemFromList( mCurrAcc.Get());
						mCurrAcc = NULL;
					}
				}
				break;
			}
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BString("PrefsView_") << Name() << ":\n\t" << err.what());
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
	mSmtpControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	mCheckAccountControl->SetEnabled( enabled);
	mCheckEveryControl->SetEnabled( enabled);
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
	return mAccListView->ContainerView();
}
