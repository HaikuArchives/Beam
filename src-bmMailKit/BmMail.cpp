/*
	BmMail.cpp
		$Id$
*/

//#include <ctime>
//#include <memory>

#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

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
	,	mStatus( "New")
	,	mHasAttachments( false)
	,	mMailRef( NULL)
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( new BmBodyPartList( this))
	,	mInitCheck( B_NO_INIT)
{
}

/*------------------------------------------------------------------------------*\
	BmMail( entry_ref)
		-	constructor
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BmMailRef* ref) 
	:	inherited( BString("MailModel_") << ref->Inode())
	,	mStatus( "New")
	,	mHasAttachments( false)
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( new BmBodyPartList( this))
	,	mMailRef( new BmMailRef( *ref))	// copy the mail-ref
	,	mInitCheck( B_NO_INIT)
{
}
	
/*------------------------------------------------------------------------------*\
	BmMail( msgText, msgUID, account)
		-	constructor
		-	creates message with given text and receiver-account
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BString &msgText, const BString &account) 
	:	inherited( "MailModel_dummy")
	,	mStatus( "New")
	,	mHasAttachments( false)
	,	mHeader( NULL)
	,	mHeaderLength( 0)
	,	mBody( new BmBodyPartList( this))
	,	mMailRef( NULL)
	,	mInitCheck( B_NO_INIT)
{
	SetTo( msgText, account);
	mInitCheck = B_OK;
}
	
/*------------------------------------------------------------------------------*\
	~BmMail()
	-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
	delete mMailRef;
	delete mHeader;
}

/*------------------------------------------------------------------------------*\
	SetTo( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMail::SetTo( BString &msgText, const BString &account) {
	Regexx rx;
	
	// find end of header (and start of body):
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw BM_mail_format_error("BmMail: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// include cr/nl in header-string
	mHeaderLength = headerLen;

	mText.Adopt( msgText);					// take over the msg-string
	mAccountName = account;

	BString header;
	header.SetTo( mText, headerLen);
	mHeader = new BmMailHeader( header, this);
}
	
/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BPath path;
	status_t err;
	ssize_t res;
	BString basicFilename;
	BString inboxPath;

	try {
		if (mParentEntry.InitCheck() == B_NO_INIT) {
//			(inboxPath = ThePrefs->MailboxPath()) << "/mailbox";
			(inboxPath = ThePrefs->MailboxPath()) << "/in_beam";
			(err = mParentEntry.SetTo( inboxPath.String(), true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-folder <") << inboxPath << ">\n\n Result: " << strerror(err));
		}
		basicFilename = CreateBasicFilename();
		mParentEntry.GetPath( &path);
		BString filename( BString(path.Path()) << "/" << basicFilename);
		(err = mMailEntry.SetTo( filename.String())) == B_OK
											|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << filename << ">\n\n Result: " << strerror(err));
		if (mMailEntry.Exists()) {
			BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">.\n\n Result: " << strerror(err));
		}
		// we create the new mailfile...
		(err = mailFile.SetTo( &mMailEntry, B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
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
	mailFile.WriteAttr( "MAIL:status", B_STRING_TYPE, 0, mStatus.String(), mStatus.Length()+1);
	mailFile.WriteAttr( "MAIL:account", B_STRING_TYPE, 0, mAccountName.String(), mAccountName.Length()+1);
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
	mailFile.WriteAttr( "MAIL:header_length", B_INT32_TYPE, 0, &headerLength, sizeof(int32));
	mailFile.WriteAttr( "MAIL:content_length", B_INT32_TYPE, 0, &contentLength, sizeof(int32));
	//
	mailFile.WriteAttr( "MAIL:attachments", B_BOOL_TYPE, 0, &mHasAttachments, sizeof(mHasAttachments));
}
/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BString BmMail::CreateBasicFilename() {
	static int32 counter = 1;
	BString name = mHeader->GetEnhancedFieldVal("Name");
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now << "_" << counter++;
	RemoveSetFromString( name, "/ ':\"\t");
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
		(err = mailFile.SetTo( &eref, B_READ_ONLY)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not open mail-file <") << eref.name << "> \n\nError:" << strerror(err));
		
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
		// we create the BmMail from the plain text:
		BM_LOG2( BM_LogMailParse, BString("creating a BmMail-instance from msgtext"));
		SetTo( mailText, mMailRef->Account());
		BM_LOG2( BM_LogMailParse, BString("Done, mail is initialized"));
		mInitCheck = B_OK;
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	return InitCheck() == B_OK;
}

