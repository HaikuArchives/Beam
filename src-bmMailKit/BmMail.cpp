/*
	BmMail.cpp
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


#include <Directory.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmBodyPartList.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmFilter.h"
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFilter.h"
#include "BmMailHeader.h"
#include "BmMailRef.h"
#include "BmMsgTypes.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"
#include "BmSmtpAccount.h"
#include "BmStorageUtil.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

const char* BM_MAIL_ATTR_NAME 			= B_MAIL_ATTR_NAME;
const char* BM_MAIL_ATTR_STATUS			= B_MAIL_ATTR_STATUS;
const char* BM_MAIL_ATTR_PRIORITY		= B_MAIL_ATTR_PRIORITY;
const char* BM_MAIL_ATTR_TO				= B_MAIL_ATTR_TO;
const char* BM_MAIL_ATTR_CC				= B_MAIL_ATTR_CC;
const char* BM_MAIL_ATTR_FROM				= B_MAIL_ATTR_FROM;
const char* BM_MAIL_ATTR_SUBJECT			= B_MAIL_ATTR_SUBJECT;
const char* BM_MAIL_ATTR_REPLY			= B_MAIL_ATTR_REPLY;
const char* BM_MAIL_ATTR_WHEN				= B_MAIL_ATTR_WHEN;
const char* BM_MAIL_ATTR_FLAGS			= B_MAIL_ATTR_FLAGS;
const char* BM_MAIL_ATTR_RECIPIENTS 	= B_MAIL_ATTR_RECIPIENTS;
const char* BM_MAIL_ATTR_MIME				= B_MAIL_ATTR_MIME;
const char* BM_MAIL_ATTR_HEADER			= B_MAIL_ATTR_HEADER;
const char* BM_MAIL_ATTR_CONTENT			= B_MAIL_ATTR_CONTENT;
const char* BM_MAIL_ATTR_ATTACHMENTS 	= "MAIL:has_attachment";
const char* BM_MAIL_ATTR_ACCOUNT			= "MAIL:account";
//
const char* BM_MAIL_ATTR_IDENTITY		= "MAIL:beam/identity";
const char* BM_MAIL_ATTR_MARGIN	 		= "MAIL:beam/margin";
const char* BM_MAIL_ATTR_WHEN_CREATED = "MAIL:beam/when-created";

const char* BM_FIELD_BCC 					= "Bcc";
const char* BM_FIELD_CC 					= "Cc";
const char* BM_FIELD_CONTENT_TYPE 		= "Content-Type";
const char* BM_FIELD_CONTENT_DISPOSITION = "Content-Disposition";
const char* BM_FIELD_CONTENT_DESCRIPTION = "Content-Description";
const char* BM_FIELD_CONTENT_LANGUAGE 	= "Content-Language";
const char* BM_FIELD_CONTENT_TRANSFER_ENCODING = "Content-Transfer-Encoding";
const char* BM_FIELD_CONTENT_ID 			= "Content-Id";
const char* BM_FIELD_DATE 					= "Date";
const char* BM_FIELD_FROM 					= "From";
const char* BM_FIELD_IN_REPLY_TO			= "In-Reply-To";
const char* BM_FIELD_LIST_ARCHIVE		= "List-Archive";
const char* BM_FIELD_LIST_HELP			= "List-Help";
const char* BM_FIELD_LIST_ID				= "List-Id";
const char* BM_FIELD_LIST_POST			= "List-Post";
const char* BM_FIELD_LIST_SUBSCRIBE		= "List-Subscribe";
const char* BM_FIELD_LIST_UNSUBSCRIBE	= "List-Unsubscribe";
const char* BM_FIELD_MAIL_FOLLOWUP_TO	= "Mail-Followup-To";
const char* BM_FIELD_MAIL_REPLY_TO		= "Mail-Reply-To";
const char* BM_FIELD_MAILING_LIST		= "Mailing-List";
const char* BM_FIELD_MESSAGE_ID			= "Message-Id";
const char* BM_FIELD_MIME 					= "Mime-Version";
const char* BM_FIELD_PRIORITY				= "Priority";
const char* BM_FIELD_REFERENCES			= "References";
const char* BM_FIELD_REPLY_TO				= "Reply-To";
const char* BM_FIELD_RESENT_BCC			= "Resent-Bcc";
const char* BM_FIELD_RESENT_CC			= "Resent-Cc";
const char* BM_FIELD_RESENT_DATE			= "Resent-Date";
const char* BM_FIELD_RESENT_FROM			= "Resent-From";
const char* BM_FIELD_RESENT_MESSAGE_ID	= "Resent-Message-Id";
const char* BM_FIELD_RESENT_REPLY_TO	= "Resent-Reply-To";
const char* BM_FIELD_RESENT_SENDER 		= "Resent-Sender";
const char* BM_FIELD_RESENT_TO			= "Resent-To";
const char* BM_FIELD_SENDER 				= "Sender";
const char* BM_FIELD_SUBJECT 				= "Subject";
const char* BM_FIELD_TO 					= "To";
const char* BM_FIELD_USER_AGENT			= "User-Agent";
const char* BM_FIELD_X_LIST				= "X-List";
const char* BM_FIELD_X_MAILER				= "X-Mailer";
const char* BM_FIELD_X_PRIORITY			= "X-Priority";

const char* BM_MAIL_STATUS_DRAFT			= "Draft";
const char* BM_MAIL_STATUS_FORWARDED	= "Forwarded";
const char* BM_MAIL_STATUS_NEW			= "New";
const char* BM_MAIL_STATUS_PENDING		= "Pending";
const char* BM_MAIL_STATUS_READ			= "Read";
const char* BM_MAIL_STATUS_REDIRECTED	= "Redirected";
const char* BM_MAIL_STATUS_REPLIED		= "Replied";
const char* BM_MAIL_STATUS_SENT			= "Sent";

const char* BM_MAIL_FOLDER_DRAFT			= "draft";
const char* BM_MAIL_FOLDER_IN				= "in";
const char* BM_MAIL_FOLDER_OUT			= "out";

/********************************************************************************\
	BmMail
\********************************************************************************/

#define BM_MAILKEY(ref) \
	(BmString("Mail_") << ref->NodeRef().node)

const char* const BmMail::BM_QUOTE_AUTO_WRAP = 		"Auto Wrap";
const char* const BmMail::BM_QUOTE_SIMPLE = 			"Simple";
const char* const BmMail::BM_QUOTE_PUSH_MARGIN = 	"Push Margin";

