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
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmMailRef.h"
#include "BmPopAccount.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmSignature.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

/********************************************************************************\
	BmMail
\********************************************************************************/

#define BM_REFKEY(x) (BString("MailModel_") << x->Inode())

/*------------------------------------------------------------------------------*\
	CreateInstance( mailref)
		-	constructs a mail from file
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateInstance( BmMailRef* ref) {
/*
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMail).name());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		BString key( BM_REFKEY( ref));
		BmRef<BmMail> mail( dynamic_cast<BmMail*>( proxy->FetchObject( key)));
		if (mail)
			return mail;
	}
*/
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
	,	mHeaderLength( 0)
	,	mBody( NULL)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( outbound)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
{
	BString emptyMsg = BString(BM_FIELD_MIME)+": 1.0\r\n";
	emptyMsg << "Content-Type: text/plain; charset=\"" 
				<< EncodingToCharset( ThePrefs->GetInt( "DefaultEncoding"))
				<<	"\"\r\n";
	emptyMsg << BM_FIELD_DATE << ": " << TimeToString( time( NULL), 
																		"%a, %d %b %Y %H:%M:%S %z");
	emptyMsg << "\r\n\r\n";
	SetTo( emptyMsg, "");
	if (outbound) {
		// if outbound, stuff basic info (from-address, smtp-account, sig) into mail:
		BmRef<BmPopAccount> accRef = ThePopAccountList->DefaultAccount();
		if (accRef) {
			SetFieldVal( BM_FIELD_FROM, accRef->GetFromAddress());
			mAccountName = accRef->SMTPAccount();
			SetSignatureByName( accRef->SignatureName(), DefaultEncoding());
		}
	}
}

/*------------------------------------------------------------------------------*\
	BmMail( msgText, msgUID, account)
		-	constructor
		-	creates message with given text and receiver-account
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BString &msgText, const BString account) 
	:	inherited( "MailModel_dummy")
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( NULL)
	,	mMailRef( NULL)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
{
	SetTo( msgText, account);
}
	
/*------------------------------------------------------------------------------*\
	BmMail( mailref)
		-	constructor via a mail-ref (from file)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BmMailRef* ref) 
	:	inherited( BM_REFKEY( ref))
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( NULL)
	,	mMailRef( ref)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
{
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
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMail::SetTo( const BString &text, const BString account) {
	BString msgText;
	ConvertLinebreaksToCRLF( text, msgText);

	// find end of header (and start of body):
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR)
		throw BM_mail_format_error("BmMail: Could not determine borderline between header and text of message");

	headerLen += 2;							// don't include separator-line in header-string
	mHeaderLength = headerLen;

	mText.Adopt( msgText);					// take over the msg-string
	mAccountName = account;

	BString header;
	header.SetTo( mText, headerLen);
	mHeader = new BmMailHeader( header, this);

	mBody = new BmBodyPartList( this);
	mBody->ParseMail();

	mInitCheck = B_OK;
}
	
/*------------------------------------------------------------------------------*\
	SetNewHeader()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetNewHeader( const BString& headerStr) {
	BString newMsgText;
	ConvertLinebreaksToCRLF( headerStr, newMsgText);
	int32 len = newMsgText.Length();
	if (newMsgText[len-1] != '\n')
		newMsgText << "\r\n";
	newMsgText << mText.String()+mHeaderLength;
	SetTo( newMsgText, mAccountName);
	Store();
	StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	SetSignatureByName( sigName, encoding)
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetSignatureByName( const BString sigName, int32 encoding) {
	if (!mBody || mSignatureName==sigName)
		return;
	mSignatureName = sigName;
	BString encodedSig;
	ConvertFromUTF8( encoding, 
						  TheSignatureList->GetSignatureStringFor( sigName), encodedSig);
	mBody->Signature( encodedSig);
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
			if (mEntry.InitCheck() == B_OK)
				mEntry.GetRef( &eref);
			else
				return;
			(err = mailNode.SetTo( &eref)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create node for current mail-file.\n\n Result: ") << strerror(err));
			mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, status, strlen( status)+1);
		}
	} catch( exception &e) {
		BM_SHOWERR(e.what());
	}
}
	
/*------------------------------------------------------------------------------*\
	Status()
		-	
\*------------------------------------------------------------------------------*/
const BString BmMail::Status() const { 
	return mMailRef ? mMailRef->Status() : DefaultStatus();
}

