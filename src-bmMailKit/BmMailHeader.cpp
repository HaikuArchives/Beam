/*
	BmMailHeader.cpp
		$Id$
*/

#include <ctime>
#include <parsedate.h>

#include <List.h>
#include <NodeInfo.h>

#include <regexx/regexx.hh>
using namespace regexx;

#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmResources.h"

#undef BM_LOGNAME
#define BM_LOGNAME mMail->AccountName()

static BString BmAddressFieldNames = 
	"<Bcc><Resent-Bcc><Cc><Resent-Cc><From><Resent-From><Reply-To><Resent-Reply-To><Sender><Resent-Sender><To><Resent-To>";

/*
static BString RegX( const char* s, int32 recursionCount=0);

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
//static BString COMMENT = 			RegX(	"\\((?:$ctext|$quoted_pair|#)\\)", 2 );
//static BString DELIMITERS = 		RegX( "(?:$specials|$linear_white_space|$comment)" );
static BString WORD = 				RegX( "(?:$atom|$quoted_string)" );
static BString PHRASE = 			RegX(	"(?:$word)+" );

enum BM_TOKEN {
	BMT_LINEAR_WHITESPACE = 0,
	BMT_SPECIAL,
	BMT_TEXT,
	BMT_COMMENT,
	BMT_DOMAIN,
	BMT_QUOTED_STRING,
};

struct BmToken {
	BmToken
};

static BmToken MatchTypeList[] = {
		{ QUOTED_STRING, },
};
*/
/*------------------------------------------------------------------------------*\
	
\*------------------------------------------------------------------------------*/
/*
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

#define SRegX(s) (BString("^")<<s)
*/


/********************************************************************************\
	BmAddress
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::BmAddress( BString fullText)
	:	mInitOK( false)
{
	Regexx rx;
	BString addrText, phraseText;

	// first we check whether the addresstext contains a leading phrase:
	if (rx.exec( fullText, "^\\s*(.+?)\\s*<\\s*(?:.+:)?([^:]+)\\s*>\\s*$")) {
		// it's a phrase followed by an address (which possibly contains a source-route).
		fullText.CopyInto( phraseText, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		fullText.CopyInto( addrText, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		// strip leading&trailing quotes:
		if (rx.exec( phraseText, "^[\"']+(.+?)[\"']+$")) {
			phraseText.CopyInto( mPhrase, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		} else {
			mPhrase = phraseText;
		}
	} else {
		// it's just a simple address (without textual phrase).
		addrText = fullText;
	}
	if (!addrText.Length()) {
		// address-part is empty!? not ok.
		return;
	}
	// finally strip all whitespace from the address-part (for easier comparison):
	addrText.RemoveSet( " \t");
	mAddrSpec = addrText;
	mInitOK = true;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::~BmAddress() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::operator BString() const {
	return mPhrase.Length() ? (mPhrase + " <" + mAddrSpec + ">") : mAddrSpec;
}



/********************************************************************************\
	BmAddressList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::BmAddressList()
	:	mInitOK( false)
	,	mIsGroup( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::BmAddressList( BString canonicalFieldVal)
	:	mIsGroup( false) 
{
	mInitOK = Set( canonicalFieldVal);
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::~BmAddressList() {
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddressList::Set( BString canonicalFieldVal) {
	BString addrText;
	Regexx rx;
	bool res = true;

	mGroupName = "";
	mAddrList.clear();

	if (rx.exec( canonicalFieldVal, "^\\s*(.+?):(.+)?;\\s*$")) {
		// it's a group list:
		mIsGroup = true;
		canonicalFieldVal.CopyInto( mGroupName, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		if (rx.match[0].atom.size() > 1)
			canonicalFieldVal.CopyInto( addrText, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		if (!addrText.Length())
			return false;
	} else {
		// simple address (or list of addresses)
		mIsGroup = false;
		addrText = canonicalFieldVal;
	}
	BmStringList addrList = SplitIntoAddresses( addrText);
	size_t num = addrList.size();
	for( size_t i=0; i<num; ++i) {
		BmAddress addr( addrList[i]);
		if (addr.InitOK()) {
			mAddrList.push_back( addr);
		} else {
			res = false;
		}
	}
	return res;
}
	
/*------------------------------------------------------------------------------*\
	CanonicalizeField()
		-	
\*------------------------------------------------------------------------------*/
BmStringList BmAddressList::SplitIntoAddresses( BString addrListText) {
	BmStringList addrList;
	BString currAddr;
	const char* pos = addrListText.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			BString quotedString;
			// quoted-string started, we remove the quotes and unquote quoted-pairs.
			for( endPos=pos+1; *endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string.
				int32 numChars = 1+endPos-pos;
				quotedString.Append( pos+1, numChars-2);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder to be 
				// part of the quoted string:
				quotedString.Append( pos+1);
				pos = endPos;
			}
			// we deescape characters that are escaped by a backslash (quoted-pairs):
			currAddr.Append( quotedString.CharacterDeescape( '\\'));
		} else if (*pos == '<') {
			// route-address started, we copy it as a block in order to avoid problems
			// with possibly contained separator-chars (commas).
			for( endPos=pos+1; *endPos && (*endPos!='>'); ++endPos)
				;
			if (*endPos) {
				// found complete route-address.
				int32 numChars = 1+endPos-pos;
				currAddr.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending '>', we assume the remainder to be 
				// part of the route-address (and append the missing '>'):
				currAddr.Append( pos);
				currAddr.Append( ">");
				pos = endPos;
			}
		} else {
			// we copy characters until we find the start of a quoted string or the separator char:
			for(  endPos=pos+1; *endPos && *endPos!='"' && *endPos!=','; ++endPos)
				;
			int32 numChars = endPos-pos;
			currAddr.Append( pos, numChars);
			pos += numChars;
			if (*endPos == ',') {
				addrList.push_back( currAddr);
				currAddr = "";
			}
		}
	}
	if (currAddr.Length()) {
		addrList.push_back( currAddr);
	}
	return addrList;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList::operator BString() const {
	BString addrString;
	if (mIsGroup)
		addrString << mGroupName << ": ";
	BmAddrList::const_iterator pos;
	for( pos=mAddrList.begin(); pos!=mAddrList.end(); ++pos) {
		addrString << BString( *pos) << ", ";
	}
	addrString.Truncate( addrString.Length()-2);
							// remove trailing comma+space
	if (mIsGroup)
		addrString << ";";
	return addrString;
}