/*------------------------------------------------------------------------------*\
	CreateInstance( mailref)
		-	constructs a mail from file
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateInstance( BmMailRef* ref) {
	BAutolock lock( GlobalLocker());
	if (!lock.IsLocked()) {
		BM_SHOWERR("BmMail::CreateInstance(): Could not acquire global lock!");
		return NULL;
	}
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMail).name());
	if (proxy) {
		BmString key( BM_MAILKEY( ref));
		BmRef<BmMail> mail( dynamic_cast<BmMail*>( proxy->FetchObject( key)));
		if (mail)
			return mail;
	}
	return new BmMail( ref);
}

/*------------------------------------------------------------------------------*\
	BmMail()
	-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMail::BmMail( bool outbound)
	:	inherited("MailModel_dummy")
	,	mMailRef( NULL)
	,	mHeader( NULL)
	,	mBody( NULL)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( outbound)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
	,	mMoveToTrash( false)
{
	BmString emptyMsg = BmString(BM_FIELD_MIME)+": 1.0\r\n";
	emptyMsg << "Content-Type: text/plain; charset=\"" 
				<< ThePrefs->GetString( "DefaultCharset")
				<<	"\"\r\n";
	emptyMsg << BM_FIELD_DATE << ": " 
				<< TimeToString( time( NULL), "%a, %d %b %Y %H:%M:%S %z");
	emptyMsg << "\r\n\r\n";
	SetTo( emptyMsg, "");
	if (outbound) {
		// if outbound, stuff basic info (from-address, smtp-account, sig)
		// into mail:
		BmRef<BmIdentity> identRef = TheIdentityList->CurrIdentity();
		if (identRef) {
			SetFieldVal( BM_FIELD_FROM, identRef->GetFromAddress());
			mAccountName = identRef->SMTPAccount();
			SetSignatureByName( identRef->SignatureName());
			mIdentityName = identRef->Key();
		}
	}
}

/*------------------------------------------------------------------------------*\
	BmMail( msgText, msgUID, account)
		-	constructor
		-	creates message with given text and receiver-account
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BmString &msgText, const BmString account) 
	:	inherited( "MailModel_dummy")
	,	mHeader( NULL)
	,	mBody( NULL)
	,	mMailRef( NULL)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
	,	mMoveToTrash( false)
{
	SetTo( msgText, account);

	// get default-identity corresponding to given account:
	BmRef<BmIdentity> identRef 
		= TheIdentityList->FindIdentityForPopAccount( account);
	BmIdentity* ident = dynamic_cast< BmIdentity*>( identRef.Get());
	if (ident) {
		// now try to find a better match through recipient-addresses:
		BmAddress recvAddr( mHeader->DetermineReceivingAddrFor( ident));
		if (recvAddr.InitOK()) {
			identRef	
				= TheIdentityList->FindIdentityForAddrSpec( recvAddr.AddrSpec());
			if (identRef)
				ident = dynamic_cast< BmIdentity*>( identRef.Get());
		}
	}
	if (ident)
		mIdentityName = ident->Key();
}
	
/*------------------------------------------------------------------------------*\
	BmMail( mailref)
		-	constructor via a mail-ref (from file)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BmMailRef* ref) 
	:	inherited( BM_MAILKEY( ref))
	,	mHeader( NULL)
	,	mBody( NULL)
	,	mMailRef( ref)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
	,	mMoveToTrash( false)
{
	mOutbound = 
		Status() == BM_MAIL_STATUS_DRAFT
		||	Status() == BM_MAIL_STATUS_PENDING
		||	Status() == BM_MAIL_STATUS_SENT;
}

/*------------------------------------------------------------------------------*\
	~BmMail()
	-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
}

/*------------------------------------------------------------------------------*\
	SetTo( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was 
			received from
\*------------------------------------------------------------------------------*/
void BmMail::SetTo( BmString &text, const BmString account) {
	BM_LOG2( BM_LogMailParse, "Converting Linebreaks to CRLF...");
	text.ConvertLinebreaksToCRLF();
	BM_LOG2( BM_LogMailParse, "done (Converting Linebreaks to CRLF)");

	// find end of header (and start of body):
	int32 headerLen = text.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR)
		throw BM_mail_format_error("BmMail: Could not determine borderline "
											"between header and text of message");

	headerLen += 2;
							// don't include separator-line in header-string

	BM_LOG2( BM_LogMailParse, "Adopting mailtext...");
	mText.Adopt( text);						// take over the msg-string
	BM_LOG2( BM_LogMailParse, "...done (Adopting mailtext)");
	mAccountName = account;

	BM_LOG2( BM_LogMailParse, "setting header-string...");
	BmString header;
	header.SetTo( mText, headerLen);
	BM_LOG2( BM_LogMailParse, "...init header from header-string...");
	mHeader = new BmMailHeader( header, this);
	BM_LOG2( BM_LogMailParse, "...done (header)");

	BM_LOG2( BM_LogMailParse, "init of body...");
	mBody = new BmBodyPartList( this);
	mBody->ParseMail();
	BM_LOG2( BM_LogMailParse, "done (init of body)");

	mInitCheck = B_OK;
}
	
/*------------------------------------------------------------------------------*\
	SetNewHeader()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetNewHeader( const BmString& headerStr) {
	BmString newMsgText;
	newMsgText.ConvertLinebreaksToCRLF( &headerStr);
	int32 len = newMsgText.Length();
	if (newMsgText[len-1] != '\n')
		newMsgText << "\r\n";
	newMsgText << mText.String()+HeaderLength();
	SetTo( newMsgText, mAccountName);
	Store();
	StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	SetSignatureByName( sigName)
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetSignatureByName( const BmString sigName) {
	if (!mBody || mSignatureName==sigName)
		return;
	mSignatureName = sigName;
	mBody->Signature( TheSignatureList->GetSignatureStringFor( sigName));
}

/*------------------------------------------------------------------------------*\
	MarkAs()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::MarkAs( const char* status) {
	if (InitCheck() != B_OK)
		return;
	try {
		if (mMailRef)
			mMailRef->MarkAs( status);
		else {
			BNode mailNode;
			status_t err;
			entry_ref eref;
			// we write the new status...
			if (mEntry.InitCheck() != B_OK) {
				mDefaultStatus = status;
				return;
			}
			mEntry.GetRef( &eref);
			if ((err = mailNode.SetTo( &eref)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not create node for current mail-file.\n\n "
								"Result: ") << strerror(err)
				);
			mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, status, 
									  strlen( status)+1);
		}
	} catch( BM_error &e) {
		BM_SHOWERR(e.what());
	}
}
	
/*------------------------------------------------------------------------------*\
	Status()
		-	
\*------------------------------------------------------------------------------*/
const BmString BmMail::Status() const { 
	return mMailRef ? mMailRef->Status() : DefaultStatus();
}

/*------------------------------------------------------------------------------*\
	DefaultStatus()
		-	
\*------------------------------------------------------------------------------*/
const BmString BmMail::DefaultStatus() const {
	if (!mDefaultStatus.Length())
		mDefaultStatus = mOutbound 
								? BM_MAIL_STATUS_DRAFT 
								: BM_MAIL_STATUS_NEW;
	return mDefaultStatus;
}

