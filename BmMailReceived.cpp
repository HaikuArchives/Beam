/*
	BmMailReceived.cpp
		$Id$
*/

#include <ctime>
#include <parsedate.h>

#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmApp.h"
#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMailReceived.h"
#include "BmPrefs.h"

#undef BM_LOGNAME
#define BM_LOGNAME mAccountName

/*
static BString RegX( const char* s);

static BString CHAR = 						"\\0-\\177";
static BString CHAR_ = 				RegX( "[$char]" );
static BString ALPHA = 						"a-zA-Z";
static BString ALPHA_ = 			RegX( "[$alpha]" );
static BString DIGIT = 						"0-9";
static BString DIGIT_ = 			RegX( "[$digit]" );
static BString CTL = 						"\\0-\\37";
static BString CTL_ = 				RegX( "[$ctl]" );
static BString CR = 							"\\r";
static BString LF = 							"\\n";
static BString SPACE = 						" ";
static BString HTAB = 						"\\t";
static BString QUOTE = 						"\\\"";
static BString CRLF = 						CR+LF;
static BString LWSP_CHAR = 				"\\t ";
static BString LWSP_CHAR_ = 		RegX( "[$lwsp_char]" );
static BString LINEAR_WHITE_SPACE = 	"(?:(?:\\r\\n)?[\\t ]+)";
static BString SPECIALS = 					"()\\[\\]<>@,;:\".\\\\";
static BString SPECIALS_ = 		RegX( "[$specials]" );
static BString TEXT = 						"(?:[^\\r]|\\r(?!\\n))";
static BString ATOM = 						"(?:[^$specials$space$ctl])+";
static BString CTEXT = 						"[^\"\\r\\(\\)\\\\]";
static BString DTEXT = 						"[^\"\\r\\[\\]\\\\]";
static BString QTEXT = 						"[^\"\\r\\\\]";
static BString QUOTED_PAIR = 		RegX( "\\\\$char" );
static BString QUOTED_STRING = 	RegX( "$quote(?:$qtext|$quoted_pair)*$quote" );
static BString DOMAIN_LITERAL = 	RegX( "\\[(?:$dtext|$quoted_pair)*\\]" );
static BString COMMENT = 			RegX(	"\\((?:$ctext|$quoted_pair|#)\\)", 2 );
static BString DELIMITERS = 		RegX( "(?:$specials|$linear_white_space|$comment)" );
static BString WORD = 				RegX( "(?:$atom|$quoted_string)" );
static BString ENCODED_WORD = "";
static BString PHRASE = 			RegX(	"(?:$word)+" );

/------------------------------------------------------------------------------\
	
\------------------------------------------------------------------------------/
static BString RegX( const char* s) {
	BString in(s);
	in.IReplaceAll( "$linear_white_space", LINEAR_WHITE_SPACE.String());
	in.IReplaceAll( "$domain_literal", DOMAIN_LITERAL.String());
	in.IReplaceAll( "$quoted_string", QUOTED_STRING.String());
	in.IReplaceAll( "$quoted_pair", QUOTED_PAIR.String());
	in.IReplaceAll( "$lwsp_char", LWSP_CHAR.String());
	in.IReplaceAll( "$specials", SPECIALS.String());
	in.IReplaceAll( "$alpha", ALPHA.String());
	in.IReplaceAll( "$digit", DIGIT.String());
	in.IReplaceAll( "$space", SPACE.String());
	in.IReplaceAll( "$ctext", CTEXT.String());
	in.IReplaceAll( "$dtext", DTEXT.String());
	in.IReplaceAll( "$qtext", QTEXT.String());
	in.IReplaceAll( "$quote", QUOTE.String());
	in.IReplaceAll( "$atom", ATOM.String());
	in.IReplaceAll( "$char", CHAR.String());
	in.IReplaceAll( "$crlf", CRLF.String());
	in.IReplaceAll( "$htab", HTAB.String());
	in.IReplaceAll( "$text", TEXT.String());
	in.IReplaceAll( "$ctl", CTL.String());
	in.IReplaceAll( "$cr", CR.String());
	in.IReplaceAll( "$lf", LF.String());
	return in;
}

*/


/*------------------------------------------------------------------------------*\
	default constructor
\*------------------------------------------------------------------------------*/
BmMailReceived::BmMailReceived( ) 
	:	mStatus( "New")
	,	mHasAttachments( false)
{
}

/*------------------------------------------------------------------------------*\
	constructor( msgText, msgUID)
		-	creates message with given text and unique-ID (as received from server)
\*------------------------------------------------------------------------------*/
BmMailReceived::BmMailReceived( BString &msgText, const BString &msgUID, const BString &account) 
	:	mStatus( "New")
	,	mHasAttachments( false)
{
	Set( msgText, msgUID, account);
}
	