/*------------------------------------------------------------------------------*\
	DefaultStatus()
		-	
\*------------------------------------------------------------------------------*/
const BString BmMail::DefaultStatus() const { 
	return mOutbound ? BM_MAIL_STATUS_DRAFT : BM_MAIL_STATUS_NEW;
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
const BString& BmMail::GetFieldVal( const BString fieldName) {
	if (mHeader)
		return mHeader->GetFieldVal( fieldName);
	else
		return BM_DEFAULT_STRING;
}

/*------------------------------------------------------------------------------*\
	GetStrippedFieldVal()
	-	
\*------------------------------------------------------------------------------*/
BString BmMail::GetStrippedFieldVal( const BString fieldName) {
	if (mHeader)
		return mHeader->GetStrippedFieldVal( fieldName);
	else
		return BM_DEFAULT_STRING;
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetFieldVal( const BString fieldName, const BString value) {
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
void BmMail::RemoveField( const BString fieldName) {
	mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	CreateInlineForward()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateInlineForward( bool withAttachments, const BString selectedText) {
	BmRef<BmMail> newMail = new BmMail( true);
	// massage subject, if neccessary:
	BString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));
	newMail->AddPartsFromMail( this, withAttachments, BM_IS_FORWARD, selectedText);
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
		newMail->Body()->AddAttachmentFromRef( mMailRef->EntryRefPtr());
	// massage subject, if neccessary:
	BString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateForwardSubjectFor( subject));
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_FORWARDED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateReply()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateReply( bool replyToAll, const BString selectedText) {
	BmRef<BmMail> newMail = new BmMail( true);
	// copy old message ID into in-reply-to and references fields:
	BString messageID = GetFieldVal( BM_FIELD_MESSAGE_ID);
	newMail->SetFieldVal( BM_FIELD_IN_REPLY_TO, messageID);
	newMail->SetFieldVal( BM_FIELD_REFERENCES, GetFieldVal( BM_FIELD_REFERENCES) + " " + messageID);
	// fill address information:
	BString newTo = Header()->DetermineOriginator();
	newMail->SetFieldVal( BM_FIELD_TO, newTo);
	// fetch info about encoding from old mail:
	int32 encoding = DefaultEncoding();
	// Since we are replying, we generate the new mail's from-address 
	// from the received mail's to-info:
	BmRef<BmListModelItem> accRef = ThePopAccountList->FindItemByKey( AccountName());
	BmPopAccount* acc = dynamic_cast< BmPopAccount*>( accRef.Get());
	BString receivingAddr;
	if (acc) {
		receivingAddr = Header()->DetermineReceivingAddrFor( acc);
		if (!receivingAddr.Length())
			receivingAddr = acc->GetFromAddress();
		newMail->SetFieldVal( BM_FIELD_FROM, receivingAddr);
		newMail->SetSignatureByName( acc->SignatureName(), encoding);
		newMail->AccountName( acc->SMTPAccount());
	}
	// if we are replying to all, we may need to include more addresses:
	if (replyToAll) {
		BString newCc;
		if (ThePrefs->GetBool( "UseResentFieldsInReply", false))
			newCc = GetFieldVal( BM_FIELD_RESENT_CC);
		else
			newCc = GetFieldVal( BM_FIELD_CC);
		newMail->SetFieldVal( BM_FIELD_CC, newCc);
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_CC, receivingAddr);
		BString additionalCc;
		if (ThePrefs->GetBool( "UseResentFieldsInReply", false))
			additionalCc = GetFieldVal( BM_FIELD_RESENT_TO);
		else
			additionalCc = GetFieldVal( BM_FIELD_TO);
		newMail->Header()->AddFieldVal( BM_FIELD_CC, additionalCc);
		newMail->Header()->RemoveAddrFieldVal( BM_FIELD_CC, receivingAddr);
	}
	// massage subject, if neccessary:
	BString subject = GetFieldVal( BM_FIELD_SUBJECT);
	newMail->SetFieldVal( BM_FIELD_SUBJECT, CreateReplySubjectFor( subject));
	// copy and quote text-body:
	BmRef<BmBodyPart> textBody( mBody->EditableTextBody());
	BString text;
	if (selectedText.Length())
		ConvertFromUTF8( encoding, selectedText, text);
	else
		text = textBody ? textBody->DecodedData() : "";
	BString quotedText;
	int32 newMaxLineLen = QuoteText( text, quotedText,
				 								ThePrefs->GetString( "QuotingString"),
												ThePrefs->GetInt( "MaxLineLen"));
	newMail->Body()->SetEditableText( CreateReplyIntro() << "\n" << quotedText, 
												 encoding);
	newMail->BumpRightMargin( newMaxLineLen);
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_REPLIED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateResend()
	-	
\*------------------------------------------------------------------------------*/
BmRef<BmMail> BmMail::CreateRedirect() {
	BmRef<BmMail> newMail = new BmMail( true);
	BString msgText( mText);
	newMail->SetTo( msgText, "");
	if (newMail->IsRedirect()) {
		// oops, mail already has been redirected, we clobber the existing Resent-fields,
		// since STD11 says multiple Resent-fields result in undefined behaviour:
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
								 TimeToString( time( NULL), "%a, %d %b %Y %H:%M:%S %z"));
	newMail->SetBaseMailInfo( MailRef(), BM_MAIL_STATUS_REDIRECTED);
	return newMail;
}

/*------------------------------------------------------------------------------*\
	CreateReplySubjectFor()
	-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateReplySubjectFor( const BString subject) {
	BString isReplyRX = ThePrefs->GetString( "ReplySubjectRX", "^\\s*(Re|Aw)(\\[\\d+\\])?:");
	Regexx rx;
	if (!rx.exec( subject, isReplyRX, Regexx::nocase|Regexx::nomatch)) {
		BString subjectStr = ThePrefs->GetString( "ReplySubjectStr", "Re: %s");
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
BString BmMail::CreateForwardSubjectFor( const BString subject) {
	BString isForwardRX = ThePrefs->GetString( "ForwardSubjectRX", "^\\s*\\[?\\s*Fwd(\\[\\d+\\])?:");
	Regexx rx;
	if (!rx.exec( subject, isForwardRX, Regexx::nocase|Regexx::nomatch)) {
		BString subjectStr = ThePrefs->GetString( "ForwardSubjectStr", "[Fwd: %s]");
		subjectStr = rx.replace( subjectStr, "%s", subject, 
										 Regexx::nocase|Regexx::global|Regexx::noatom);
		return subjectStr;
	}
	return subject;
}

/*------------------------------------------------------------------------------*\
	CreateReplyIntro()
	-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateReplyIntro() {
	Regexx rx;
	BString intro = ThePrefs->GetString( "ReplyIntroStr", "On %D at %T, %F wrote:");
	intro = rx.replace( intro, "%D", TimeToString( mMailRef->When(), "%Y-%m-%d"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%T", TimeToString( mMailRef->When(), "%X [%z]"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BmAddress fromAddr = Header()->DetermineOriginator();
	intro = rx.replace( intro, "%F", 
							  fromAddr.HasPhrase() ? fromAddr.Phrase() : fromAddr.AddrSpec(), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BString convertedIntro;
	ConvertFromUTF8( DefaultEncoding(), intro, convertedIntro);
	return convertedIntro;
}

/*------------------------------------------------------------------------------*\
	CreateForwardIntro()
	-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateForwardIntro() {
	Regexx rx;
	BString intro = ThePrefs->GetString( "ForwardIntroStr", "On %D at %T, %F wrote:");
	intro = rx.replace( intro, "%D", TimeToString( mMailRef->When(), "%Y-%m-%d"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	intro = rx.replace( intro, "%T", TimeToString( mMailRef->When(), "%X [%z]"), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BmAddress fromAddr = Header()->DetermineOriginator();
	intro = rx.replace( intro, "%F", 
							  fromAddr.HasPhrase() ? fromAddr.Phrase() : fromAddr.AddrSpec(), 
							  Regexx::nocase|Regexx::global|Regexx::noatom);
	BString convertedIntro;
	ConvertFromUTF8( DefaultEncoding(), intro, convertedIntro);
	return convertedIntro;
}

/*------------------------------------------------------------------------------*\
	AddAttachmenstFromRef()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::AddAttachmentFromRef( const entry_ref* ref) {
	if (mBody)
		mBody->AddAttachmentFromRef( ref);
}

/*------------------------------------------------------------------------------*\
	AddPartsFromMail()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::AddPartsFromMail( BmRef<BmMail> mail, bool withAttachments, 
										 bool isForward,
										 const BString selectedText) {
	if (!mail || !mail->Body() || !mBody)
		return;
	// copy and quote text-body:
	BmRef<BmBodyPart> newTextBody( mBody->EditableTextBody());
	BString oldText = newTextBody ? newTextBody->DecodedData() : "";
	BmRef<BmBodyPart> textBody( mail->Body()->EditableTextBody());
	// copy info about encoding from old into new mail:
	int32 encoding = mail->DefaultEncoding();
	BString charset = EncodingToCharset( encoding);
	BString newText;
	if (textBody) {
		charset = textBody->Charset();
		if (selectedText.Length())
			ConvertFromUTF8( encoding, selectedText, newText);
		else
			newText = textBody->DecodedData();
	} else
		newText = selectedText;
	BString quotedText;
	int32 newLineLen = QuoteText( newText, quotedText,
											ThePrefs->GetString( "QuotingString"),
											ThePrefs->GetInt( "MaxLineLen"));
	BumpRightMargin( newLineLen);
	BString intro( isForward 
							? mail->CreateForwardIntro() 
							: mail->CreateReplyIntro() 
						<< "\n");
	mBody->SetEditableText( oldText + "\n" + intro + quotedText, CharsetToEncoding( charset));
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
		bool doNotAttachVCards = ThePrefs->GetBool( "DoNotAttachVCardsToForward", true);
		for( ; iter != end; iter++) {
			BmBodyPart* bodyPart = dynamic_cast< BmBodyPart*>( iter->second.Get());
			if (textBody != bodyPart) {
				if (doNotAttachVCards && bodyPart->MimeType().ICompare( "text/x-vcard") == 0)
					continue;
				BmBodyPart* copiedBody = new BmBodyPart( *bodyPart);
				mBody->AddItemToList( copiedBody);
			}
		}
	}
	AddBaseMailRef( mail->MailRef());
}

/*------------------------------------------------------------------------------*\
	ConstructRawText()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ConstructRawText( const BString& editedText, int32 encoding,
										 const BString smtpAccount) {
	BString msgText;
	mText.Truncate( 0);
	mAccountName = smtpAccount;
	if (!mHeader->ConstructRawText( msgText, encoding))
		return false;
	mBody->SetEditableText( editedText, encoding);
	if (!mBody->ConstructBodyForSending( msgText))
		return false;
	BM_LOG3( BM_LogMailParse, BString("CONSTRUCTED MSG: \n-----START--------\n") << msgText << "\n-----END----------");
	SetTo( msgText, smtpAccount);
	return mInitCheck == B_OK;
}

/*------------------------------------------------------------------------------*\
	DefaultEncoding()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMail::DefaultEncoding() const {
	uint32 defEncoding;
	if (mBody && (defEncoding = mBody->DefaultEncoding())!=BM_UNKNOWN_ENCODING)
		return defEncoding;
	return B_ISO1_CONVERSION;
}

/*------------------------------------------------------------------------------*\
	SetBaseMailInfo( ref, newStatus)
		-	
\*------------------------------------------------------------------------------*/
void BmMail::SetBaseMailInfo( BmMailRef* ref, const BString newStatus) {
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
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BDirectory homeDir;
	BDirectory tmpDir;
	BPath tmpPath;
	status_t err;
	ssize_t res;
	char basicFilename[B_FILE_NAME_LENGTH];

	try {
		// Find out where mail shall be living.
		// N.B.: Since we have a node-monitor watching our actions within the mail-folders,
		//			we create all mails in a temp-folder (outside the mail-hierarchy, so that
		//			the node-monitor doesn't notice), write out the mail-file completely, and
		//			then move the complete mail-file into the mail-folder hierarchy.
		//			This way we can ensure that the node-monitor does not trigger on a
		//			mail-file before it is complete (which may result in files appearing to
		//			have zero-length or similar peculiarities).
		//
		// So: first, we determine the tmp-folder where the mail is being created:
		(err = find_directory( B_COMMON_TEMP_DIRECTORY, &tmpPath, true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not find tmp-directory on this system") << "\n\n Result: " << strerror(err));
		(err = tmpDir.SetTo( tmpPath.Path())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not find tmp-directory on this system") << "\n\n Result: " << strerror(err));

		if (mEntry.InitCheck() == B_OK) {
			// mail has just been written to disk, we recycle the current entry, but move 
			// it to the temp-folder:
			mEntry.GetName( basicFilename);
			(err = mEntry.GetParent( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not get parent for mail <")<<basicFilename<<">\n\n Result: " << strerror(err));
			(err = mEntry.MoveTo( &tmpDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not move mail <")<<basicFilename<<"> to tmp-folder\n\n Result: " << strerror(err));
		} else if (mMailRef && mMailRef->InitCheck() == B_OK) {
			// mail has been read from disk, we recycle the old name, but move 
			// it to the temp-folder:
			(err = mEntry.SetTo( mMailRef->EntryRefPtr())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry from mail-ref <") << mMailRef->Key() << ">\n\n Result: " << strerror(err));
			(err = mEntry.GetParent( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not get parent for mail <") << mMailRef->Key() << ">\n\n Result: " << strerror(err));
			(err = mEntry.MoveTo( &tmpDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not move mail <") << mMailRef->Key() << "> to tmp-folder\n\n Result: " << strerror(err));
			mEntry.GetName( basicFilename);
		} else {
			// mail is new, we create a new filename for it, and create the entry 
			// in the temp-folder::
			BString homePath  = ThePrefs->GetString("MailboxPath") 
									+ (mOutbound ? "/out" : "/in");
			create_directory( homePath.String(), 0755);
			(err = homeDir.SetTo( homePath.String())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not set directory to <") << homePath << ">\n\n Result: " << strerror(err));
			BString newName  = BString(tmpPath.Path()) + "/" + CreateBasicFilename();
			(err = mEntry.SetTo( newName.String())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-file <") << newName << ">\n\n Result: " << strerror(err));
			mEntry.GetName( basicFilename);
		}
			
		// we create the new mailfile...
		(err = mailFile.SetTo( &mEntry, B_WRITE_ONLY | B_ERASE_FILE | B_CREATE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		// ...set the correct mime-type...
		(err = mailInfo.SetTo( &mailFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not set node-info for mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		mailInfo.SetType( "text/x-email");
		// ...store all other attributes...
		StoreAttributes( mailFile);
		mHeader->StoreAttributes( mailFile);
		// ...and finally write the raw mail into the file:
		int32 len = mText.Length();
		if ((res = mailFile.Write( mText.String(), len)) < len) {
			if (res < 0) {
				BM_THROW_RUNTIME( BString("Unable to write to mailfile <") << basicFilename << ">\n\n Result: " << strerror(err));
			} else {
				BM_THROW_RUNTIME( BString("Could not write complete mail to file.\nWrote ") << res << " bytes instead of " << len);
			}
		}
		mailFile.Sync();
		// now move mail to it's real home:
		(err = mEntry.MoveTo( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not move mail <")<<basicFilename<<"> to home-folder\n\n Result: " << strerror(err));
		if (!mMailRef) {
			// mail has been freshly created, we add a mail-ref for it:
			entry_ref eref;
			struct stat st;
			mEntry.GetRef( &eref);
			mEntry.GetStat( &st);
			mMailRef = BmMailRef::CreateInstance( NULL, eref, st);
		} else
			mMailRef->ResyncFromDisk();
		for( uint32 i=0; i<mBaseRefVect.size(); ++i) {
			mBaseRefVect[i]->MarkAs( mNewBaseStatus.String());
		}
		mBaseRefVect.clear();
		StartJobInThisThread();
	} catch( exception &e) {
		BM_SHOWERR(e.what());
		return false;
	}

	return true;
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMail::StoreAttributes( BFile& mailFile) {
	//
	BString st = Status();
	mailFile.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, st.String(), st.Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_ACCOUNT, B_STRING_TYPE, 0, mAccountName.String(), mAccountName.Length()+1);
	//
	int32 headerLength, contentLength;
	if ((headerLength = mText.FindFirst("\r\n\r\n")) != B_ERROR) {
		headerLength+=2;
		contentLength = mText.Length()-headerLength;
	} else {
		headerLength = mText.Length();
		contentLength = 0;
	}
	mHeaderLength = headerLength;
	mailFile.WriteAttr( BM_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &headerLength, sizeof(int32));
	mailFile.WriteAttr( BM_MAIL_ATTR_CONTENT, B_INT32_TYPE, 0, &contentLength, sizeof(int32));
	//
	int32 hasAttachments = HasAttachments();
	mailFile.WriteAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, &hasAttachments, sizeof(hasAttachments));
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_MARGIN, B_INT32_TYPE, 0, &mRightMargin, sizeof(int32));
}

/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateBasicFilename() {
	static int32 counter = 1;
	BString name = mHeader->GetFieldVal(BM_FIELD_SUBJECT);
	int32 nl=name.Length();
	if (nl) {
		// we remove some illegal characters from filename, if present:
		char* buf = name.LockBuffer( nl);
		unsigned char c;
		BString illegalChars = "/`´:\"\\";
		for( int i=0; i<nl; ++i) {
			c = buf[i];
			if (c < 32 || illegalChars.FindFirst( c)!=B_ERROR)
				buf[i] = '_';
		}
		name.UnlockBuffer( nl);
	}
	if (nl > B_FILE_NAME_LENGTH-25)
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
		// mail is illdefined or has already been initialized -> there's nothing left to do
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
		BM_LOG2( BM_LogMailParse, BString("opening mail-file <") << eref.name << ">");
		for( int i=0; (err = mailFile.SetTo( &eref, B_READ_ONLY)) == B_BUSY; ++i) {
			if (i==100)
				throw BM_runtime_error( BString("Node is locked too long for mail-file <") << eref.name << "> \n\nError:" << strerror(err));
			BM_LOG2( BM_LogMailTracking, BString("Node is locked for mail-file <") << eref.name << ">. We take a nap and try again...");
			snooze( 200*1000);
		}
		if (err != B_OK)
			throw BM_runtime_error(BString("Could not open mail-file <") << eref.name << "> \n\nError:" << strerror(err));
		
		// ...ok, mail-file found, we fetch the mail from it:
		BString mailText;
		// read special attributes for mail-state...
		mailFile.ReadAttr( BM_MAIL_ATTR_MARGIN, B_INT32_TYPE, 0, &mRightMargin, sizeof(int32));
		// ...and read file contents:
		off_t mailSize;
		(err = mailFile.GetSize( &mailSize)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get size of mail-file <") << eref.name << "> \n\nError:" << strerror(err));
		BM_LOG2( BM_LogMailParse, BString("...should be reading ") << mailSize << " bytes");
		char* buf = mailText.LockBuffer( mailSize+1);
		off_t realSize = 0;
		const size_t blocksize = 65536;
		for( int32 offs=0; (skipChecks || ShouldContinue()) && offs < mailSize; ) {
			char* pos = buf+offs;
			ssize_t read = mailFile.Read( pos, mailSize-offs < blocksize ? mailSize-offs : blocksize);
			BM_LOG3( BM_LogMailParse, BString("...read a block of ") << read << " bytes");
			if (read < 0)
				throw BM_runtime_error( BString("Could not fetch mail from file\n\t<") << eref.name << ">\n\n Result: " << strerror(read));
			if (!read)
				break;
			realSize += read;
			offs += read;
		}
		if (!skipChecks && !ShouldContinue())
			return false;
		BM_LOG2( BM_LogMailParse, BString("...real size is ") << realSize << " bytes");
		buf[realSize] = '\0';
		mailText.UnlockBuffer( realSize);
		// we initialize the BmMail-internals from the plain text:
		BM_LOG2( BM_LogMailParse, BString("initializing BmMail from msgtext"));
		SetTo( mailText, mMailRef->Account());
		BM_LOG2( BM_LogMailParse, BString("Done, mail is initialized"));
		mInitCheck = B_OK;
	} catch (exception &e) {
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
int32 BmMail::QuoteText( const BString& in, BString& out, BString inQuoteString, 
								 int maxLineLen) {
	out = "";
	if (!in.Length())
		return maxLineLen;
	BString quoteString;
	ConvertTabsToSpaces( inQuoteString, quoteString);
	BString qStyle = ThePrefs->GetString( "QuoteFormatting");
	if (qStyle == BM_QUOTE_AUTO_WRAP)
		return QuoteTextWithReWrap( in, out, quoteString, maxLineLen);
	BString quote;
	BString text;
	Regexx rx;
	rx.str( in);
	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	int modifiedMaxLen = maxLineLen;
	int maxTextLen;
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		ConvertTabsToSpaces( rx.match[i].atom[0], quote);
		if (qStyle == BM_QUOTE_SIMPLE) {
			// always respect maxLineLen, wrap when lines exceed right margin.
			// This results in a combing-effect when long lines are wrapped
			// around, producing a very short next line.
			maxTextLen = maxLineLen - quote.Length() - quoteString.Length();
		} else {
			// qStyle == BM_QUOTE_PUSH_MARGIN
			// push right margin for new quote-string, if needed, in effect leaving 
			// the mail-formatting intact more often (but possibly exceeding 80 chars 
			// per line):
			maxTextLen = maxLineLen - quote.Length();
		}
		text = rx.match[i].atom[1];
		int32 len = text.Length();
		while( len>0 && text[len-1]==' ')
			len--;
		text.Truncate( len);
		int32 newLen = AddQuotedText( text, out, quote, quoteString, maxTextLen);
		modifiedMaxLen = MAX( newLen, modifiedMaxLen);
	}
	BString emptyLinesAtEndRX = BString("(?:") << quoteString << "(" << quote << ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", Regexx::newline|Regexx::global|Regexx::noatom);
	return modifiedMaxLen;
}

/*------------------------------------------------------------------------------*\
	QuoteText()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMail::QuoteTextWithReWrap( const BString& in, BString& out, 
											  BString quoteString, int maxLineLen) {
	out = "";
	if (!in.Length())
		return maxLineLen;
	Regexx rx;
	rx.str( in);
	rx.expr( ThePrefs->GetString( "QuotingLevelRX"));
	BString currQuote;
	BString text;
	BString line;
	BString quote;
	Regexx rxl;
	int maxTextLen;
	int minLenForWrappedLine = ThePrefs->GetInt( "MinLenForWrappedLine", 50);
	bool lastWasSpecialLine = true;
	int32 lastLineLen = 0;
	int32 count = rx.exec( Regexx::study | Regexx::global | Regexx::newline);
	for( int32 i=0; i<count; ++i) {
		ConvertTabsToSpaces( rx.match[i].atom[0], quote);
		line = rx.match[i].atom[1];
		if ((line.Length() < minLenForWrappedLine && lastWasSpecialLine)
		|| rxl.exec( line, ThePrefs->GetString( "QuotingLevelEmptyLineRX", "^[ \\t]*$"))
		|| rxl.exec( line, ThePrefs->GetString( "QuotingLevelListLineRX", "^[*+\\-\\d]+.*?$"))) {
			if (i != 0) {
				maxTextLen = maxLineLen - currQuote.Length() - quoteString.Length();
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = true;
		} else if (lastWasSpecialLine || currQuote != quote || lastLineLen < minLenForWrappedLine) {
			if (i != 0) {
				maxTextLen = maxLineLen - currQuote.Length() - quoteString.Length();
				AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
				text.Truncate(0);
			}
			lastWasSpecialLine = false;
		}
		currQuote = quote;
		lastLineLen = line.Length();
		if (!text.Length())
			text = line;
		else {
			int32 len = text.Length();
			while( len>0 && text[len-1]==' ')
				len--;
			text.Truncate( len);
			text << " " << line;
		}
	}
	maxTextLen = maxLineLen - currQuote.Length() - quoteString.Length();
	AddQuotedText( text, out, currQuote, quoteString, maxTextLen);
	BString emptyLinesAtEndRX = BString("(?:") << quoteString << "(" << currQuote << ")?[ \\t]*\\n)+\\z";
	out = rx.replace( out, emptyLinesAtEndRX, "", Regexx::newline|Regexx::global|Regexx::noatom);
	return maxLineLen;
}

/*------------------------------------------------------------------------------*\
	AddQuotedLine()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMail::AddQuotedText( const BString& inText, BString& out, 
									  const BString& quote,
									  const BString& quoteString,
								     int maxTextLen) {
	int32 modifiedMaxLen = 0;
	BString tmp;
	BString text;
	ConvertTabsToSpaces( inText, text);
	while( text.Length() > maxTextLen) {
		int32 spcPos = text.FindLast( " ", maxTextLen);
		if (spcPos == B_ERROR)
			spcPos = maxTextLen;
		else if (spcPos < maxTextLen)
			spcPos++;
		text.MoveInto( tmp, 0, spcPos);
		tmp.Prepend( quoteString + quote);
		modifiedMaxLen = MAX( tmp.Length(), modifiedMaxLen);
		out << tmp << "\n";
	}
	tmp = quoteString + quote + text;
	modifiedMaxLen = MAX( tmp.Length(), modifiedMaxLen);
	out << tmp << "\n";
	return modifiedMaxLen;
}
