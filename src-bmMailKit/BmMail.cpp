/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <algorithm>

#include <Directory.h>
#include <FindDirectory.h>

#include "split.hh"
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
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailHeader.h"
#include "BmMailRef.h"
#include "BmPrefs.h"
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
const char* BM_MAIL_ATTR_CLASSIFICATION = "MAIL:classification";
const char* BM_MAIL_ATTR_RATIO_SPAM		= "MAIL:ratio_spam";
//
const char* BM_MAIL_ATTR_IDENTITY		= "MAIL:beam/identity";
const char* BM_MAIL_ATTR_MARGIN	 		= "MAIL:beam/margin";
const char* BM_MAIL_ATTR_WHEN_CREATED = "MAIL:beam/when-created";
const char* BM_MAIL_ATTR_IMAP_UID	 	= "MAIL:beam/imap-uid";

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
const char* BM_FIELD_X_BEENTHERE			= "X-Beenthere";
const char* BM_FIELD_X_LIST				= "X-List";
const char* BM_FIELD_X_MAILER				= "X-Mailer";
const char* BM_FIELD_X_PRIORITY			= "X-Priority";

const char* BM_MAIL_STATUS_DRAFT			= "Draft";
const char* BM_MAIL_STATUS_ERROR			= "Error";
const char* BM_MAIL_STATUS_FORWARDED	= "Forwarded";
const char* BM_MAIL_STATUS_NEW			= "New";
const char* BM_MAIL_STATUS_PENDING		= "Pending";
const char* BM_MAIL_STATUS_READ			= "Read";
const char* BM_MAIL_STATUS_REDIRECTED	= "Redirected";
const char* BM_MAIL_STATUS_REPLIED		= "Replied";
const char* BM_MAIL_STATUS_SENT			= "Sent";

const char* BM_MAIL_CLASS_SPAM			= "Spam";
const char* BM_MAIL_CLASS_TOFU			= "Genuine";

static BmString BM_MAILKEY( const BmMailRef* ref)
{
	return BmString("Mail_") << ref->NodeRef().node;
}

// #pragma mark - Initialization
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
	BmString key( BM_MAILKEY( ref));
	BmRef<BmMail> mail( 
		dynamic_cast<BmMail*>( 
			BmRefObj::FetchObject( typeid(BmMail).name(), key)
		)
	);
	if (mail)
		return mail;
	else
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
	,	mRatioSpam( BmMailRef::UNKNOWN_RATIO)
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
			SetupFromIdentityAndRecvAddr( identRef.Get(), 
													identRef->GetFromAddress());
		}
	}
}