/*------------------------------------------------------------------------------*\
	destructor
\*------------------------------------------------------------------------------*/
BmMailReceived::~BmMailReceived() {
}

/*------------------------------------------------------------------------------*\
	Set( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMailReceived::Set( BString &msgText, const BString &msgUID, const BString &account) {
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw BM_mail_format_error("BmMailReceived: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// include cr/nl in header-string

	mText.Adopt( msgText);					// take over the msg-string
	mAccountName = account;
	mUID = msgUID;

	BString header;
	header.SetTo( mText, headerLen);
	ParseHeader( header);
}
	
/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMailReceived::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold;
	int32 nm;

	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline))) {
		throw BM_mail_format_error( BString("Could not find any header-fields in this header: \n") << header);
	}
	vector<RegexxMatch>::const_iterator i;

	BM_LOG( BM_LogMailParse, "The mail-header");
	BM_LOG3( BM_LogMailParse, BString(header) << "\n------------------");
	BM_LOG( BM_LogMailParse, BString("contains ") << nm << " headerfields\n");

	for( i = rxHeaderFields.match.begin(); i != rxHeaderFields.match.end(); ++i) {

		// split each headerfield into field-name and field-body:
		BString headerField, fieldName, fieldBody;
		header.CopyInto( headerField, i->start(), i->Length());
		headerField.RemoveAll( "\r");
		int32 pos = headerField.FindFirst( ':');
		if (pos == B_ERROR) { 
			BM_SHOWERR(BString("Could not determine field-name of mail-header-part:\n") << headerField << "\n\nThis header-field will be ignored."); 
			continue;
		}
		fieldName.SetTo( headerField, pos);
		fieldName.RemoveSet( bmApp->WHITESPACE);
		fieldName.CapitalizeEachWord();
							// capitalized fieldnames seem to be popular...
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "\\r\\n\\s*", " ", Regexx::newline | Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "^\\s+", "", Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "\\s+$", "", Regexx::global);

		// insert pair into header-map:
		mHeaders[fieldName] = ParseHeaderField( fieldName, fieldBody);

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody << "\n------------------");
	}
}

/*------------------------------------------------------------------------------*\
	ParseHeaderField( fieldName, fieldValue)
		-	parses header-field and encodes it in UTF8
\*------------------------------------------------------------------------------*/
BString BmMailReceived::ParseHeaderField( BString fieldName, BString fieldValue) {
	Regexx rx;

	if (fieldName == "Subject") {
		return ConvertHeaderPartToUTF8( fieldValue);
	}
	return fieldValue;
/*
	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline))) {
		throw BM_mail_format_error( BString("Could not find any header-fields in this header: \n") << header);
	}
	vector<RegexxMatch>::const_iterator i;

	BM_LOG( BM_LogMailParse, "The mail-header");
	BM_LOG3( BM_LogMailParse, BString(header) << "\n------------------");
	BM_LOG( BM_LogMailParse, BString("contains ") << nm << " headerfields\n");

	for( i = rxHeaderFields.match.begin(); i != rxHeaderFields.match.end(); ++i) {
	}
*/
}