/*------------------------------------------------------------------------------*\
	HasAttachments()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::HasAttachments() const { 
	if (mBody)
		return mBody->HasAttachments();
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	GetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BmString& BmMail::GetFieldVal( const BmString fieldName) {
	if (mHeader)
		return mHeader->GetFieldVal( fieldName);
	else
		return BM_DEFAULT_STRING;
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetFieldVal( const BmString fieldName, const BmString value) {
	// we set the field-value inside the mail-header only if it has content
	// otherwise we remove the field from the header:
	if (!mHeader)
		return;
	if (value.Length())
		mHeader->SetFieldVal( fieldName, value);
	else
		mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	RemoveFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::RemoveField( const BmString fieldName) {
	mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	HasComeFromList()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::HasComeFromList() const {
	return mHeader 
			 && (!mHeader->IsFieldEmpty( BM_FIELD_LIST_ID)
			 	  || !mHeader->IsFieldEmpty( BM_FIELD_MAILING_LIST)
			 	  || !mHeader->IsFieldEmpty( BM_FIELD_X_LIST));
}

/*------------------------------------------------------------------------------*\
	CreateInlineForward()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateInlineForward( bool withAttachments, 
														 const BmString selectedText) {
	BmRef<BmMail> newMail = new BmMail( true);
	// massage subject, if neccessary:
	BmString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));
	newMail->AddPartsFromMail( this, withAttachments, BM_IS_FORWARD, false, 
										selectedText);
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_FORWARDED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateAttachedForward()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateAttachedForward() {
	BmRef<BmMail> newMail = new BmMail( true);
	if (mMailRef->InitCheck() == B_OK)
		newMail->Body()->AddAttachmentFromRef( mMailRef->EntryRefPtr(), 
															DefaultCharset());
	// massage subject, if neccessary:
	BmString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_FORWARDED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	DetermineReplyAddress()
	-	
\*------------------------------------------------------------------------------*/
BmString BmMail::DetermineReplyAddress( int32 replyMode, bool canonicalize,
													 bool& replyGoesToPersonOnly) {
	// fill address information, depending on reply-mode:
	BmString replyAddr;
	if (!Header())
		return "";
	if (replyMode == BMM_REPLY) {
		// smart (*cough*) mode: If the mail has come from a list, we react
		// according to user prefs (reply-to-list or reply-to-originator).
		bool hasComeFromList = HasComeFromList();
		replyGoesToPersonOnly = !hasComeFromList;
		if (hasComeFromList) {
			replyAddr = ThePrefs->GetBool( "PreferReplyToList", true)
							? Header()->DetermineListAddress()
							: Header()->DetermineOriginator();
		}
		if (!replyAddr.Length()) {
			replyAddr = Header()->DetermineOriginator();
		}
	} else if (replyMode == BMM_REPLY_LIST) {
		// blindly use list-address for reply (this might mean that we send
		// a reply to the list although the messages has not come from the list):
		replyAddr = Header()->DetermineListAddress( true);
		replyGoesToPersonOnly = false;
	} else if (replyMode == BMM_REPLY_ORIGINATOR) {
		// bypass the reply-to, this way one can send mail to the 
		// original author of a mail which has been 'reply-to'-munged 
		// by a mailing-list processor:
		replyAddr = Header()->DetermineOriginator( true);
		replyGoesToPersonOnly = true;
	} else if (replyMode == BMM_REPLY_ALL) {
		// since we are replying to all recipients of this message,
		// we now include the Originator (plain and standard way):
		replyAddr = Header()->DetermineOriginator();
		replyGoesToPersonOnly = false;
	}
	if (canonicalize)
		return BmAddressList( replyAddr).AddrString();
	else
		return replyAddr;
}

