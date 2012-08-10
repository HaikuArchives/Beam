/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */
#include <Alert.h>
#include <MenuItem.h>
#include <Screen.h>

#include <HGroup.h>
#include <LayeredGroup.h>
#include <MPopup.h>
#include <Space.h>
#include <VGroup.h>

#include "BetterButton.h"
#include "BubbleHelper.h"
#include "Colors.h"
#include "ColumnListView.h"
#include "CLVEasyItem.h"
#include "TextEntryAlert.h"

#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmLogHandler.h"
#include "BmMenuControl.h"
#include "BmMenuController.h"
#include "BmRecvAccount.h"
#include "BmPrefs.h"
#include "BmPrefsIdentityView.h"
#include "BmPrefsWin.h"
#include "BmRosterBase.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmTextControl.h"
#include "BmUtil.h"

/********************************************************************************\
	BmRecvIdentItem
\********************************************************************************/

enum Columns {
	COL_KEY = 0,
	COL_REAL_NAME,
	COL_MAIL_ADDR,
	COL_MAIL_ALIASES,
	COL_RECV_ACC,
	COL_BITBUCKET,
	COL_SIGNATURE,
	COL_SMTP_ACC
};

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvIdentItem::BmRecvIdentItem( ColumnListView* lv, 
											 BmListModelItem* _item)
	:	inherited( lv, _item, false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvIdentItem::~BmRecvIdentItem() { 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmRecvIdentItem::UpdateView( BmUpdFlags flags, bool redraw, 
											 uint32 updColBitmap) {
	BmRecvIdent* ident = ModelItem();
	if (flags & UPD_ALL) {
		const char* cols[] = {
			ident->Key().String(),
			ident->RealName().String(),
			ident->MailAddr().String(),
			ident->MailAliases().String(),
			ident->RecvAccount().String(),
			ident->MarkedAsBitBucket() ? "*" : "",
			ident->SignatureName().String(),
			ident->SMTPAccount().String(),
			NULL
		};
		SetTextCols( 0, cols);
		if (redraw)
			updColBitmap = 0xFFFFFFFF;
	}
	inherited::UpdateView( flags, redraw, updColBitmap);
}



/********************************************************************************\
	BmRecvIdentView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvIdentView::BmRecvIdentView( int32 width, int32 height)
	:	inherited( BRect(0,0,float(width-1),float(height-1)), "Beam_IdentView", 
					  B_SINGLE_SELECTION_LIST, 
					  false, true)
{
	int32 flags = CLV_SORT_KEYABLE;
	SetViewColor( B_TRANSPARENT_COLOR);
	if (ThePrefs->GetBool("StripedListView"))
		SetStripedBackground( true);
	else 
		flags |= CLV_TELL_ITEMS_WIDTH;

	AddColumn( new CLVColumn( "Identity", 80.0, flags, 50.0));
	AddColumn( new CLVColumn( "Real Name", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Mailaddress", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Aliases", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Receiving Account", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "F", 20.0, flags, 20.0, "(F)allback Identity?"));
	AddColumn( new CLVColumn( "Signature", 80.0, flags, 40.0));
	AddColumn( new CLVColumn( "Sending Account", 80.0, flags, 40.0));

	SetSortFunction( CLVEasyItem::CompareItems);
	SetSortKey( COL_KEY);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmRecvIdentView::~BmRecvIdentView() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmRecvIdentView::CreateListViewItem( BmListModelItem* item,
																	  BMessage*) {
	return new BmRecvIdentItem( this, item);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
BmListViewItem* BmRecvIdentView::AddModelItem( BmListModelItem* item) {
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
void BmRecvIdentView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			default:
				inherited::MessageReceived( msg);
		}
	} catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("RecvIdentView:\n\t") << err.what());
	}
}



/********************************************************************************\
	BmPrefsIdentityView
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsIdentityView::BmPrefsIdentityView() 
	:	inherited( "Identities")
{
	MView* view = 
		new VGroup(
			new BetterScrollView( 
				minimax(400,100,1E5,1E5), 
				mIdentListView = new BmRecvIdentView( 400, 100),
				BM_SV_H_SCROLLBAR | BM_SV_V_SCROLLBAR | BM_SV_CORNER
				| BM_SV_CAPTION,
				"99 identities"
			),
			new HGroup(
				mAddButton = new BetterButton( "Add Identity", 
												  new BMessage(BM_ADD_IDENTITY), 
												  this),
				mRemoveButton = new BetterButton("Remove Identity", 
													 new BMessage( BM_REMOVE_IDENTITY), 
													 this),
				0
			),
			new Space( minimax(0,10,0,10)),
			new HGroup(
				new MBorder( M_LABELED_BORDER, 10, (char*)"Identity Info",
					new VGroup(
						mIdentityControl = new BmTextControl( "Identity name:", 
																		  false, 0, 25),
						mRealNameControl = new BmTextControl( "Real name:"),
						mMailAddrControl = new BmTextControl( "Mail address:"),
						new HGroup(
							mReplyToControl = new BmTextControl( "Reply-To:"),
							mSpecialHeadersButton = new BetterButton( 
								"Special Headers...", 
							   new BMessage(BM_SET_SPECIAL_HEADERS), 
							   this, minimax(-1,-1,-1,-1)
							),
							0
						),
						mAliasesControl = new BmTextControl( "Aliases:"),
						new Space( minimax(0,5,0,5)),
						mRecvControl = new BmMenuControl( 
							"Receiving Account:", 
							new BmMenuController( 
								"Receiving Account:", 
								this, 
								new BMessage( BM_RECV_SELECTED),
								&BmGuiRosterBase::RebuildRecvAccountMenu, 
								BM_MC_LABEL_FROM_MARKED
							)
						),
						new Space( minimax(0,5,0,5)),
						mSmtpControl = new BmMenuControl( 
							"SMTP-account:", 
							new BmMenuController( 
								"SMTP-account:", 
								this, 
								new BMessage( BM_SMTP_SELECTED),
								&BmGuiRosterBase::RebuildSmtpAccountMenu, 
								BM_MC_LABEL_FROM_MARKED
							)
						),
						new Space( minimax(0,5,0,5)),
						mSignatureControl = new BmMenuControl( 
							"Signature:", 
							new BmMenuController( 
								"Signature:", 
								this, 
								new BMessage( BM_SIGNATURE_SELECTED),
								&BmGuiRosterBase::RebuildSignatureMenu, 
								BM_MC_LABEL_FROM_MARKED | BM_MC_ADD_NONE_ITEM
							)
						),
						0
					)
				),
				new VGroup(
					new MBorder( M_LABELED_BORDER, 10, (char*)"Options",
						new VGroup(
							mIsBucketControl 
								= new BmCheckControl( 
									"Use as fallback identity for the Receiving Account", 
									new BMessage(BM_IS_BUCKET_CHANGED), 
									this
								),
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
	
	BmDividable::DivideSame(
		mIdentityControl,
		mMailAddrControl,
		mAliasesControl,
		mRealNameControl,
		mReplyToControl,
		mSignatureControl,
		mSmtpControl,
		mRecvControl,
		NULL
	);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmPrefsIdentityView::~BmPrefsIdentityView() {
	TheBubbleHelper->SetHelp( mIdentListView, NULL);
	TheBubbleHelper->SetHelp( mIdentityControl, NULL);
	TheBubbleHelper->SetHelp( mMailAddrControl, NULL);
	TheBubbleHelper->SetHelp( mAliasesControl, NULL);
	TheBubbleHelper->SetHelp( mRealNameControl, NULL);
	TheBubbleHelper->SetHelp( mReplyToControl, NULL);
	TheBubbleHelper->SetHelp( mSpecialHeadersButton, NULL);
	TheBubbleHelper->SetHelp( mIsBucketControl, NULL);
	TheBubbleHelper->SetHelp( mSignatureControl, NULL);
	TheBubbleHelper->SetHelp( mSmtpControl, NULL);
	TheBubbleHelper->SetHelp( mRecvControl, NULL);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::Initialize() {
	inherited::Initialize();

	TheBubbleHelper->SetHelp( 
		mIdentListView, 
		"This listview shows every identity you have defined."
	);
	TheBubbleHelper->SetHelp( 
		mIdentityControl, 
		"Here you can enter a name for this identity.\n"
		"This name is used when you select an identity in Beam."
	);
	TheBubbleHelper->SetHelp( 
		mMailAddrControl, 
		"Here you can define the mail-address this identity will use.\n"
		"Beam creates the mail-address automatically,\n"
		"but you can override this by specifying the address here."
	);
	TheBubbleHelper->SetHelp( 
		mAliasesControl, 
			"Some email-providers allow definition of aliases for mail-accounts.\n"
			"In this case mails addressed to any of the aliases will be\n"
			"delivered to the real account. If you have defined such aliases,\n"
			"you can enter them here.\n\n"
			"(This information is used by Beam when trying to determine the\n"
			"identity to use when replying to mails that were addressed\n"
			"to one of the aliases).");
	TheBubbleHelper->SetHelp( 
		mRealNameControl, 
		"Please enter your real name here (e.g. 'Bob Meyer')."
	);
	TheBubbleHelper->SetHelp( 
		mReplyToControl, 
		"Here you can specify a mail-address that should receive the answers\n"
		"to the mails you sent (using this identity).\n"
		"If you leave this empty, the answers will go to the mail-address\n"
		"given above (or the default)."
		"Use this only if you want to explicitly redirect answers to somewhere\n"
		"else."
	);
	TheBubbleHelper->SetHelp( 
		mSpecialHeadersButton, 
		"Click this button if you want to define more default headers\n"
		"used for every mail being created from this identity.\n\n"
		"A typical use would be to set a standard Bcc: - address."
	);
	TheBubbleHelper->SetHelp( 
		mIsBucketControl, 
		"Check this if the corresponding receiving account is a catch-all account,\n"
		"and this identity should be used for unknown addresses\n"
		"(Beam uses this information when trying to determine\n"
		"the identity to use when replying to mails)."
	);
	TheBubbleHelper->SetHelp( 
		mSignatureControl, 
		"Here you can select the signature to be used \n"
		"for every mail sent from this identity."
	);
	TheBubbleHelper->SetHelp( 
		mRecvControl, 
		"Here you can select the receiving account that corresponds\n"
		"to this identity."
	);
	TheBubbleHelper->SetHelp( 
		mSmtpControl, 
		"Here you can select the sending account that shall be used\n"
		"to send mails from this identity."
	);

	mIdentityControl->SetTarget( this);
	mMailAddrControl->SetTarget( this);
	mAliasesControl->SetTarget( this);
	mRealNameControl->SetTarget( this);
	mReplyToControl->SetTarget( this);
	mIsBucketControl->SetTarget( this);

	mIdentListView->SetSelectionMessage( new BMessage( BM_SELECTION_CHANGED));
	mIdentListView->SetTarget( this);
	mIdentListView->StartJob( TheIdentityList.Get(), false);
	ShowIdentity( -1);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::Activated() {
	inherited::Activated();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::WriteStateInfo() {
	mIdentListView->WriteStateInfo();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmPrefsIdentityView::SanityCheck() {
	return DoSanityCheck( TheIdentityList.Get(), Name());
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::SaveData() {
	TheIdentityList->Store();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::UndoChanges() {
	TheIdentityList->ResetToSaved();
	ShowIdentity( -1);
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmPrefsIdentityView::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case BM_SELECTION_CHANGED: {
				int32 index = mIdentListView->CurrentSelection( 0);
				ShowIdentity( index);
				break;
			}
			case BM_TEXTFIELD_MODIFIED: {
				if (mCurrIdent) {
					BView* srcView = NULL;
					msg->FindPointer( "source", (void**)&srcView);
					BmTextControl* source = dynamic_cast<BmTextControl*>( srcView);
					if ( source == mIdentityControl)
						TheIdentityList->RenameItem( mCurrIdent->Name(), 
															  mIdentityControl->Text());
					else if ( source == mMailAddrControl)
						mCurrIdent->MailAddr( mMailAddrControl->Text());
					else if ( source == mAliasesControl)
						mCurrIdent->MailAliases( mAliasesControl->Text());
					else if ( source == mRealNameControl)
						mCurrIdent->RealName( mRealNameControl->Text());
					else if ( source == mReplyToControl)
						mCurrIdent->ReplyTo( mReplyToControl->Text());
					NoticeChange();
				}
				break;
			}
			case BM_IS_BUCKET_CHANGED: {
				if (mCurrIdent)
					mCurrIdent->MarkedAsBitBucket( mIsBucketControl->Value());
				NoticeChange();
				break;
			}
			case BM_SMTP_SELECTED: {
				BMenuItem* item = mSmtpControl->Menu()->FindMarked();
				if (item && BM_NoItemLabel != item->Label())
					mCurrIdent->SMTPAccount( item->Label());
				else
					mCurrIdent->SMTPAccount( "");
				NoticeChange();
				break;
			}
			case BM_RECV_SELECTED: {
				BMenuItem* item = mRecvControl->Menu()->FindMarked();
				if (item && BM_NoItemLabel != item->Label())
					mCurrIdent->RecvAccount( item->Label());
				else
					mCurrIdent->RecvAccount( "");
				NoticeChange();
				break;
			}
			case BM_SIGNATURE_SELECTED: {
				BMenuItem* item = mSignatureControl->Menu()->FindMarked();
				if (item && BM_NoItemLabel != item->Label())
					mCurrIdent->SignatureName( item->Label());
				else
					mCurrIdent->SignatureName( "");
				NoticeChange();
				break;
			}
			case BM_SET_SPECIAL_HEADERS: {
				if (!mCurrIdent)
					break;
				BmString specialHeaders = mCurrIdent->SpecialHeaders();
				BScreen scr;
				BRect screen( scr.Frame());
				float w=600, h=400;
				BRect alertFrame( (screen.Width()-w)/2,
										(screen.Height()-h)/2,
										(screen.Width()+w)/2,
										(screen.Height()+h)/2);
				TextEntryAlert* alert = 
					new TextEntryAlert( 
						"Edit Special Headers", 
						"Please enter any special headers below, "
						"just as they would appear in a mail.\n\n"
						"Here's an example: \n"
						"    Bcc: maileater <bitbucket@test.org>\n"
						"    Cc: Jim-Bob <jimbob@test.org>\n"
						"        John-Boy <johnboy@test.org>\n"
						"    X-Company-Special: grok.dis",
						specialHeaders.String(),
						"Cancel",
						"OK",
						false, 80, 20, B_WIDTH_FROM_LABEL, true,
						&alertFrame
					);
				alert->SetShortcut( 0, B_ESCAPE);
				alert->TextEntryView()->DisallowChar( 27);
				alert->TextEntryView()->SetFontAndColor( be_fixed_font);
				alert->SetShortcut( 0, B_ESCAPE);
				int32 choice = alert->Go( specialHeaders);
				if (choice == 1) {
					if (specialHeaders.Length()
					&& specialHeaders[specialHeaders.Length()-1] != '\n')
						specialHeaders << '\n';
					mCurrIdent->SpecialHeaders( specialHeaders);
				}
				break;
			}
			case BM_ADD_IDENTITY: {
				BmString key( "new identity");
				for( int32 i=1; TheIdentityList->FindItemByKey( key); ++i) {
					key = BmString("new identity_")<<i;
				}
				TheIdentityList->AddItemToList( 
					new BmIdentity( key.String(), 
										 TheIdentityList.Get())
				);
				mIdentityControl->MakeFocus( true);
				mIdentityControl->TextView()->SelectAll();
				NoticeChange();
				break;
			}
			case BM_REMOVE_IDENTITY: {
				int32 buttonPressed;
				if (msg->FindInt32( "which", &buttonPressed) != B_OK) {
					// first step, ask user about it:
					BAlert* alert 
						= new BAlert( "Remove Mail-Identity", 
										  (BmString(
										  		"Are you sure about removing the "
										  		"identity <") << mCurrIdent->Name() 
											   << ">?"
										  ).String(),
										  "Remove", "Cancel", NULL, B_WIDTH_AS_USUAL,
										  B_WARNING_ALERT);
					alert->SetShortcut( 1, B_ESCAPE);
					alert->Go( new BInvoker( new BMessage(BM_REMOVE_IDENTITY), 
													 BMessenger( this)));
				} else {
					// second step, do it if user said ok:
					if (buttonPressed == 0) {
						TheIdentityList->RemoveItemFromList( mCurrIdent.Get());
						mCurrIdent = NULL;
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
					BmIdentity* ident=NULL;
					msg->FindPointer( MSG_ITEM, (void**)&ident);
					BmListViewItem* accItem 
						= mIdentListView->FindViewItemFor( ident);
					if (accItem)
						mIdentListView->Select( mIdentListView->IndexOf( accItem));
				} else {
					// second step, set corresponding focus:
					BmString fieldName;
					fieldName = msg->FindString( MSG_FIELD_NAME);
					if (fieldName.ICompare( "smtpaccount")==0)
						mSmtpControl->MakeFocus( true);
					else if (fieldName.ICompare( "recvaccount")==0)
						mRecvControl->MakeFocus( true);
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
void BmPrefsIdentityView::ShowIdentity( int32 selection) {
	bool enabled = (selection != -1);
	mIdentityControl->SetEnabled( enabled);
	mMailAddrControl->SetEnabled( enabled);
	mAliasesControl->SetEnabled( enabled);
	mRealNameControl->SetEnabled( enabled);
	mReplyToControl->SetEnabled( enabled);
	mSignatureControl->SetEnabled( enabled);
	mSmtpControl->SetEnabled( enabled);
	mSpecialHeadersButton->SetEnabled( enabled);
	mRecvControl->SetEnabled( enabled);
	mRemoveButton->SetEnabled( enabled);
	mIsBucketControl->SetEnabled( enabled);
	
	if (selection == -1) {
		mCurrIdent = NULL;
		mIdentityControl->SetTextSilently( "");
		mMailAddrControl->SetTextSilently( "");
		mAliasesControl->SetTextSilently( "");
		mRealNameControl->SetTextSilently( "");
		mReplyToControl->SetTextSilently( "");
		mSignatureControl->ClearMark();
		mRecvControl->ClearMark();
		mSmtpControl->ClearMark();
		mIsBucketControl->SetValue( 0);
	} else {
		BmRecvIdentItem* identItem 
			= dynamic_cast<BmRecvIdentItem*>(mIdentListView->ItemAt( selection));
		if (identItem) {
			if  (mCurrIdent != identItem->ModelItem()) {
				mCurrIdent = identItem->ModelItem();
				if (mCurrIdent) {
					mIdentityControl->SetTextSilently( 
						mCurrIdent->Name().String()
					);
					mMailAddrControl->SetTextSilently( 
						mCurrIdent->MailAddr().String()
					);
					mAliasesControl->SetTextSilently( 
						mCurrIdent->MailAliases().String()
					);
					mRealNameControl->SetTextSilently( 
						mCurrIdent->RealName().String()
					);
					mReplyToControl->SetTextSilently( 
						mCurrIdent->ReplyTo().String()
					);
					mSignatureControl->MarkItem( 
						mCurrIdent->SignatureName().Length() 
							? mCurrIdent->SignatureName().String()
							: BM_NoItemLabel.String()
					);
					mRecvControl->MarkItem( mCurrIdent->RecvAccount().Length() 
													? mCurrIdent->RecvAccount().String()
													: "");
					mSmtpControl->MarkItem( mCurrIdent->SMTPAccount().Length() 
														? mCurrIdent->SMTPAccount().String()
														: "");
					mIsBucketControl->SetValue( mCurrIdent->MarkedAsBitBucket());
				}
			}
		} else
			mCurrIdent = NULL;
	}
}
