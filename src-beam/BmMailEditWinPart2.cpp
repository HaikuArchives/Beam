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
bool BmMailEditWin::EditHeaders( )
{
	BmRef<BmMail> mail = mMailView->CurrMail();
	if (!mail)
		return false;
	if (!CreateMailFromFields( false))
		return false;
	// allow user to edit mail-header before we send it:
	BRect screen( bmApp->ScreenFrame());
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
				= labelItem ? labelItem->Label() : BM_MAIL_FOLDER_OUT;
			mail->SetDestFoldername( destFolderName);
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