/*------------------------------------------------------------------------------*\
	CreateReply()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateReply( int32 replyMode, 
											  const BmString selectedText) {
	bool dummy;
	return doCreateReply( replyMode, dummy, selectedText, true);
}

/*------------------------------------------------------------------------------*\
	CreateReply()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateReply( int32 replyMode, 
											  bool& replyGoesToPersonOnly,
											  const BmString selectedText) {
	return doCreateReply( replyMode, replyGoesToPersonOnly, selectedText, false);
}

/*------------------------------------------------------------------------------*\
	CreateReply()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::doCreateReply( int32 replyMode, 
												 bool& replyGoesToPersonOnly,
											 	 const BmString selectedText,
											 	 bool ignoreReplyGoesToPersonOnly) {
	BmRef<BmMail> newMail = new BmMail( true);
	// copy old message ID into in-reply-to and references fields:
	BmString messageID = GetFieldVal( BM_FIELD_MESSAGE_ID);
	newMail->SetFieldVal( BM_FIELD_IN_REPLY_TO, messageID);
	BmString oldRefs = GetFieldVal( BM_FIELD_REFERENCES);
	if (oldRefs.Length())
		newMail->SetFieldVal( BM_FIELD_REFERENCES, oldRefs + " " + messageID);
	else
		newMail->SetFieldVal( BM_FIELD_REFERENCES, messageID);
	BmString newTo 
		= DetermineReplyAddress( replyMode, false, replyGoesToPersonOnly);
	newMail->SetFieldVal( BM_FIELD_TO, newTo);
	// Since we are replying, we generate the new mail's from-address 
	// from the received mail's to-/cc-/bcc-info in several steps.
	// First, we check if this mail has an identity assigned to it:
	BmString receivingAddr;
	BmRef<BmListModelItem> identRef 
		= TheIdentityList->FindItemByKey( mIdentityName);
	BmIdentity* ident = dynamic_cast< BmIdentity*>( identRef.Get());
	if (!ident) {
		// second, we check if the account through which the mail has been 
		// received can be identified (not always possible, since the account 
		// may have been renamed or deleted by now):
		BmRef<BmIdentity> identRef 
			= TheIdentityList->FindIdentityForPopAccount( mAccountName);
		ident = dynamic_cast< BmIdentity*>( identRef.Get());
	}
	if (ident) {
		// receiving identity is known, we let it find the receiving address
		receivingAddr = Header()->DetermineReceivingAddrFor( ident);
		// if the identity doesn't handle any receiving address of this mail,
		// we simply use the identity's default address:
		if (!receivingAddr.Length())
			receivingAddr = ident->GetFromAddress();
	}
	if (!receivingAddr.Length()) {
		// the receiving address could not be determined, so we iterate through 
		// all identities and try to find one that (may) have received this mail.
		BmAutolockCheckGlobal lock( ThePopAccountList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				"CreateReply(): Unable to get lock on PopAccountList"
			);
		BmModelItemMap::const_iterator iter;
		for( iter = TheIdentityList->begin(); 
			  iter != TheIdentityList->end() && !receivingAddr.Length(); 
			  ++iter) {
			ident = dynamic_cast< BmIdentity*>( iter->second.Get());
			if (ident)
				receivingAddr = Header()->DetermineReceivingAddrFor( ident);
		}
	}
	if (ident && receivingAddr.Length()) {
		newMail->SetFieldVal( BM_FIELD_FROM, receivingAddr);
		newMail->SetSignatureByName( ident->SignatureName());
		newMail->AccountName( ident->SMTPAccount());
		newMail->IdentityName( ident->Key());
	}
	// if we are replying to all, we may need to include more addresses:
	if (replyMode == BMM_REPLY_ALL) {
		BmString newCc = GetFieldVal( BM_FIELD_CC);
		if (newCc != newTo)
			// add address only if not already done so
			newMail->SetFieldVal( BM_FIELD_CC, newCc);
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_CC, receivingAddr);
		BmString additionalCc = GetFieldVal( BM_FIELD_TO);
		if (additionalCc != newTo)
			// add address only if not already done so
			newMail->Header()->AddFieldVal( BM_FIELD_CC, additionalCc);
		// remove the receiving address from list of recipients, since we
		// do not want to send ourselves a reply:
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_CC, receivingAddr);
	}
	// massage subject, if neccessary:
	BmString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateReplySubjectFor( subject));
	newMail->AddPartsFromMail( this, false, BM_IS_REPLY, 
										ignoreReplyGoesToPersonOnly 
											? false
											: replyGoesToPersonOnly, 
										selectedText);
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_REPLIED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateRedirect()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateRedirect() {
	BmRef<BmMail> newMail = new BmMail( true);
	BmString msgText( mText);
	newMail->SetTo( msgText, "");
	if (newMail->IsRedirect()) {
		// oops, mail already had been redirected, we clobber the existing
		// Resent-fields, since STD11 says multiple Resent-fields result in 
		// undefined behaviour:
		newMail->RemoveField( BM_FIELD_RESENT_BCC);
		newMail->RemoveField( BM_FIELD_RESENT_CC);
		newMail->RemoveField( BM_FIELD_RESENT_DATE);
		newMail->RemoveField( BM_FIELD_RESENT_FROM);
		newMail->RemoveField( BM_FIELD_RESENT_MESSAGE_ID);
		newMail->RemoveField( BM_FIELD_RESENT_REPLY_TO);
		newMail->RemoveField( BM_FIELD_RESENT_SENDER);
		newMail->RemoveField( BM_FIELD_RESENT_TO);
	}
	newMail->IsRedirect( true);
	newMail->SetFieldVal( BM_FIELD_RESENT_DATE, 
								 TimeToString( time( NULL), 
								 					"%a, %d %b %Y %H:%M:%S %z"));
	BmRef<BmIdentity> identRef = TheIdentityList->CurrIdentity();
	if (identRef) {
		newMail->SetFieldVal( BM_FIELD_RESENT_FROM, identRef->GetFromAddress());
		newMail->AccountName( identRef->SMTPAccount());
		newMail->SetSignatureByName( identRef->SignatureName());
	}
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_REDIRECTED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateAsNew()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateAsNew() {
	BmRef<BmMail> newMail = new BmMail( true);
	BmString msgText( mText);
	newMail->SetTo( msgText, "");
	BmRef<BmListModelItem> identRef 
		= TheIdentityList->FindItemByKey( mIdentityName);
	BmIdentity* ident = dynamic_cast<BmIdentity*>( identRef.Get());
	if (ident) {
		newMail->AccountName( ident->SMTPAccount());
		newMail->SetSignatureByName( ident->SignatureName());
		newMail->IdentityName( ident->Key());
	}
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateReplySubjectFor()
	-	
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateReplySubjectFor( const BmString subject) {
	BmString isReplyRX 
		= ThePrefs->GetString( "ReplySubjectRX", "^\\s*(Re|Aw)(\\[\\d+\\])?:");
	Regexx rx;
	if (!rx.exec( subject, isReplyRX, Regexx::nocase|Regexx::nomatch)) {
		BmString subjectStr = ThePrefs->GetString( "ReplySubjectStr", "Re: %s");
		subjectStr = rx.replace( subjectStr, "%s", subject, 
								 		 Regexx::nocase|Regexx::global|Regexx::noatom);
		return subjectStr;
	}
	return subject;
}

/*------------------------------------------------------------------------------*\
	CreateForwardSubjectFor()
	-	
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateForwardSubjectFor( const BmString subject) {
	BmString isForwardRX 
		= ThePrefs->GetString( "ForwardSubjectRX", 
									  "^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	Regexx rx;
	if (!rx.exec( subject, isForwardRX, Regexx::nocase|Regexx::nomatch)) {
		BmString subjectStr 
			= ThePrefs->GetString( "ForwardSubjectStr", "[Fwd: %s]");
		subjectStr = rx.replace( subjectStr, "%s", subject, 
										 Regexx::nocase|Regexx::global|Regexx::noatom);
		return subjectStr;
	}
	return subject;
}

/*------------------------------------------------------------------------------*\
	CreateReplyIntro()
		-	creates an appropriate intro-line for a reply-message
		-	the returned string is the intro in UTF8
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateReplyIntro( bool mailIsToPersonOnly) {
	Regexx rx;
	BmString intro = ThePrefs->GetString( "ReplyIntroStr");
	intro = rx.replace( intro, "%D", 
							  TimeToString( mMailRef->When(), "%Y-%m-%d"),
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%T", 
							  TimeToString( mMailRef->When(), "%X [%z]"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "\\n", "\n", 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%S", mMailRef->Subject(), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BmString fromNicks;
	if (!mailIsToPersonOnly) {
		// the more formal approach, we replace %F by the originator(s) 
		// nickname or address:
		BmAddressList fromAddr = Header()->DetermineOriginator( true);
		BmAddrList::const_iterator pos;
		for( pos=fromAddr.begin(); pos!=fromAddr.end(); ++pos) {
			if (pos != fromAddr.begin())
				fromNicks << ", ";
			fromNicks << (pos->HasPhrase() ? pos->Phrase() : pos->AddrSpec());
		}
	} else
		// less formal way, replace %F by ReplyIntroDefaultNick in order
		// to say something like "On xxx, you wrote":
		fromNicks = ThePrefs->GetString( "ReplyIntroDefaultNick", "you");
	intro = rx.replace( intro, "%F", fromNicks, 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	return intro;
}

/*------------------------------------------------------------------------------*\
	CreateForwardIntro()
		-	creates an appropriate intro-line for a forwarded message
		-	the returned string is the intro in UTF8
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateForwardIntro() {
	Regexx rx;
	BmString intro = ThePrefs->GetString( "ForwardIntroStr");
	intro = rx.replace( intro, "%D", 
							  TimeToString( mMailRef->When(), "%Y-%m-%d"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%T", 
							  TimeToString( mMailRef->When(), "%X [%z]"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "\\n", "\n", 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%S", mMailRef->Subject(), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BmAddressList fromAddr = Header()->DetermineOriginator();
	BmString fromNicks;
	BmAddrList::const_iterator pos;
	for( pos=fromAddr.begin(); pos!=fromAddr.end(); ++pos) {
		if (pos != fromAddr.begin())
			fromNicks << ", ";
		fromNicks << (pos->HasPhrase() ? pos->Phrase() : pos->AddrSpec());
	}
	intro = rx.replace( intro, "%F", fromNicks, 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	return intro;
}

/*------------------------------------------------------------------------------*\
	AddAttachmenstFromRef()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::AddAttachmentFromRef( const entry_ref* ref,
											  const BmString& charset) {
	if (mBody)
		mBody->AddAttachmentFromRef( ref, charset);
}

/*------------------------------------------------------------------------------*\
	AddPartsFromMail()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::AddPartsFromMail( BmRef<BmMail> mail, bool withAttachments, 
										 bool isForward, bool mailIsToPersonOnly,
										 const BmString selectedText) {
	if (!mail || !mail->Body() || !mBody)
		return;
	// copy and quote text-body:
	BmRef<BmBodyPart> newTextBody( mBody->EditableTextBody());
	BmRef<BmBodyPart> textBody( mail->Body()->EditableTextBody());
	// copy info about charset from old into new mail:
	BmString charset = mail->DefaultCharset();
	BmString quotedText;
	int32 newLineLen = QuoteText( (selectedText.Length() || !textBody)
													? selectedText 
													: textBody->DecodedData(),
											quotedText,
				 							ThePrefs->GetString( "QuotingString"),
											ThePrefs->GetInt( "MaxLineLen"));
	BmString intro( isForward 
							? mail->CreateForwardIntro() << "\n"
							: mail->CreateReplyIntro( mailIsToPersonOnly) << "\n");
	if (newTextBody)
		mBody->SetEditableText( newTextBody->DecodedData() + "\n" + intro 
											+ quotedText, 
										charset);
	else
		mBody->SetEditableText( intro + quotedText, charset);
	BumpRightMargin( newLineLen);
	if (withAttachments && mail->Body()->HasAttachments()) {
		BmModelItemMap::const_iterator iter, end;
		if (mail->Body()->IsMultiPart()) {
			iter = mail->Body()->begin()->second->begin();
			end = mail->Body()->begin()->second->end();
		} else {
			iter = mail->Body()->begin();
			end = mail->Body()->end();
		}
		// copy all attachments (maybe except v-cards):
		bool doNotAttachVCards 
			= ThePrefs->GetBool( "DoNotAttachVCardsToForward", true);
		for( ; iter != end; ++iter) {
			BmBodyPart* bodyPart 
				= dynamic_cast< BmBodyPart*>( iter->second.Get());
			if (textBody != bodyPart) {
				if (doNotAttachVCards 
				&& bodyPart->MimeType().ICompare( "text/x-vcard") == 0)
					continue;
				BmBodyPart* copiedBody = new BmBodyPart( *bodyPart);
				mBody->AddItemToList( copiedBody);
			}
		}
	}
	AddBaseMailRef( mail->MailRef());
}

/*------------------------------------------------------------------------------*\
	ConstructAndStore()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::ConstructAndStore() {
	BmRef< BmBodyPart> bodyPart( mBody->EditableTextBody());
	if (bodyPart && ConstructRawText( bodyPart->DecodedData(),
												 DefaultCharset(), 
												 mAccountName)) {
		Store();
	}
}

/*------------------------------------------------------------------------------*\
	ConstructRawText()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ConstructRawText( const BmString& editedUtf8Text, 
										 const BmString& charset,
										 const BmString smtpAccount) {
	int32 startSize = mBody->EstimateEncodedSize() + editedUtf8Text.Length() 
							+ max( mHeader->HeaderLength(), (int32)4096)+4096;
	startSize += 65536-(startSize%65536);
	BmStringOBuf msgText( startSize, 1.2);
	mAccountName = smtpAccount;
	if (!mHeader->ConstructRawText( msgText, charset))
		return false;
	mBody->SetEditableText( editedUtf8Text, charset);
	if (!mBody->ConstructBodyForSending( msgText))
		return false;
	uint32 len = msgText.CurrPos();
	if (len && msgText.ByteAt( len-1) != '\n')
		msgText << "\r\n";
	mText.Adopt( msgText.TheString());
	BM_LOG3( BM_LogMailParse, 
				BmString("CONSTRUCTED MSG: \n-----START--------\n") << mText 
					<< "\n-----END----------");
	return true;
}

/*------------------------------------------------------------------------------*\
	DefaultCharset()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmMail::DefaultCharset() const {
	if (mBody)
		return mBody->DefaultCharset();
	return BmEncoding::DefaultCharset;
}

/*------------------------------------------------------------------------------*\
	SetBaseMailInfo( ref, newStatus)
		-	
\*------------------------------------------------------------------------------*/
void BmMail::SetBaseMailInfo( BmMailRef* ref, const BmString newStatus) {
	mBaseRefVect.push_back( ref);
	mNewBaseStatus = newStatus;
}