/*------------------------------------------------------------------------------*\
	BmMail( msgText, msgUID, account)
		-	constructor
		-	creates message with given text and receiver-account
\*------------------------------------------------------------------------------*/
BmMail::BmMail( const BmString &msgText, const BmString account) 
	:	inherited( "MailModel_dummy")
	,	mHeader( NULL)
	,	mBody( NULL)
	,	mMailRef( NULL)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
	,	mRightMargin( ThePrefs->GetInt( "MaxLineLen"))
	,	mMoveToTrash( false)
	,	mRatioSpam( BmMailRef::UNKNOWN_RATIO)
{
	SetTo( msgText, account);

	// get default-identity corresponding to given account:
	BmRef<BmIdentity> identRef 
		= TheIdentityList->FindIdentityForRecvAccount( account);
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
	,	mClassification( ref ? ref->Classification() : NULL)
	,	mRatioSpam( ref ? ref->RatioSpam() : BmMailRef::UNKNOWN_RATIO)
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
	SetTo( msgText, account)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was 
			received from
\*------------------------------------------------------------------------------*/
void BmMail::SetTo( const BmString &_text, const BmString account) {
	BmString text;
	BM_LOG2( BM_LogMailParse, "Converting Linebreaks to CRLF...");
		// take care to remove all binary nulls
	text.ConvertLinebreaksToCRLF(&_text);
	text.ReplaceAll( 0, 32);
	BM_LOG2( BM_LogMailParse, "done (Converting Linebreaks to CRLF)");

	// find end of header (and start of body):
	int32 headerLen = text.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR)
		// mail consists of the header only:
		headerLen = text.Length();
	else
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
	
// #pragma mark - Loading
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
		if (!buf)
			throw BM_runtime_error( BmString("Not enough memory for mail from "
														"file\n\t<") << eref.name << ">");
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
		if (realSize > mailSize)
			throw BM_runtime_error( BmString("Real size is ") << realSize 
							<< " bytes but expected size was only " << mailSize 
							<< " bytes!?!");
		buf[realSize] = '\0';
		mailText.UnlockBuffer( realSize);
		// take care to remove all binary nulls:
		mailText.ReplaceAll( 0, 32);
		// we initialize the BmMail-internals from the plain text:
		BM_LOG2( BM_LogMailParse, BmString("initializing BmMail from msgtext"));
		mIdentityName = mMailRef->Identity();
		mImapUID = mMailRef->ImapUID();
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
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailRef* BmMail::MailRef() const
{
	return mMailRef.Get();
}

// #pragma mark - Storing
/*------------------------------------------------------------------------------*\
	Store()
		-	determines where this mail should be living and the stores it there
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	status_t err = B_NO_INIT;
	BmString status;
	bigtime_t whenCreated;
	BmString filename;
	BEntry backupEntry;
	BDirectory destDir;

	try {
		// Find out where mail shall be living:
		BmRef<BmMailFolder> destFolder = DestFolder();
		if (destFolder)
			destDir.SetTo(&destFolder->NodeRef());

		if (mMailRef && mMailRef->InitCheck() == B_OK) {
			// mail has been read from disk, we re-use the old entry_ref
			if (mEntry.InitCheck() != B_OK) {
				if ((err = mEntry.SetTo( mMailRef->EntryRefPtr())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not create entry from mail-ref <") 
							<< mMailRef->Key() << ">\n\n Result: " << strerror(err)
					);
			}
			whenCreated = mMailRef->WhenCreated();
			status = mMailRef->Status();

			// fetch the mail's name:
			char* filenameBuf = filename.LockBuffer(B_FILE_NAME_LENGTH);
			if (filenameBuf) {
				*filenameBuf = '\0';
				mEntry.GetName(filenameBuf);
				filename.UnlockBuffer();
			}
			// set destination folder if there isn't any yet:
			if (destDir.InitCheck() == B_NO_INIT)
				mEntry.GetParent(&destDir);

			// copy entry for later use as a backup file
			backupEntry = mEntry;
		} else {
			// this mail has never been stored before
			whenCreated = real_time_clock_usecs();
			status = DefaultStatus();
		}
		if (!filename.Length())
			filename = CreateBasicFilename();

		// write mail to disk...
		if ((err = destDir.InitCheck()) != B_OK)
			BM_THROW_RUNTIME( 
				BmString("Could not set up a directory for mail <") << filename
					<< ">.\n\n Result: " << strerror(err)
			);
		StoreIntoFile( &destDir, filename, status, whenCreated, &backupEntry);
		// ...and fetch resulting entry-ref
		entry_ref eref;
		if ((err = mEntry.GetRef( &eref)) != B_OK) {
			BM_THROW_RUNTIME( 
				BmString("Could not get entry-ref for mail <") << filename
					<< ">.\n\n Result: " << strerror(err)
			);
		}

		// now that the mail lives on disk, we move it to trash, if requested:
		if (mMoveToTrash)
			::MoveToTrash( &eref, 1);

		// create a (new) mail-ref for the freshly saved mail:
		mMailRef = BmMailRef::CreateInstance( eref);

		// set new status for any mail(s) this one is based on:
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
	StoreIntoFile()
		-	writes mail-data and attributes into a file
\*------------------------------------------------------------------------------*/
void BmMail::StoreIntoFile( BDirectory* destDir, BmString filename, 
									 const BmString& status, bigtime_t whenCreated, 
									 BEntry* backupEntry) {
	status_t err = B_NO_INIT;
	ssize_t res;

	if ((err = mEntry.SetTo( destDir, filename.String())) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("Could not create entry for mail-file <") 
				<< filename << ">\n\n Result: " << strerror(err)
		);

	if (!backupEntry || backupEntry->InitCheck() != B_OK) {
		// this mail has never been stored on disk before, so we
		// need to make sure the name is unique:
		if (mEntry.Exists()) {
			// bump the filename until there's no such entry:
			uint32 i = 1;
			BmString fn;
			while (mEntry.Exists()) {
				if (i > 1000000) {
					// something is fishy if more than a million mails have 
					// arrived within one second - we complain
					BM_THROW_RUNTIME( 
						BmString("Unable to find unique name for mail-file <") 
							<< fn << ">!"
					);
				}
				fn = BmString(filename) << "-" << i++;
				if ((err = mEntry.SetTo( destDir, fn.String())) != B_OK) {
					BM_THROW_RUNTIME( 
						BmString("Could not create entry for mail-file <") 
							<< fn << ">\n\n Result: " << strerror(err)
					);
				}
			}
			filename = fn;
		}
	}

	// we create/open the new mailfile (keeping a backup)...
	BmBackedFile mailFile;
	err = mailFile.SetTo( mEntry, "text/x-email", backupEntry);
	if (err != B_OK)
		BM_THROW_RUNTIME( 
			BmString("Could not create backed mail-file\n\t<") 
				<< filename << ">\n\n Result: " << strerror(err)
		);
	// ...store all attributes...
	BM_LOG2( BM_LogMailParse, "storing mail-attributes...");
	StoreAttributes( mailFile.File(), status, whenCreated);
	BM_LOG2( BM_LogMailParse, "storing header-attributes...");
	mHeader->StoreAttributes( mailFile.File());

	// ...and finally write the raw mail into the file:
	BM_LOG2( BM_LogMailParse, "storing mail-data...");
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
	BM_LOG2( BM_LogMailParse, "done with storing");
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMail::StoreAttributes( BNode& mailNode, const BmString& status, 
										bigtime_t whenCreated) {
	//
	mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
							  status.String(), status.Length()+1);
	mailNode.WriteAttr( BM_MAIL_ATTR_ACCOUNT, B_STRING_TYPE, 0, 
							  mAccountName.String(), mAccountName.Length()+1);
	mailNode.WriteAttr( BM_MAIL_ATTR_IDENTITY, B_STRING_TYPE, 0, 
							  mIdentityName.String(), mIdentityName.Length()+1);
	mailNode.WriteAttr( BM_MAIL_ATTR_IMAP_UID, B_STRING_TYPE, 0, 
							  mImapUID.String(), mIdentityName.Length()+1);
	//
	if (mOutbound) {
		// write MAIL:flags in order to cooperate nicely with MDR:
		int32 flags = 0;
		if (status==BM_MAIL_STATUS_PENDING)
			flags = B_MAIL_PENDING | B_MAIL_SAVE;
		else if (status==BM_MAIL_STATUS_SENT)
			flags = B_MAIL_SENT;
		mailNode.WriteAttr( BM_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, 
								  &flags, sizeof(int32));
	}
	//
	int32 headerLength = HeaderLength();
	int32 contentLength = MAX( 0, mText.Length()-headerLength);
	
	mailNode.WriteAttr( BM_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, 
							  &headerLength, sizeof(int32));
	mailNode.WriteAttr( BM_MAIL_ATTR_CONTENT, B_INT32_TYPE, 0, 
							  &contentLength, sizeof(int32));
	//
	int32 hasAttachments = HasAttachments();
	mailNode.WriteAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, 
							  &hasAttachments, sizeof(hasAttachments));
	//
	mailNode.WriteAttr( BM_MAIL_ATTR_MARGIN, B_INT32_TYPE, 0, 
							  &mRightMargin, sizeof(int32));
	//
	mailNode.WriteAttr( BM_MAIL_ATTR_WHEN_CREATED, B_UINT64_TYPE, 0, 
							  &whenCreated, sizeof(whenCreated));
	//
	mailNode.WriteAttr( BM_MAIL_ATTR_CLASSIFICATION, B_STRING_TYPE, 0, 
							  mClassification.String(), mClassification.Length()+1);
	if (mRatioSpam != BmMailRef::UNKNOWN_RATIO)
		mailNode.WriteAttr( BM_MAIL_ATTR_RATIO_SPAM, B_FLOAT_TYPE, 0, 
								  &mRatioSpam, sizeof(mRatioSpam));
}

