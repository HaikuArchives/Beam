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

#include <MenuItem.h>
#include <PopUpMenu.h>

#include <layout-all.h>

#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"

#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmPrefs.h"
#include "BmPrefsRecvMailView.h"
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
			{ (BString()<<acc->PortNr()).String(),	true },
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
	:	inherited( minmax, BRect(0,0,width-1,height-1), "Beam_FolderView", 
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

#define BM_PWD_STORED_CHANGED 'bmPS'

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsRecvMailView::BmPrefsRecvMailView() 
	:	inherited( "Receiving Mail (POP3)")
{
	MView* view = 
		new VGroup(
			CreateAccListView( minimax(500,100,1E5,1E5), 500, 100),
			new HGroup(
				mAddButton = new MButton("Add Account"),
				mRemoveButton = new MButton("Remove Account"),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 4, (char*)"Account Info",
					new VGroup(
						mAccountControl = new BmTextControl( "Account Name:"),
						mRealNameControl = new BmTextControl( "Real Name:"),
						mMailAddrControl = new BmTextControl( "Mail-Address:"),
						mAliasesControl = new BmTextControl( "Aliases:"),
						new Space( minimax(0,5,0,5)),
						mServerControl = new BmTextControl( "Servername:"),
						mPortControl = new BmTextControl( "Port:"),
						new HGroup( 
							mAuthControl = new BmMenuControl( "Auth-Method:", new BPopUpMenu("")),
							new Space(),
							0
						),
						mLoginControl = new BmTextControl( "Login:"),
						mPwdControl = new BmTextControl( "Password:"),
						new Space( minimax(0,5,0,5)),
						new HGroup( 
							mSignatureControl = new BmMenuControl( "Signature:", new BPopUpMenu("")),
							new Space(),
							0
						),
						new Space( minimax(0,5,0,5)),
						new HGroup( 
							mSmtpControl = new BmMenuControl( "SMTP-Account:", new BPopUpMenu("")),
							new Space(),
							0
						),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 4, (char*)"Options",
						new VGroup(
							mCheckAccountControl = new MCheckBox( "Check mail"),
							mRemoveMailControl = new MCheckBox( "Remove Mails from Server"),
							new Space( minimax(0,10,0,10)),
							mIsDefaultControl = new MCheckBox( "Use this as default account"),
							mIsBucketControl = new MCheckBox( "This is a fallback account"),
							new Space( minimax(0,10,0,10)),
							mStorePwdControl = new MCheckBox( "Store Password on Disk (UNSAFE!)", new BMessage(BM_PWD_STORED_CHANGED), this),
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

	mAuthControl->Menu()->AddItem( new BMenuItem( "POP3", 0));
	mAuthControl->Menu()->AddItem( new BMenuItem( "APOP", 0));
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
	mAccListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mAccListView->SetTarget( BMessenger( this));
	mAccListView->StartJob( ThePopAccountList.Get());
	ShowAccount( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsRecvMailView::Activated() {
	inherited::Activated();
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
	ThePopAccountList->StartJob();
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
			case BM_PWD_STORED_CHANGED: {
				mPwdControl->SetEnabled( mStorePwdControl->Value());
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
	mRemoveMailControl->SetEnabled( enabled);
	mIsDefaultControl->SetEnabled( enabled);
	mIsBucketControl->SetEnabled( enabled);
	mStorePwdControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mAccountControl->SetTextSilently( "");
		mAliasesControl->SetTextSilently( "");
		mLoginControl->SetTextSilently( "");
		mMailAddrControl->SetTextSilently( "");
		mPortControl->SetTextSilently( "");
		mPwdControl->SetTextSilently( "");
		mRealNameControl->SetTextSilently( "");
		mServerControl->SetTextSilently( "");
//		mAuthControl->Menu()->SetLabel( "");
//		mSignatureControl->Menu()->SetLabel( "");
//		mSmtpControl->Menu()->SetLabel( "");
		mCheckAccountControl->SetValue( 0);
		mRemoveMailControl->SetValue( 0);
		mIsDefaultControl->SetValue( 0);
		mIsBucketControl->SetValue( 0);
		mStorePwdControl->SetValue( 0);
		mPwdControl->SetEnabled( false);
	} else {
		BmRecvAccItem* accItem = dynamic_cast<BmRecvAccItem*>(mAccListView->ItemAt( selection));
		if (accItem) {
			BmRef<BmPopAccount> acc = dynamic_cast<BmPopAccount*>(accItem->ModelItem());
			if (acc) {
				mAccountControl->SetTextSilently( acc->Name().String());
				mAliasesControl->SetTextSilently( acc->MailAliases().String());
				mLoginControl->SetTextSilently( acc->Username().String());
				mMailAddrControl->SetTextSilently( acc->MailAddr().String());
				mPortControl->SetTextSilently( acc->PortNrString().String());
				mPwdControl->SetTextSilently( acc->Password().String());
				mRealNameControl->SetTextSilently( acc->RealName().String());
				mServerControl->SetTextSilently( acc->POPServer().String());
				BString val = acc->AuthMethod();
				mAuthControl->MarkItem( val.ToUpper().String());
				val = acc->SignatureName();
				mSignatureControl->MarkItem( val.Capitalize().String());
				val = acc->SMTPAccount();
				mSmtpControl->MarkItem( val.Capitalize().String());
				mCheckAccountControl->SetValue( acc->CheckMail());
				mRemoveMailControl->SetValue( acc->DeleteMailFromServer());
				mIsDefaultControl->SetValue( acc->MarkedAsDefault());
				mIsBucketControl->SetValue( acc->MarkedAsBitBucket());
				mStorePwdControl->SetValue( acc->PwdStoredOnDisk());
				mPwdControl->SetEnabled( acc->PwdStoredOnDisk());
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
CLVContainerView* BmPrefsRecvMailView::CreateAccListView( minimax minmax, int32 width, int32 height) {
	mAccListView = BmRecvAccView::CreateInstance( minmax, width, height);
	return mAccListView->ContainerView();
}
