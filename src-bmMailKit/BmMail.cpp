/*
	BmMail.cpp
		$Id$
*/

//#include <ctime>
//#include <memory>

#include <Directory.h>
#include <FindDirectory.h>
#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmBasics.h"
#include "BmBodyPartList.h"
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
BmMail::BmMail()
	:	inherited("MailModel_dummy")
	,	mMailRef( NULL)
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( new BmBodyPartList( this))
	,	mInitCheck( B_NO_INIT)
{
	BString emptyMsg( "Mime: 1.0\r\n\r\n");
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
	,	mBody( new BmBodyPartList( this))
	,	mMailRef( NULL)
	,	mInitCheck( B_NO_INIT)
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
	,	mBody( new BmBodyPartList( this))
	,	mMailRef( ref)
	,	mInitCheck( B_NO_INIT)
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
void BmMail::SetTo( BString &msgText, const BString account) {
	Regexx rx;
	
	// find end of header (and start of body):
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw BM_mail_format_error("BmMail: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// don't include separator-line in header-string
	mHeaderLength = headerLen;

	mText.Adopt( msgText);					// take over the msg-string
	mAccountName = account;

	BString header;
	header.SetTo( mText, headerLen);
	mHeader = new BmMailHeader( header, this);
	
	mBody->ParseMail();

	mInitCheck = B_OK;
}
	
/*------------------------------------------------------------------------------*\
	MarkAsRead()
		-	
\*------------------------------------------------------------------------------*/
void BmMail::MarkAs( const char* status) {
	if (!mMailRef || InitCheck() != B_OK)
		return;
	try {
		BNode mailNode;
		status_t err;
		// we write the new status...
		(err = mailNode.SetTo( mMailRef->EntryRefPtr())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create node for current mail-file.\n\n Result: ") << strerror(err));
		mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, status, strlen( status)+1);
		// ...and tell the mail-ref:
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
	return mMailRef ? mMailRef->Status() : "New";
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
void BmMail::SetFieldVal( const BString fieldName, const BString value) {
	// we set the field-value inside the mail-header only if it has content
	// otherwise we remove the field from the header:
	if (value.Length())
		mHeader->SetFieldVal( fieldName, value);
	else
		mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	ConstructMailForSending()
	-	
\*------------------------------------------------------------------------------*/
bool BmMail::ConstructMailForSending( const BString& editedText, int32 encoding) {
	BString msgText;
	if (!mHeader->ConstructHeaderForSending( msgText, encoding))
		return false;
	mBody->SetEditableText( editedText, encoding);
	if (!mBody->ConstructBodyForSending( msgText))
		return false;
BM_LOG2( BM_LogMailParse, BString("CONSTRUCTED MSG: \n------------------\n") << msgText << "\n------------------");
		
	return true;
}

/*------------------------------------------------------------------------------*\
	RemoveFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMail::RemoveField( const BString fieldName) {
	mHeader->RemoveField( fieldName);
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BPath path;
	BPath tmpPath;
	status_t err;
	ssize_t res;
	BString basicFilename;
	BString inboxPath;
	BDirectory homeDir;
	BEntry mailEntry;

	try {
		if (mParentEntry.InitCheck() == B_NO_INIT) {
			(inboxPath = ThePrefs->GetString("MailboxPath")) << "/mailbox";
			(err = mParentEntry.SetTo( inboxPath.String(), true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-folder <") << inboxPath << ">\n\n Result: " << strerror(err));
		}

		basicFilename = CreateBasicFilename();

		find_directory( B_COMMON_TEMP_DIRECTORY, &tmpPath, true) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not find tmp-directory on this system") << "\n\n Result: " << strerror(err));
		// determine real home of new mail:		
		mParentEntry.GetPath( &path);
		BString filename( BString(path.Path()) << "/" << basicFilename);
		// determing tmp-folder where the mail is being created:
		BString tmpFilename( BString(tmpPath.Path()) << "/" << basicFilename);
		(err = mailEntry.SetTo( tmpFilename.String())) == B_OK
											|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << tmpFilename << ">\n\n Result: " << strerror(err));
		if (mailEntry.Exists()) {
			BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">.\n\n Result: " << strerror(err));
		}
		// we create the new mailfile...
		(err = mailFile.SetTo( &mailEntry, B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
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
		(err = homeDir.SetTo( &mParentEntry)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not get directory for \n\t<") << path.Path() << ">\n\n Result: " << strerror(err));
		(err = mailEntry.MoveTo( &homeDir)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not move mail from tmp-folder to \n\t<") << filename << ">\n\n Result: " << strerror(err));
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
	mailFile.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "New", 4);
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
	BString name = mHeader->Name();
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now << "_" << counter++;
	RemoveSetFromString( name, "/ ':\"\t");
							// we avoid some characters in filename
	return name;
}



/********************************************************************************\
	BmLocalMail
\********************************************************************************/

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