/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BmString BmMail::CreateBasicFilename() {
	static int32 counter = 1;
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	BmString name = BmString("mail-") << now << "-" << counter++;
	return name;
}

// #pragma mark - Destination Folder
/*------------------------------------------------------------------------------*\
	SetDestFolderName(folderName)
		-	sets the given folder-name as the folders where this mail should be 
			stored into.
		-	returns true if the folder has actually changed, false if the call
			was a no-op.
\*------------------------------------------------------------------------------*/
bool BmMail::SetDestFolderName( const BmString& destFolderName) {
	if (destFolderName != mDestFolderName) {
		mDestFolderName = destFolderName;
		return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	DestFolder()
		-	returns the mail folder this mail should be stored into.
		-	if a destination folder has been specified (via SetDestFolderName())
			this folder is tried first.
		-	if the specified destination folder can't be used, an appropriate
			default folder is used.
\*------------------------------------------------------------------------------*/
BmRef<BmMailFolder> BmMail::DestFolder() const {
	BmRef<BmMailFolder> folder;
	if (mDestFolderName.Length()) {
		while (!folder) {
			folder = TheMailFolderList->FindMailFolderBySubPath(mDestFolderName);
			if (!folder) {
				// folder does not exists, we check if its a system folder...
				if (BmMailFolder::IsSystemFolderSubPath(mDestFolderName)) {
					// yep, its a system folder, so we silently (re-)create it:
					BmString fullPath
						= ThePrefs->GetString("MailboxPath") + "/" + mDestFolderName;
					create_directory( fullPath.String(), 0755);
					// now wait until folder shows up (via node-monitor):
					for(int i=0; !folder && i<100; ++i) {
						snooze(100*1000);
						folder 
							= TheMailFolderList->FindMailFolderBySubPath(mDestFolderName);
					}
				} 
			}
			if (!folder) {
				// we may have to wait for the folder to appear:
				if (TheMailFolderList->IsJobRunning())
					snooze(200*1000);
				else
					break;
			}
		}
	}
	while (!folder) {
		// no destination folder has been set up, or it wasn't found, we
		// fall back to default folder:
		if (mMailRef && mMailRef->InitCheck() == B_OK) {
			// the mail is living on disk, so we set the destination folder 
			// to the one where the mail is living now:
			entry_ref eref = mMailRef->EntryRef();
			node_ref folderRef;
			folderRef.device = eref.device;
			folderRef.node = eref.directory;
			folder = dynamic_cast<BmMailFolder*>( 
				TheMailFolderList->FindItemByKey(BM_REFKEY(folderRef)).Get()
			);
		} else {
			// mail has never been stored, so we use in/out:
			BmString folderName 
				= mOutbound 
					? BmMailFolder::OUT_FOLDER_NAME 
					: BmMailFolder::IN_FOLDER_NAME;
			folder = TheMailFolderList->FindMailFolderBySubPath(folderName);
		}
		if (!folder) {
			// we may have to wait for the folder to appear:
			if (TheMailFolderList->IsJobRunning())
				snooze(200*1000);
			else
				break;
		}
	}
	return folder;
}

/*------------------------------------------------------------------------------*\
	MoveToDestFolder()
		-	moves a mail into a new destination folder (which has previously been
			specified via SetDestFolderName())
		-	the mail is just moved, it is not (re-)written to disk.
		-	returns true if the mail has been moved, false if call was a no-op 
			(since no dest-folder had been set via SetDestFolderName() or the
			mail had never been stored).
\*------------------------------------------------------------------------------*/
bool BmMail::MoveToDestFolder() {
	if (mDestFolderName.Length()) {
		if (mEntry.InitCheck() == B_OK
		|| mMailRef && mMailRef->InitCheck() == B_OK) {
			status_t err;
			if (mEntry.InitCheck() != B_OK) {
				if ((err = mEntry.SetTo( mMailRef->EntryRefPtr())) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not create entry from mail-ref <") 
							<< mMailRef->Key() << ">\n\n Result: " << strerror(err)
					);
			}
			BmRef<BmMailFolder> destFolder = DestFolder();
			if (!destFolder)
				return false;
			entry_ref mailEntryRef = mMailRef->EntryRef();
			node_ref folderRef = destFolder->NodeRef();
			if (folderRef.device == mailEntryRef.device
			&& folderRef.node == mailEntryRef.directory) {
				// mail already lives in that folder, nothing to do:
				return true;
			}
			BDirectory newHomeDir( destFolder->EntryRefPtr());
			if (newHomeDir.InitCheck() != B_OK)
				return false;
			if ((err = mEntry.MoveTo( &newHomeDir)) != B_OK) {
				BM_LOGERR( 
					BmString("Could not move mail <") << Name() 
						<< "> to folder <" << destFolder->DisplayKey()
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
			mMailRef = BmMailRef::CreateInstance( eref);
			return true;
		}
	}
	return false;
}


// #pragma mark - Filtering
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::ApplyOutboundFilters() {
	BmRef<BmMailFilter> filterJob = new BmMailFilter( Name(), NULL, false, false);
	filterJob->AddMail( this);
	filterJob->StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::ApplyInboundFilters() {
	BmRef<BmMailFilter> filterJob = new BmMailFilter( Name(), NULL, true, false);
	filterJob->AddMail( this);
	filterJob->StartJobInThisThread();
}

// #pragma mark - Sending
/*------------------------------------------------------------------------------*\
	Send()
		-	queues this mail for sending
\*------------------------------------------------------------------------------*/
bool BmMail::Send(bool now) 
{
	if (!mOutbound)
		return false;
	MarkAs( BM_MAIL_STATUS_PENDING);
	ApplyOutboundFilters();
	if (now) {
		BmRef<BmListModelItem> smtpRef 
			= TheSmtpAccountList->FindItemByKey( AccountName());
		BmSmtpAccount* smtpAcc 
			= dynamic_cast< BmSmtpAccount*>( smtpRef.Get());
		if (!smtpAcc || !mMailRef)
			return false;
		smtpAcc->SendMail( mMailRef->EntryRef());
	}
	return true;
}

// #pragma mark - Attachments
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
	AddAttachmenstFromRef()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::AddAttachmentFromRef( const entry_ref* ref,
											  const BmString& charset) {
	if (mBody)
		mBody->AddAttachmentFromRef( ref, charset);
}

// #pragma mark - Body Text
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmBodyPartList* BmMail::Body() const
{
	return mBody.Get(); 
}

/*------------------------------------------------------------------------------*\
	ReconstructRawText()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ReconstructRawText() {
	BmRef< BmBodyPart> bodyPart( mBody->EditableTextBody());
	return ConstructRawText( bodyPart ? bodyPart->DecodedData() : BM_DEFAULT_STRING,
								 DefaultCharset(), 
								 mAccountName);
}

/*------------------------------------------------------------------------------*\
	ConstructAndStore()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ConstructAndStore() {
	if (ReconstructRawText())
		return Store();
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	ConstructRawText()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ConstructRawText( const BmString& editedUtf8Text, 
										 const BmString& charset,
										 const BmString smtpAccount) {
	int32 startSize = mBody->EstimateEncodedSize() + editedUtf8Text.Length() 
							+ std::max( mHeader->HeaderLength(), (int32)4096)+4096;
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

// #pragma mark - Charset
/*------------------------------------------------------------------------------*\
	DefaultCharset()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmMail::DefaultCharset() const {
	if (mSuggestedCharset.Length())
		return mSuggestedCharset;
	if (mBody)
		return mBody->DefaultCharset();
	return BmEncoding::DefaultCharset;
}

// #pragma mark - Base Mail
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

// #pragma mark - Header
/*------------------------------------------------------------------------------*\
	SetDefaultHeaders()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetDefaultHeaders( const BmString& defaultHeaders)
{
	if (mHeader) {
		mHeader->UnplugDefaultHeader( mDefaultHeader.Get());
		if (defaultHeaders.Length()) {
			BmString defaultHeadersCrLf;
			defaultHeadersCrLf.ConvertLinebreaksToCRLF(&defaultHeaders);
			mDefaultHeader = new BmMailHeader( defaultHeadersCrLf, NULL);
			mHeader->PlugDefaultHeader( mDefaultHeader.Get());
		} else
			mDefaultHeader = NULL;
	}
}

/*------------------------------------------------------------------------------*\
	SetNewHeader()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetNewHeader( const BmString& headerStr) {
	BmString newMsgText;
	newMsgText.ConvertLinebreaksToCRLF( &headerStr);
	uint32 len = newMsgText.Length();
	if (!len || newMsgText[len-1] != '\n')
		newMsgText << "\r\n";
	newMsgText << mText.String() + HeaderLength();
	SetTo( newMsgText, mAccountName);
	Store();
	StartJobInThisThread();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeader* BmMail::Header() const
{
	return mHeader.Get(); 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMail::HeaderLength() const
{ 
	return mHeader 
				? mHeader->HeaderLength() 
				: 0; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmMail::HeaderText() const
{
	return mHeader 
				? mHeader->HeaderString() 
				: BM_DEFAULT_STRING; 
}

// #pragma mark - Header Fields
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
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::IsFieldEmpty( const BmString fieldName)
{ 
	return mHeader 
				? mHeader->IsFieldEmpty(fieldName)
				: true; 
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::IsRedirect() const
{
	return mHeader 
				? mHeader->IsRedirect()
				: false;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::IsRedirect( bool b)
{ 
	if (mHeader)
		mHeader->IsRedirect( b); 
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

// #pragma mark - Identities
/*------------------------------------------------------------------------------*\
	SetupFromIdentityAndRecvAddr()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::SetupFromIdentityAndRecvAddr( BmIdentity* ident, 
														 const BmString& recvAddr) 
{
	if (ident && recvAddr.Length()) {
		SetFieldVal( BM_FIELD_FROM, recvAddr);
		if (ident->ReplyTo().Length())
			SetFieldVal( BM_FIELD_REPLY_TO, ident->ReplyTo());
		else
			RemoveField( BM_FIELD_REPLY_TO);
		SetSignatureByName( ident->SignatureName());
		AccountName( ident->SMTPAccount());
		IdentityName( ident->Key());
		SetDefaultHeaders( ident->SpecialHeaders());
	}
}

/*------------------------------------------------------------------------------*\
	DetermineRecvAddrAndIdentity()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::DetermineRecvAddrAndIdentity( BmString& receivingAddr,
														 BmRef<BmIdentity>& ident)
{
	// try to determince the receiving address and identity
	// from the received mail's to-/cc-/bcc-info in several steps.
	// First, we check if this mail has an identity assigned to it:
	ident = dynamic_cast< BmIdentity*>( 
		TheIdentityList->FindItemByKey( IdentityName()).Get()
	);
	if (!ident) {
		// second, we check if the account through which the mail has been 
		// received can be identified (not always possible, since the account 
		// may have been renamed or deleted by now):
		ident = dynamic_cast< BmIdentity*>( 
			TheIdentityList->FindIdentityForRecvAccount( AccountName()).Get()
		);
	}
	if (ident) {
		// receiving identity is known, we let it find the receiving address
		receivingAddr = Header()->DetermineReceivingAddrFor( ident.Get());
		// if the identity doesn't handle any receiving address of this mail,
		// we simply use the identity's default address:
		if (!receivingAddr.Length())
			receivingAddr = ident->GetFromAddress();
	}
	if (!receivingAddr.Length()) {
		// the receiving address could not be determined, so we iterate through 
		// all identities and try to find one that (may) have received this mail.
		BmAutolockCheckGlobal lock( TheIdentityList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				"DetermineRecvAddrAndIdentity(): "
				"Unable to get lock on IdentityList"
			);
		BmModelItemMap::const_iterator iter;
		for( iter = TheIdentityList->begin(); 
			  iter != TheIdentityList->end() && !receivingAddr.Length(); 
			  ++iter) {
			ident = dynamic_cast< BmIdentity*>( iter->second.Get());
			if (ident) {
				receivingAddr 
					= Header()->DetermineReceivingAddrFor( ident.Get());
			}
		}
	}
}

// #pragma mark - Signatures
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

// #pragma mark - Status
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
const BmString& BmMail::Status() const { 
	return mMailRef ? mMailRef->Status() : DefaultStatus();
}

/*------------------------------------------------------------------------------*\
	DefaultStatus()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmMail::DefaultStatus() const {
	if (!mDefaultStatus.Length())
		mDefaultStatus = mOutbound 
								? BM_MAIL_STATUS_DRAFT 
								: BM_MAIL_STATUS_NEW;
	return mDefaultStatus;
}

// #pragma mark - Spam/Tofu
/*------------------------------------------------------------------------------*\
	MarkAsSpam()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::MarkAsSpam() {
	mClassification = BM_MAIL_CLASS_SPAM;
	if (mMailRef)
		mMailRef->MarkAsSpam();
}

/*------------------------------------------------------------------------------*\
	MarkAsTofu()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::MarkAsTofu() {
	mClassification = BM_MAIL_CLASS_TOFU;
	if (mMailRef)
		mMailRef->MarkAsTofu();
}

/*------------------------------------------------------------------------------*\
	RatioSpam(rs)
		-	
\*------------------------------------------------------------------------------*/
void BmMail::RatioSpam( float rs)
{
	mRatioSpam = rs;
	if (mMailRef)
		mMailRef->RatioSpam(rs);
}

/*------------------------------------------------------------------------------*\
	RatioSpam()
		-	
\*------------------------------------------------------------------------------*/
float BmMail::RatioSpam() const
{
	if (mMailRef)
		return mMailRef->RatioSpam();
	else
		return mRatioSpam;
}

/*------------------------------------------------------------------------------*\
	HasBeenClassified()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::HasBeenClassified() const
{
	return this->RatioSpam() != BmMailRef::UNKNOWN_RATIO;
}

/*------------------------------------------------------------------------------*\
	IsMarkedAsSpam()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::IsMarkedAsSpam() const {
	return mMailRef 
		? mMailRef->Classification() == BM_MAIL_CLASS_SPAM 
		: mClassification == BM_MAIL_CLASS_SPAM;
}

/*------------------------------------------------------------------------------*\
	IsMarkedAsSpam()
		-	
\*------------------------------------------------------------------------------*/
bool BmMail::IsMarkedAsTofu() const {
	return mMailRef 
		? mMailRef->Classification() == BM_MAIL_CLASS_TOFU 
		: mClassification == BM_MAIL_CLASS_TOFU;
}
