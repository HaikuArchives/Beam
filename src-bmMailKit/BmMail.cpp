/*
	BmMail.cpp
		$Id$
*/


#include <regexx/regexx.hh>
using namespace regexx;

#include "BmPrefs.h"
#include "BmMail.h"

#undef BM_LOGNAME
#define BM_LOGNAME mAccountName

/*------------------------------------------------------------------------------*\
	default constructor
\*------------------------------------------------------------------------------*/
BmMail::BmMail( ) 
:	mHasChanged( false) {
}

/*------------------------------------------------------------------------------*\
	constructor( msgText, msgUID)
		-	creates message with given text and unique-ID (as received from server)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BString &msgText, const BString &msgUID, const BString &account) 
:	mHasChanged( true) {
	Set( msgText, msgUID, account);
}
	
/*------------------------------------------------------------------------------*\
	destructor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
}

/*------------------------------------------------------------------------------*\
	Set( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMail::Set( BString &msgText, const BString &msgUID, const BString &account) {
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw mail_format_error("BmMail: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// include cr/nl in header-string

	mText.Adopt( msgText);					// take over the msg-string
	AccountName( account);
	UID( msgUID);

	BString header;
	header.SetTo( mText, headerLen);
	ParseHeader( header);
}
	
/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMail::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold;
	int32 nm;

	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline))) {
		throw mail_format_error( BString("Could not find any header-fields in this header: \n") << header);
	}
	vector<RegexxMatch>::const_iterator i;

	BM_LOG( BM_LogMailParse, "The mail-header");
	BM_LOG3( BM_LogMailParse, BString(header) << "\n------------------");
	BM_LOG( BM_LogMailParse, BString("contains ") << nm << " headerfields\n");

	for( i = rxHeaderFields.match.begin(); i != rxHeaderFields.match.end(); ++i) {

		// split each headerfield into field-name and field-body:
		BString headerField, fieldName, fieldBody;
		header.CopyInto( headerField, i->start(), i->Length());
		int32 pos = headerField.FindFirst( ':');
		if (pos == B_ERROR) { throw mail_format_error(""); }
		fieldName.SetTo( headerField, pos);
		fieldName.RemoveSet( Beam::WHITESPACE);
		fieldName.CapitalizeEachWord();	
							// capitalized fieldnames seem to be popular...
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "\\r\\n\\s*", " ", Regexx::newline | Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "^\\s+", "", Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "\\s+$", "", Regexx::global);

		// insert pair into header-map:
		mHeaders[fieldName] = fieldBody;

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody << "\n------------------");
	}
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMail::Store() {
/*
	BFile mailFile;
	status_t err;
	BString inboxPath = "/boot/home/mail/in";

	try {
		if (mParentEntry.InitCheck() == B_NO_INIT) {
			(err = mParentEntry.SetTo( inboxPath.String(), true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-folder <") << inboxPath << ">\n\n Result: " << strerror(err));
		}
		if (mMyEntry.InitCheck() == B_NO_INIT) {
			BString basicFilename = CreateBasicFilename();
			for( int i=0; i<100; i++) {
				BString filename( mParentEntry.GetPath() << "/" << basicFilename);
				if (i) 
					filename << " " << i;
				(err = mMailEntry.SetTo( filename.String())) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << filename << ">\n\n Result: " << strerror(err));
				if (!mMailEntry.Exists())
					break;
			}
			if (mMailEntry.Exists()) {
				BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">, giving up after 10 tries.\n\n Result: " << strerror(err));
			}
		}
		(err = mailFile.SetTo( mMailPath.String(), B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create settings file\n\t<") << PrefsFilePath << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &prefsFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store settings into file\n\t<") << PrefsFilePath << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		ShowAlert();
		return false;
	}
*/
	return true;
}