/*------------------------------------------------------------------------------*\
	ParseDateTime()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailReceived::ParseDateTime( const BString& str, time_t& dateTime) {
	if (!str.Length()) return false;
	dateTime = parsedate( str.String(), -1);
	return dateTime != -1;
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores mail-data and attributes inside a file
\*------------------------------------------------------------------------------*/
bool BmMailReceived::Store() {
	BFile mailFile;
	BNodeInfo mailInfo;
	BPath path;
	status_t err;
	ssize_t res;
	BString basicFilename;
	BString inboxPath;

	try {
		if (mParentEntry.InitCheck() == B_NO_INIT) {
//			(inboxPath = bmApp->Prefs->MailboxPath()) << "/mailbox";
			(inboxPath = bmApp->Prefs->MailboxPath()) << "/in_beam";
			(err = mParentEntry.SetTo( inboxPath.String(), true)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for mail-folder <") << inboxPath << ">\n\n Result: " << strerror(err));
		}
		basicFilename = CreateBasicFilename();
		int max=100;
		for( int i=0; i<max; i++) {
			mParentEntry.GetPath( &path);
			BString filename( BString(path.Path()) << "/" << basicFilename);
			if (i) 
				filename << " " << i;
			(err = mMailEntry.SetTo( filename.String())) == B_OK
												|| BM_THROW_RUNTIME( BString("Could not create entry for new mail-file <") << filename << ">\n\n Result: " << strerror(err));
			if (!mMailEntry.Exists())
				break;
		}
		if (mMailEntry.Exists()) {
			BM_THROW_RUNTIME( BString("Unable to create a unique filename for mail <") << basicFilename << ">, giving up after "<<max<<" tries.\n\n Result: " << strerror(err));
		}
		// we create the new mailfile...
		(err = mailFile.SetTo( &mMailEntry, B_WRITE_ONLY | B_CREATE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		// ...set the correct mime-type...
		(err = mailInfo.SetTo( &mailFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not set node-info for mail-file\n\t<") << basicFilename << ">\n\n Result: " << strerror(err));
		mailInfo.SetType( "text/x-email");
		// ...store all other attributes...
		StoreAttributes( mailFile);
		// ...and finally write the raw mail into the file:
		int32 len = mText.Length();
		if ((res = mailFile.Write( mText.String(), len)) < len) {
			if (res < 0) {
				BM_THROW_RUNTIME( BString("Unable to write into mailfile <") << basicFilename << ">\n\n Result: " << strerror(err));
			} else {
				BM_THROW_RUNTIME( BString("Could not write complete mail into file.\nWrote ") << res << " bytes instead of " << len);
			}
		}
		mailFile.Sync();
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
void BmMailReceived::StoreAttributes( BFile& mailFile) {
	//
	mailFile.WriteAttr( "MAIL:name", B_STRING_TYPE, 0, mHeaders["Name"].String(), mHeaders["Name"].Length()+1);
	mailFile.WriteAttr( "MAIL:status", B_STRING_TYPE, 0, mStatus.String(), mStatus.Length()+1);
	mailFile.WriteAttr( "MAIL:account", B_STRING_TYPE, 0, mHeaders["Account"].String(), mHeaders["Account"].Length()+1);
	mailFile.WriteAttr( "MAIL:reply", B_STRING_TYPE, 0, mHeaders["Reply-To"].String(), mHeaders["Reply-To"].Length()+1);
	mailFile.WriteAttr( "MAIL:from", B_STRING_TYPE, 0, mHeaders["From"].String(), mHeaders["From"].Length()+1);
	mailFile.WriteAttr( "MAIL:subject", B_STRING_TYPE, 0, mHeaders["Subject"].String(), mHeaders["Subject"].Length()+1);
	mailFile.WriteAttr( "MAIL:to", B_STRING_TYPE, 0, mHeaders["To"].String(), mHeaders["To"].Length()+1);
	mailFile.WriteAttr( "MAIL:cc", B_STRING_TYPE, 0, mHeaders["Cc"].String(), mHeaders["Cc"].Length()+1);
	// we determine the mail's priority, first we look at X-Priority...
	BString priority = mHeaders["X-Priority"];
	// ...if that is not defined we check the Priority field:
	if (!priority.Length()) {
		// need to translate from text to number:
		BString prio = mHeaders["Priority"];
		if (!prio.ICompare("Highest")) priority = "1";
		else if (!prio.ICompare("High")) priority = "2";
		else if (!prio.ICompare("Normal")) priority = "3";
		else if (!prio.ICompare("Low")) priority = "4";
		else if (!prio.ICompare("Lowest")) priority = "5";
	}
	mailFile.WriteAttr( "MAIL:priority", B_STRING_TYPE, 0, priority.String(), priority.Length()+1);
	//
	int32 headerLength, contentLength;
	if ((headerLength = mText.FindFirst("\r\n\r\n")) != B_ERROR) {
		headerLength++;
		contentLength = mText.Length()-headerLength;
	} else {
		headerLength = mText.Length();
		contentLength = 0;
	}
	mailFile.WriteAttr( "MAIL:header_length", B_INT32_TYPE, 0, &headerLength, sizeof(int32));
	mailFile.WriteAttr( "MAIL:content_length", B_INT32_TYPE, 0, &contentLength, sizeof(int32));
	//
	time_t t;
	if (ParseDateTime( mHeaders["Resent-Date"], t)) {
		mailFile.WriteAttr( "MAIL:when", B_TIME_TYPE, 0, &t, sizeof(t));
	} else if (ParseDateTime( mHeaders["Date"], t)) {
		mailFile.WriteAttr( "MAIL:when", B_TIME_TYPE, 0, &t, sizeof(t));
	}
	//
	mailFile.WriteAttr( "MAIL:attachments", B_BOOL_TYPE, 0, &mHasAttachments, sizeof(mHasAttachments));
}

/*------------------------------------------------------------------------------*\
	CreateBasicFilename()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailReceived::CreateBasicFilename() {
	BString name = mHeaders["From"];
	char now[16];
	time_t t = time(NULL);
	strftime( now, 15, "%0Y%0m%0d%0H%0M%0S", localtime( &t));
	name << "_" << now;
	name.ReplaceAll( "/", "-");
							// we avoid slashes in filename
	return name;
}