/*------------------------------------------------------------------------------*\
	AddBaseMailRef( ref)
		-	
\*------------------------------------------------------------------------------*/
void BmMail::AddBaseMailRef( BmMailRef* ref) {
	mBaseRefVect.push_back( ref);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const BPath& BmMail::DestFolderpath() const {
	if (mDestFolderpath.InitCheck() != B_OK) {
		if (mEntry.InitCheck() == B_OK 
		|| mMailRef && mMailRef->InitCheck() == B_OK) {
			status_t err;
			BDirectory homeDir;
			BEntry entry;
			if (mEntry.InitCheck() == B_OK) {
				entry = mEntry;
			} else if (mMailRef && mMailRef->InitCheck() == B_OK) {
				if ((err = entry.SetTo( mMailRef->EntryRefPtr())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not create entry from mail-ref <") 
							<< mMailRef->Key() << ">\n\n Result: " << strerror(err)
					);
			}
			if ((err = entry.GetParent( &homeDir)) != B_OK)
				BM_THROW_RUNTIME( BmString("Could not get parent for mail <")
											<< Name() << ">\n\n Result: " 
											<< strerror(err));
			if ((err = mDestFolderpath.SetTo( &homeDir, NULL)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not get path for homeDir of mail <")
						<< Name() << ">\n\n Result: " << strerror(err)
				);
		}
	}
	return mDestFolderpath;
}

/*------------------------------------------------------------------------------*\
	MoveToDestFolderpath()
		-	moves a mail into a new destination folder (which is specified in 
			mDestFolderpath)
		-	the mail is just moved, it is not re-written to disk.
\*------------------------------------------------------------------------------*/
bool BmMail::MoveToDestFolderpath() {
	if (mDestFolderpath.InitCheck() == B_OK) {
		if (mEntry.InitCheck() == B_OK
		|| mMailRef && mMailRef->InitCheck() == B_OK) {
			status_t err;
			BDirectory homeDir;
			BEntry entry;
			if (mEntry.InitCheck() != B_OK) {
				if ((err = mEntry.SetTo( mMailRef->EntryRefPtr())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not create entry from mail-ref <") 
							<< mMailRef->Key() << ">\n\n Result: " << strerror(err)
					);
			}
			BDirectory newHomeDir( mDestFolderpath.Path());
			if (newHomeDir.InitCheck() != B_OK)
				return false;
			if ((err = mEntry.MoveTo( &newHomeDir)) != B_OK) {
				BM_LOGERR( 
					BmString("Could not move mail <") << Name() 
						<< "> to folder <" << mDestFolderpath.Path()
						<< ">\n\n Result: " << strerror(err)
				);
				return false;
			}
			entry_ref eref;
			if ((err = mEntry.GetRef( &eref)) != B_OK) {
				BM_LOGERR( 
					BmString("Could not get entry-ref for mail <") << Name()
						<< ">.\n\n Result: " << strerror(err)
				);
				return false;
			}
			// create a (new) mail-ref for the freshly saved mail:
			struct stat st;
			mEntry.GetStat( &st);
			mMailRef = BmMailRef::CreateInstance( eref, st);
			return true;
		}
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::SetDestFoldername( const BmString& inFoldername) {
	BmString destFoldername( inFoldername);
	destFoldername.ReplaceAll( '.', '/');
	if (!destFoldername.Length()) {
		if (mDestFolderpath.InitCheck() == B_OK) {
			mDestFolderpath.Unset();
			return true;
		}
		return false;
	}
	BmString p( ThePrefs->GetString("MailboxPath") + "/" << destFoldername);
	BPath newPath( p.String(), NULL, true);
	if (newPath != DestFolderpath()) {
		mDestFolderpath = newPath;
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::ApplyFilter( BmRef<BmFilter> filter) {
	BmRef<BmMailFilter> filterJob = new BmMailFilter( Name(), filter.Get());
	filterJob->AddMail( this);
	filterJob->StartJobInThisThread( BmMailFilter::BM_EXECUTE_FILTER_IN_MEM);
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BNode backupNode;
	BNodeInfo backupInfo;
	BDirectory homeDir;
	status_t err;
	ssize_t res;
	char filenameBuf[B_FILE_NAME_LENGTH];
	BmString filename;
	BPath newHomePath;
	BEntry backupEntry;
	BmString backupName;
	BmString status;
	bigtime_t whenCreated;

	try {
		// Find out where mail shall be living:
		DestFolderpath();
							// init mDestFolderpath to default (mailbox/in)

		if (mEntry.InitCheck() == B_OK 
		|| mMailRef && mMailRef->InitCheck() == B_OK) {
			if (mEntry.InitCheck() != B_OK) {
				// mail has been read from disk, we recycle the old name
				if ((err = mEntry.SetTo( mMailRef->EntryRefPtr())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not create entry from mail-ref <") 
							<< mMailRef->Key() << ">\n\n Result: " << strerror(err)
					);
			}
			whenCreated = mMailRef->WhenCreated();
			status = mMailRef->Status();

			mEntry.GetName( filenameBuf);
			filename = filenameBuf;
			if ((err = mEntry.GetParent( &homeDir)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not get parent for mail <")
						<< filename << ">\n\n Result: " << strerror(err)
				);
			if ((err = newHomePath.SetTo( &homeDir, NULL)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not get path for homeDir of mail <")
						<< filename << ">\n\n Result: " << strerror(err)
				);
			// now create a backup-entry for this mail and find a unique
			// name for the backup-file...
			backupEntry = mEntry;
			backupName = filename + " (backup_by_beam)";
			// ...set the mime-type to something other than mail
			//    (in order to cause this mail to disappear from ref-view)...
			if ((err = backupNode.SetTo( &backupEntry)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not set node for backup mail\n\t<") 
						<< backupName << ">\n\n Result: " << strerror(err)
				);
			if ((err = backupInfo.SetTo( &backupNode)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not set node-info for backup mail\n\t<") 
						<< backupName << ">\n\n Result: " << strerror(err)
				);
			backupInfo.SetType( "text/x-email-backup");
			// ...and rename the old mail-file, leaving it as backup:
			if ((err = backupEntry.Rename( backupName.String())) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not backup mail <")
						<< filename << "> to <" << backupName 
						<< ">\n\n Result: " << strerror(err)
				);
		} else {
			// this mail is new, so first we find it's new home (a mail-folder)...
			BmString defaultPath( ThePrefs->GetString("MailboxPath") + "/" 
							+ (mOutbound ? BM_MAIL_FOLDER_OUT : BM_MAIL_FOLDER_IN));
			newHomePath.SetTo( defaultPath.String());
			// since mail is new, we initialize its creation-time...
			whenCreated = real_time_clock_usecs();
			// ...and the status:
			status = DefaultStatus();
		}

		// now check whether mail-filtering has decided that the mail shall live
		// in a specific folder:
		if (newHomePath != mDestFolderpath) {
			// try to file mail into a different destination-folder:
			if ((err = homeDir.SetTo( mDestFolderpath.Path())) != B_OK) {
				BmLogHandler::Log( "Filter", 
										 BmString("Could not file message into "
										 			 "mail-folder\n") 
										 	<< mDestFolderpath.Path()
											<< "\nError: " << strerror(err)
											<< "\n\nMessage will now be filed "
											<<	"into the folder\n" 
											<< newHomePath.Path());
			} else
				newHomePath = mDestFolderpath;
		}

		// finally we set homeDir to the (possibly new) folder that the mail
		// shall live in:
		if ((err = homeDir.SetTo( newHomePath.Path())) != B_OK) {
			// folder does not exists, we check if its a system folder
			BmString mbox( ThePrefs->GetString("MailboxPath") + "/");
			BmString newBox( newHomePath.Path());
			if (newBox ==  mbox + BM_MAIL_FOLDER_IN
			|| newBox == mbox + BM_MAIL_FOLDER_OUT
			|| newBox == mbox + BM_MAIL_FOLDER_DRAFT) {
				// yep, its a system folder, so we silently (re-)create it:
				create_directory( newHomePath.Path(), 0755);
				if ((err = homeDir.SetTo( newHomePath.Path())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not set directory to <") 
							<< newHomePath.Path() << ">\n\n Result: " 
							<< strerror(err)
					);
			} else
				BM_THROW_RUNTIME( 
					BmString("Could not set directory to <") 
						<< newHomePath.Path() << ">\n\n Result: " 
						<< strerror(err)
				);
		}
			
		// create entry for new mail-file:
		BmString newName;
		if (!filename.Length())
			// mail is new, we create a new filename for it and set the entry:
			newName = BmString(newHomePath.Path()) + "/" + CreateBasicFilename();
		else
			// reuse old name for mail that already lives on disk:
			newName = BmString(newHomePath.Path()) + "/" + filename;
		if ((err = mEntry.SetTo( newName.String())) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not create entry for mail-file <") 
					<< newName << ">\n\n Result: " << strerror(err)
			);
		mEntry.GetName( filenameBuf);
		filename = filenameBuf;

		// we create/open the new mailfile...
		if ((err = mailFile.SetTo( 
			&mEntry, 
			B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE
		)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not create mail-file\n\t<") 
					<< filename << ">\n\n Result: " << strerror(err)
			);
		// ...set the correct mime-type...
		if ((err = mailInfo.SetTo( &mailFile)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not set node-info for mail-file\n\t<") 
					<< filename << ">\n\n Result: " << strerror(err)
			);
		mailInfo.SetType( "text/x-email");
		// ...store all other attributes...
		StoreAttributes( mailFile, status, whenCreated);
		mHeader->StoreAttributes( mailFile);
		// ...and finally write the raw mail into the file:
		int32 len = mText.Length();
		if ((res = mailFile.Write( mText.String(), len)) < len) {
			if (res < 0) {
				BM_THROW_RUNTIME( BmString("Unable to write to mailfile <") 
											<< filename << ">\n\n Result: " 
											<< strerror(err));
			} else {
				BM_THROW_RUNTIME( BmString("Could not write complete mail to "
													"file.\nWrote ") 
											<< res << " bytes instead of " << len);
			}
		}
		if ((err = mailFile.Sync()) != B_OK)
			BM_THROW_RUNTIME( BmString("Unable to sync mailfile <") 
										<< filename << ">\n\n Result: " 
										<< strerror(err));
		// now remove the backup mail (if any):
		if (backupEntry.InitCheck() == B_OK
		&& (err = backupEntry.Remove()) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not remove mail-backup <") << backupName
					<< ">\n\n Result: " << strerror(err)
			);
		entry_ref eref;
		if ((err = mEntry.GetRef( &eref)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not get entry-ref for mail <") << filename
					<< ">.\n\n Result: " << strerror(err)
			);
		// now that the mail lives on the disk, we move it to trash, if requested:
		if (mMoveToTrash)
			::MoveToTrash( &eref, 1);
		// create a (new) mail-ref for the freshly saved mail:
		struct stat st;
		mEntry.GetStat( &st);
		mMailRef = BmMailRef::CreateInstance( eref, st);
		for( uint32 i=0; i<mBaseRefVect.size(); ++i) {
			mBaseRefVect[i]->MarkAs( mNewBaseStatus.String());
		}
		mBaseRefVect.clear();
	} catch( BM_error &e) {
		BM_SHOWERR(e.what());
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMail::StoreAttributes( BFile& mailFile, const BmString& status, 
										bigtime_t whenCreated) {
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
							  status.String(), status.Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_ACCOUNT, B_STRING_TYPE, 0, 
							  mAccountName.String(), mAccountName.Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_IDENTITY, B_STRING_TYPE, 0, 
							  mIdentityName.String(), mIdentityName.Length()+1);
	//
	if (mOutbound) {
		// write MAIL:flags in order to cooperate nicely with MDR:
		BmString status = Status();
		int32 flags = 0;
		if (status==BM_MAIL_STATUS_PENDING)
			flags = B_MAIL_PENDING | B_MAIL_SAVE;
		else if (status==BM_MAIL_STATUS_SENT)
			flags = B_MAIL_SENT;
		mailFile.WriteAttr( BM_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, 
								  &flags, sizeof(int32));
	}
	//
	int32 headerLength = HeaderLength();
	int32 contentLength = MAX( 0, mText.Length()-headerLength);
	
	mailFile.WriteAttr( BM_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, 
							  &headerLength, sizeof(int32));
	mailFile.WriteAttr( BM_MAIL_ATTR_CONTENT, B_INT32_TYPE, 0, 
							  &contentLength, sizeof(int32));
	//
	int32 hasAttachments = HasAttachments();
	mailFile.WriteAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, 
							  &hasAttachments, sizeof(hasAttachments));
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_MARGIN, B_INT32_TYPE, 0, 
							  &mRightMargin, sizeof(int32));
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_WHEN_CREATED, B_UINT64_TYPE, 0, 
							  &whenCreated, sizeof(whenCreated));
}

/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateBasicFilename() {
	static int32 counter = 1;
	BmString name = mHeader->GetFieldVal(BM_FIELD_SUBJECT);
	// we remove some illegal characters from filename, if present:
	name.ReplaceSet( "/`´:\"\\", "_");
	if (name.Length() > B_FILE_NAME_LENGTH-25)
		name.Truncate( B_FILE_NAME_LENGTH-25);
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now << "_" << counter++;
	return name;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::StartJob() {
	// try to open corresponding mail file, then read and parse mail contents...
	status_t err;
	BFile mailFile;
	
	if (!mMailRef || InitCheck() == B_OK) {
		// mail is illdefined or has already been initialized 
		// -> there's nothing left to do
		return true;
	}

	try {
		// N.B.: We skip any checks for the explicit read-mail-job, since
		//       in this mode we really, really want to read the mail now.
		bool skipChecks = mJobSpecifier == BM_READ_MAIL_JOB;
		if (!skipChecks) {
			// we take a little nap (giving the user time to navigate onwards),
			// after which we check if we should really read the mail:
			snooze( 50*1000);
			if (!ShouldContinue())
				return false;
		}

		entry_ref eref = mMailRef->EntryRef();
		BM_LOG2( BM_LogMailParse, 
					BmString("opening mail-file <") << eref.name << ">");
		for(  int i=0; 
				(err = mailFile.SetTo( &eref, B_READ_ONLY)) == B_BUSY; ++i) {
			if (i==100)
				throw BM_runtime_error( BmString("Node is locked too long for "
															"mail-file <") 
													<< eref.name << "> \n\nError:" 
													<< strerror(err));
			BM_LOG2( BM_LogMailTracking, 
						BmString("Node is locked for mail-file <") << eref.name 
							<< ">. We take a nap and try again...");
			snooze( 200*1000);
		}
		if (err != B_OK) {
			// mail-file doesn't exist anymore, most probably because 
			// the user has deleted it (in a multi-delete operation), 
			// we no longer throw, but return quietly
			return false;
		}
		
		// ...ok, mail-file found, we fetch the mail from it:
		BmString mailText;
		// read special attributes for mail-state...
		mailFile.ReadAttr( BM_MAIL_ATTR_MARGIN, B_INT32_TYPE, 0, 
								 &mRightMargin, sizeof(int32));
		// ...and read file contents:
		off_t mailSize;
		if ((err = mailFile.GetSize( &mailSize)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not get size of mail-file <") << eref.name 
					<< "> \n\nError:" << strerror(err)
			);
		BM_LOG2( BM_LogMailParse, 
					BmString("...should be reading ") << mailSize << " bytes");
		char* buf = mailText.LockBuffer( mailSize);
		off_t realSize = 0;
		const size_t blocksize = 65536;
		for(  int32 offs=0; 
				(skipChecks || ShouldContinue()) && offs < mailSize; ) {
			char* pos = buf+offs;
			ssize_t read = mailFile.Read( 
				pos, 
				mailSize-offs < blocksize 
					? mailSize-offs 
					: blocksize
			);
			BM_LOG3( BM_LogMailParse, 
						BmString("...read a block of ") << read << " bytes");
			if (read < 0)
				throw BM_runtime_error( BmString("Could not fetch mail from "
															"file\n\t<") 
													<< eref.name << ">\n\n Result: " 
													<< strerror(read));
			if (!read)
				break;
			realSize += read;
			offs += read;
		}
		if (!skipChecks && !ShouldContinue())
			return false;
		BM_LOG2( BM_LogMailParse, 
					BmString("...real size is ") << realSize << " bytes");
		buf[realSize] = '\0';
		mailText.UnlockBuffer( realSize);
		// we initialize the BmMail-internals from the plain text:
		BM_LOG2( BM_LogMailParse, BmString("initializing BmMail from msgtext"));
		mIdentityName = mMailRef->Identity();
		SetTo( mailText, mMailRef->Account());
		BM_LOG2( BM_LogMailParse, BmString("Done, mail is initialized"));
	} catch (BM_error &e) {
		BM_SHOWERR( e.what());
	}
	return InitCheck() == B_OK;
}

/*------------------------------------------------------------------------------*\
	ResyncFromDisk()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::ResyncFromDisk() {
	mInitCheck = B_NO_INIT;
	StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	QuoteText()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMail::QuoteText( const BmString& in, BmString& out, 
								 const BmString inQuoteString, int maxLineLen) {
	out = "";
	if (!in.Length())
		return maxLineLen;
	BmString quoteString;
	quoteString.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), 
												&inQuoteString);
	BmString qStyle = ThePrefs->GetString( "QuoteFormatting");
	if (qStyle == BM_QUOTE_AUTO_WRAP)
		return QuoteTextWithReWrap( in, out, quoteString, maxLineLen);
	BmString quote;
	BmString text;
	Regexx rx;
	rx.str( in);
	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	int modifiedMaxLen = maxLineLen;
	int maxTextLen;
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		BmString q(rx.match[i].atom[0]);
		quote.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), &q);
		if (qStyle == BM_QUOTE_SIMPLE) {
			// always respect maxLineLen, wrap when lines exceed right margin.
			// This results in a combing-effect when long lines are wrapped
			// around, producing a very short next line.
			maxTextLen 
				= MAX( 0, 
						 maxLineLen - quote.CountChars() - quoteString.CountChars());
		} else {
			// qStyle == BM_QUOTE_PUSH_MARGIN
			// push right margin for new quote-string, if needed, in effect 
			// leaving the mail-formatting intact more often (but possibly
			// exceeding 80 chars per line):
			maxTextLen = MAX( 0, maxLineLen - quote.CountChars());
		}
		text = rx.match[i].atom[1];
		int32 len = text.Length();
		// trim trailing spaces:
		while( len>0 && text[len-1]==' ')
			len--;
		text.Truncate( len);
		int32 newLen = AddQuotedText( text, out, quote, quoteString, maxTextLen);
		modifiedMaxLen = MAX( newLen, modifiedMaxLen);
	}
	// now remove trailing empty lines:
	BmString emptyLinesAtEndRX 
		= BmString("(?:") << "\\Q" << quoteString << "\\E" 
								<< "(" << "\\Q" << quote << "\\E" 
								<< ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", 
							Regexx::newline|Regexx::global|Regexx::noatom);
	return modifiedMaxLen;
}

/*------------------------------------------------------------------------------*\
	QuoteText()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMail::QuoteTextWithReWrap( const BmString& in, BmString& out, 
											  BmString quoteString, int maxLineLen) {
	out = "";
	if (!in.Length())
		return maxLineLen;
	Regexx rx;
	rx.str( in);
	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	BmString currQuote;
	BmString text;
	BmString line;
	BmString quote;
	Regexx rxl;
	int maxTextLen;
	int minLenForWrappedLine = ThePrefs->GetInt( "MinLenForWrappedLine", 50);
	bool lastWasSpecialLine = true;
	int32 lastLineLen = 0;
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		BmString q(rx.match[i].atom[0]);
		quote.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), &q);
		line = rx.match[i].atom[1];
		if ((line.CountChars() < minLenForWrappedLine && lastWasSpecialLine)
		|| rxl.exec( line, ThePrefs->GetString( "QuotingLevelEmptyLineRX", 
															 "^[ \\t]*$"))
		|| rxl.exec( line, ThePrefs->GetString( "QuotingLevelListLineRX", 
															 "^[*+\\-\\d]+.*?$"))) {
			if (i != 0) {
				maxTextLen = MAX( 0, 
							 			maxLineLen - currQuote.CountChars() 
							 				- quoteString.CountChars());
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = true;
		} else if (lastWasSpecialLine || currQuote != quote 
		|| lastLineLen < minLenForWrappedLine) {
			if (i != 0) {
				maxTextLen = MAX( 0, 
										maxLineLen - currQuote.CountChars() 
											- quoteString.CountChars());
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = false;
		}
		currQuote = quote;
		lastLineLen = line.CountChars();
		if (!text.Length())
			text = line;
		else {
			int32 len = text.Length();
			// trim trailing spaces:
			while( len>0 && text[len-1]==' ')
				len--;
			text.Truncate( len);
			text << " " << line;
		}
	}
	maxTextLen = MAX( 0, 
							maxLineLen - currQuote.CountChars() 
								- quoteString.CountChars());
	AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
	// now remove trailing empty lines:
	BmString emptyLinesAtEndRX 
		= BmString("(?:") << "\\Q" << quoteString << "\\E" 
								<< "(" << "\\Q" << currQuote << "\\E" 
								<< ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", 
							Regexx::newline|Regexx::global|Regexx::noatom);
	return maxLineLen;
}

/*------------------------------------------------------------------------------*\
	AddQuotedLine()
		-	
		-	N.B.: We use the character-count in order to determine line-lengths, 
			which for some charsets (e.g. iso-2022-jp) results in lines longer than
			78 *byte* hard-limit (it just respects a limit of 78 *characters*).
			This probably violates the RFC, but I believe it just makes more sense
			for the users (since characters is what they see on screen, not bytes).
\*------------------------------------------------------------------------------*/
int32 BmMail::AddQuotedText( const BmString& inText, BmString& out, 
									  const BmString& quote,
									  const BmString& quoteString,
								     int maxTextLen) {
	int32 modifiedMaxLen = 0;
	BmString tmp;
	BmString text;
	bool isUrl = false;
	Regexx rxUrl;
	maxTextLen = MAX( 0, maxTextLen);
	text.ConvertTabsToSpaces( ThePrefs->GetInt( "SpacesPerTab", 4), &inText);
	int32 charsLeft = text.CountChars();
	while( charsLeft > maxTextLen) {
		int32 wrapPos = B_ERROR;
		int32 idx=0;
		isUrl = rxUrl.exec(
			text, "^\\s*(https?://|ftp://|nntp://|file://|mailto:)", 
			Regexx::nocase
		);
		for(  int32 charCount=0; 
				charCount<maxTextLen || (isUrl && wrapPos==B_ERROR && text[idx]); 
			   ++charCount) {
			if (IS_UTF8_STARTCHAR(text[idx])) {
				idx++;
				while( IS_WITHIN_UTF8_MULTICHAR(text[idx]))
					idx++;
			} else {
				if (text[idx]==B_SPACE)
					wrapPos = idx+1;
				if (text[idx]=='\n')
					wrapPos = idx+1;
				idx++;
			}
		}
		text.MoveInto( tmp, 0, wrapPos!=B_ERROR ? wrapPos : idx);
		charsLeft -= tmp.CountChars();
		tmp.Prepend( quoteString + quote);
		modifiedMaxLen = MAX( tmp.CountChars(), modifiedMaxLen);
		out << tmp << "\n";
	}
	if (!inText.Length() || text.Length()) {
		tmp = quoteString + quote + text;
		modifiedMaxLen = MAX( tmp.CountChars(), modifiedMaxLen);
		out << tmp << "\n";
	}
	return modifiedMaxLen;
}
