/*
	BmMail.cpp
		$Id$
*/

//#include <ctime>
//#include <memory>

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
#include "BmPrefs.h"
#include "BmResources.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

/********************************************************************************\
	BmMail
\********************************************************************************/

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
{
	BString emptyMsg( "Mime: 1.0\r\n");
	emptyMsg << "Date: " << TimeToString( time( NULL), "%a, %d %m %Y %H:%M:%S %z");
	emptyMsg << "\r\n\r\n";
	SetTo( emptyMsg, "");
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
{
	SetTo( msgText, account);
}
	
/*------------------------------------------------------------------------------*\
	BmMail( mailref)
		-	constructor via a mail-ref (from file)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BmMailRef* ref) 
	:	inherited( BString("MailModel_") << ref->Inode())
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( NULL)
	,	mMailRef( ref)
	,	mInitCheck( B_NO_INIT)
	,	mOutbound( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmMail()
	-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
	delete mHeader;
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
	delete mHeader;
	mHeader = new BmMailHeader( header, this);
	
	mBody = new BmBodyPartList( this);
	mBody->ParseMail();

	mInitCheck = B_OK;
}
	
/*------------------------------------------------------------------------------*\
	MarkAs()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::MarkAs( const char* status) {
	if (InitCheck() != B_OK)
		return;
	try {
		BNode mailNode;
		status_t err;
		entry_ref eref;
		// we write the new status...
		if (mMailRef)
			eref = mMailRef->EntryRef();
		else if (mEntry.InitCheck() == B_OK)
			mEntry.GetRef( &eref);
		else
			return;
		(err = mailNode.SetTo( &eref)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create node for current mail-file.\n\n Result: ") << strerror(err));
		mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, status, strlen( status)+1);
		// ...and tell the mail-ref, if neccessary:
		if (mMailRef)
			mMailRef->Status( status);
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
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString& BmMail::GetFieldVal( const BString fieldName) {
	if (mHeader)
		return mHeader->GetFieldVal( fieldName);
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
	RemoveFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::RemoveField( const BString fieldName) {
	mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	DefaultEncoding()
		-	
\*------------------------------------------------------------------------------*/
uint32 BmMail::DefaultEncoding() const {
	uint32 defEncoding;
	if (mBody && (defEncoding = mBody->DefaultEncoding())!=BM_UNKNOWN_ENCODING)
		return defEncoding;
	return mHeader ? mHeader->DefaultEncoding() : B_ISO1_CONVERSION;
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BPath tmpPath;
	status_t err;
	ssize_t res;
	BString basicFilename;
	BDirectory homeDir;
	BEntry tmpEntry;

	try {
		// find out where mail shall be living:
		if (mMailRef && mMailRef->InitCheck() == B_OK) {
			// mail has been read from disk, we recycle the old name:
			basicFilename = mMailRef->TrackerName();
			(err = mEntry.SetTo( mMailRef->EntryRefPtr())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry from mail-file <") << basicFilename << ">\n\n Result: " << strerror(err));
			
			(err = mEntry.GetParent( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create parent-dir from mail-file <") << basicFilename << ">\n\n Result: " << strerror(err));
		} else if (mEntry.InitCheck() == B_OK) {
			// mail has just been written to disk, we recycle the old name:
			char* buf = basicFilename.LockBuffer( B_FILE_NAME_LENGTH);
			mEntry.GetName( buf);
			basicFilename.UnlockBuffer();
			(err = mEntry.GetParent( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create parent-dir from mail-file <") << basicFilename << ">\n\n Result: " << strerror(err));
		} else {
			// mail is new, we create a new filename for it:
			basicFilename = CreateBasicFilename();
			BString inoutboxPath;
			(inoutboxPath = ThePrefs->GetString("MailboxPath")) << (mOutbound ? "/out" : "/in");
			(err = homeDir.SetTo( inoutboxPath.String())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create directory from mail-folder-path <") << inoutboxPath << ">\n\n Result: " << strerror(err));

		}
			
		// determine tmp-folder where the mail is being created:
		find_directory( B_COMMON_TEMP_DIRECTORY, &tmpPath, true) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not find tmp-directory on this system") << "\n\n Result: " << strerror(err));
		BString tmpFilename( BString(tmpPath.Path()) << "/" << basicFilename);
		(err = tmpEntry.SetTo( tmpFilename.String())) == B_OK
											|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << tmpFilename << ">\n\n Result: " << strerror(err));
		if (tmpEntry.Exists()) {
			BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">.\n\n Result: " << strerror(err));
		}
		// we create the new mailfile...
		(err = mailFile.SetTo( &tmpEntry, B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		// ...lock the file so no-one will be reading while we write...
		mailFile.Lock();
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
		mailFile.Unlock();

		// finally we move the mail-file to its new home:
		bool clobber = (mEntry.InitCheck() == B_OK);
		(err = tmpEntry.MoveTo( &homeDir, NULL, clobber)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not move mail \n\t<") << basicFilename << ">\nto new home\n\nResult: " << strerror(err));
		mEntry = tmpEntry;
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
}
/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateBasicFilename() {
	static int32 counter = 1;
	BString name = mHeader->GetFieldVal(BM_FIELD_SUBJECT);
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now << "_" << counter++;
	RemoveSetFromString( name, "/':\"\t");
							// we avoid some characters in filename
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
		// we take a little nap (giving the user time to navigate onwards),
		// after which we check if we should really read the mail:
		snooze( 50*1000);
		if (!ShouldContinue())
			return false;

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
		off_t mailSize;
		(err = mailFile.GetSize( &mailSize)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get size of mail-file <") << eref.name << "> \n\nError:" << strerror(err));
		BM_LOG2( BM_LogMailParse, BString("...should be reading ") << mailSize << " bytes");
		char* buf = mailText.LockBuffer( mailSize+1);
		off_t realSize = 0;
		const size_t blocksize = 65536;
		for( int32 offs=0; ShouldContinue() && offs < mailSize; ) {
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
		if (!ShouldContinue())
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