/********************************************************************************\
	BmHeaderList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	Set( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Set( const BString fieldName, const BString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.clear();
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Add( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Add( const BString fieldName, const BString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Remove( fieldName)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Remove( const BString fieldName) {
	mHeaders.erase( fieldName);
}

/*------------------------------------------------------------------------------*\
	operator [] ( fieldName)
		-	
\*------------------------------------------------------------------------------*/
BString& BmMailHeader::BmHeaderList::operator [] (const BString fieldName) {
	BmValueList& valueList = mHeaders[fieldName];
	if (valueList.empty())
		valueList.push_back( "");
	return valueList.front();
}



/********************************************************************************\
	BmMailHeader
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMailHeader( headerText)
		-	constructor
\*------------------------------------------------------------------------------*/
BmMailHeader::BmMailHeader( const BString &headerText, BmMail* mail)
	:	mMail( mail)
{
	ParseHeader( headerText);
}
	
/*------------------------------------------------------------------------------*\
	~BmMailHeader()
	-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailHeader::~BmMailHeader() {
}

/*------------------------------------------------------------------------------*\
	GetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString& BmMailHeader::GetFieldVal( const BString fieldName) {
	return mHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	GetCanonicalFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString& BmMailHeader::GetCanonicalFieldVal( const BString fieldName) {
	return mCanonicalHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::SetFieldVal( const BString fieldName, const BString value) {
	mHeaders.Set( fieldName, value);
	mCanonicalHeaders.Set( fieldName, CanonicalizeField( value));
}

/*------------------------------------------------------------------------------*\
	AddFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::AddFieldVal( const BString fieldName, const BString value) {
	mHeaders.Add( fieldName, value);
	mCanonicalHeaders.Add( fieldName, CanonicalizeField( value));
}

/*------------------------------------------------------------------------------*\
	RemoveField()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveField( const BString fieldName) {
	mHeaders.Remove( fieldName);
	mCanonicalHeaders.Remove( fieldName);
}

/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMailHeader::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold, rx;
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
		fieldName.RemoveSet( TheResources->WHITESPACE);
		fieldName.CapitalizeEachWord();
							// capitalized fieldnames seem to be popular...
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "\\r\\n\\s*", " ", Regexx::newline | Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "^\\s+", "", Regexx::global);
		fieldBody = rxUnfold.replace( fieldBody, "\\s+$", "", Regexx::global);

		// insert pair into header-map:
		AddFieldVal( fieldName, ConvertHeaderPartToUTF8( fieldBody));
		if (BmAddressFieldNames.IFindFirst( fieldName) != B_ERROR) {
			// field contains an address-spec, we examine the address:
			BmAddressList addr( mCanonicalHeaders[fieldName]);
			if (addr.InitOK())
				mAddrMap[fieldName] = addr;
		}

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody << "\n------------------");
	}

	// we construct the 'name' for this mail (will go into attribute MAIL:name)
	// by fetching the groupname or phrase of the first FROM-address:
	BmAddressList fromAddrList = mAddrMap["From"];
	if (fromAddrList.IsGroup()) {
		AddFieldVal( "Name", fromAddrList.GroupName());
	} else {
		BmAddress fromAddr = fromAddrList.FirstAddress();
		if (fromAddr.HasPhrase())
			AddFieldVal( "Name", fromAddr.Phrase());
		else
			AddFieldVal( "Name", fromAddr.AddrSpec());
	}
}

/*------------------------------------------------------------------------------*\
	CanonicalizeField()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::CanonicalizeField( BString fieldValue, BString* commentBuffer) {
	BString canonical;
	const char* pos = fieldValue.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			// quoted-string started, we search its end:
			for( endPos=pos+1; *endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string, we copy it:
				int32 numChars = 1+endPos-pos;
				canonical.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder to be 
				// part of the quoted string (and add the missing quote):
				canonical.Append( pos);
				canonical.Append( "\"");
				pos = endPos;
			}
		} else if (*pos == '(') {
			// comment started, we search its end:
			BString comment;
			int32 nestLevel=1;
			for( endPos=pos+1; *endPos && (*endPos!=')' || *(endPos-1)=='\\' || --nestLevel); ++endPos) {
				if (*endPos == '(' && *(endPos-1)!='\\') {
					// take a note that we found a nested comment:
					nestLevel++;
				}
			}
			if (*endPos) {
				// found complete comment, we skip it:
				int32 numChars = 1+endPos-pos;
				if (commentBuffer)
					commentBuffer->Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending paranthesis, so we assume that all the
				// remains are part of this incomplete comment. This comment will be left out.
				if (commentBuffer) {
					commentBuffer->Append( pos);
					commentBuffer->Append( ")");
				}
				pos = endPos;
			}
		} else if (*pos == ' ' || *pos == '\t') {
			// replace linear whitespace by a single space:
			for( endPos=pos+1;  *endPos=='\t' || *endPos==' ';  ++endPos)
				;
			canonical.Append( " ");
			pos += endPos-pos;
		} else {
			// we copy characters until we find the start of a quoted-string,
			// whitespace, or a comment:
			for(  endPos=pos+1; 
					*endPos && *endPos!='"' && *endPos!='(' && *endPos!='\t' && *endPos!=' ';
					++endPos)
				;
			int32 numChars = endPos-pos;
			canonical.Append( pos, numChars);
			pos += numChars;
		}
	}
	return canonical;
}

/*------------------------------------------------------------------------------*\
	ParseDateTime()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::ParseDateTime( const BString& str, time_t& dateTime) {
	if (!str.Length()) return false;
	dateTime = parsedate( str.String(), -1);
	return dateTime != -1;
}

/*------------------------------------------------------------------------------*\
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMailHeader::StoreAttributes( BFile& mailFile) {
	//
	BString s = mCanonicalHeaders["Name"];
	mailFile.WriteAttr( "MAIL:name", B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["Reply-To"];
	mailFile.WriteAttr( "MAIL:reply", B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["From"];
	mailFile.WriteAttr( "MAIL:from", B_STRING_TYPE, 0, s.String(), s.Length()+1);
	mailFile.WriteAttr( "MAIL:subject", B_STRING_TYPE, 0, mHeaders["Subject"].String(), mHeaders["Subject"].Length()+1);
	s = mAddrMap["To"];
	mailFile.WriteAttr( "MAIL:to", B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["Cc"];
	mailFile.WriteAttr( "MAIL:cc", B_STRING_TYPE, 0, s.String(), s.Length()+1);
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
	time_t t;
	if (ParseDateTime( mHeaders["Resent-Date"], t)) {
		mailFile.WriteAttr( "MAIL:when", B_TIME_TYPE, 0, &t, sizeof(t));
	} else if (ParseDateTime( mHeaders["Date"], t)) {
		mailFile.WriteAttr( "MAIL:when", B_TIME_TYPE, 0, &t, sizeof(t));
	}
}

