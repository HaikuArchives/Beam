/*
	BmMailEditWinPart2.cpp
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

/*
 * This file exists only because gcc isn't able to compile the whole
 * BmMailEditWin-code if it lives in one file.
 *
 */


#include <FilePanel.h>
#include <MenuItem.h>
#include <Message.h>

#include <HGroup.h>
#include <VGroup.h>
#include <MBorder.h>
#include <MMenuBar.h>
#include <Space.h>

#include "TextEntryAlert.h"
#include "BmToolbarButton.h"

#include "BmApp.h"
#include "BmBasics.h"
#include "BmBodyPartView.h"
#include "BmCheckControl.h"
#include "BmGuiUtil.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailEditWin.h"
#include "BmMailRef.h"
#include "BmMailView.h"
#include "BmMenuControl.h"
#include "BmMenuControllerBase.h"
#include "BmMsgTypes.h"
#include "BmPeople.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"
#include "BmTextControl.h"

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
filter_result BmShiftTabMsgFilter::Filter( BMessage* msg, 
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
				BNode personNode( &eref);
				if (personNode.InitCheck() != B_OK)
					continue;
				BmString name;
				BmReadStringAttr( &personNode, "META:name", name);
				BmString addrSpec;
				BmReadStringAttr( &personNode, "META:email", addrSpec);
				BmString addr;
				if (name.Length()
				&& ThePrefs->GetBool( "AddPeopleNameToMailAddr", true))
					addr << '"' << name << '"' << " <" << addrSpec << ">";
				else
					addr = addrSpec;
				if (addr.Length())
					win->AddAddressToTextControl( 
						dynamic_cast< BmTextControl*>( cntrl), addr
					);
				res = B_SKIP_MESSAGE;
			}
		}
	}
	return res;
}


#include <map>
typedef map< BmString, BmMailEditWin*> BmEditWinMap;
static BmEditWinMap nEditWinMap;

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
	if (sendNow) {
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
				return;
			} else {
				BM_LOG2( BM_LogGui, 
							"MailEditWin: ...marking mail as pending");
				mail->MarkAs( BM_MAIL_STATUS_PENDING);
				mail->ApplyFilter( true);
				BM_LOG2( BM_LogGui, 
							"MailEditWin: ...passing mail to smtp-account");
				smtpAcc->QueueMail( mail.Get());
				TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
			}
		} else {
			ShowAlertWithType( "Before you can send this mail, "
									 "you have to select the SMTP-Account "
									 "to use for sending it.",
									 B_INFO_ALERT);
			return;
		}
	} else 
		mail->MarkAs( BM_MAIL_STATUS_PENDING);
	mHasBeenSent = true;
	PostMessage( B_QUIT_REQUESTED);
}

/*------------------------------------------------------------------------------*\
	SendMailAfterEditHeader()
		-	
\*------------------------------------------------------------------------------*/
void BmMailEditWin::SendMailAfterEditHeader(const char* headerStr)
{
	// Reconstruct the mail with the new header and then send it:
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (!mail)
		return;
	BmRef<BmListModelItem> smtpRef 
		= TheSmtpAccountList->FindItemByKey( mail->AccountName());
	BmSmtpAccount* smtpAcc 
		= dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
	if (smtpAcc) {
		mail->SetNewHeader( headerStr);
		mail->MarkAs( BM_MAIL_STATUS_PENDING);
		mail->ApplyFilter( true);
		smtpAcc->QueueMail( mail.Get());
		TheSmtpAccountList->SendQueuedMailFor( smtpAcc->Name());
		mHasBeenSent = true;
		PostMessage( B_QUIT_REQUESTED);
	}
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

