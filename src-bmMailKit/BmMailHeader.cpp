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
#define BM_LOGNAME "MailParser"

static BString BmAddressFieldNames = 
	"<Bcc><Resent-Bcc><Cc><Resent-Cc><From><Resent-From><Reply-To><Resent-Reply-To><Sender><Resent-Sender><To><Resent-To>";

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
BmAddressList::BmAddressList( BString strippedFieldVal)
	:	mIsGroup( false) 
{
	mInitOK = Set( strippedFieldVal);
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
bool BmAddressList::Set( BString strippedFieldVal) {
	BString addrText;
	Regexx rx;
	bool res = true;

	mGroupName = "";
	mAddrList.clear();

	if (rx.exec( strippedFieldVal, "^\\s*(.+?):(.+)?;\\s*$")) {
		// it's a group list:
		mIsGroup = true;
		strippedFieldVal.CopyInto( mGroupName, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		if (rx.match[0].atom.size() > 1)
			strippedFieldVal.CopyInto( addrText, rx.match[0].atom[1].start(), rx.match[0].atom[1].Length());
		if (!addrText.Length())
			return false;
	} else {
		// simple address (or list of addresses)
		mIsGroup = false;
		addrText = strippedFieldVal;
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
	SplitIntoAddresses()
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
				pos++;
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
	()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::BmHeaderList::FoldLine( BString line, int fieldLength) const {
	BString spaces("                                                            ");
	BString temp;
	BString foldedLine;
	while( line.Length() > 76) {
		int32 pos;
		if ((pos = line.FindLast( " ", 75)) != B_ERROR
		|| (pos = line.FindLast( "\t", 75)) != B_ERROR
		|| (pos = line.FindLast( ",", 75)) != B_ERROR) {
			line.MoveInto( temp, 0, pos+1);
		} else {
			line.MoveInto( temp, 0, 76);
		}
		foldedLine << temp << "\r\n";
		foldedLine.Append( spaces, fieldLength+2);
	}
	return foldedLine << line;
}


/*------------------------------------------------------------------------------*\
	operator [] ( fieldName)
		-	returns first value found for given fieldName
\*------------------------------------------------------------------------------*/
BString& BmMailHeader::BmHeaderList::operator [] (const BString fieldName) {
	BmValueList& valueList = mHeaders[fieldName];
	if (valueList.empty())
		valueList.push_back( "");
	return valueList.front();
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeader::BmHeaderList::operator BString() const {
	BString fieldString;
	BmHeaderMap::const_iterator iter;
	for( iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
		BString fieldName = iter->first;
		const BmValueList& valueList = iter->second;
		int count = valueList.size();
		for( int i=0; i<count; ++i) {
			fieldString << FoldLine( BString(fieldName) << ": " << valueList[i], 
											 fieldName.Length()) 
							<< "\r\n";
		}
	}
	return fieldString;
}



/********************************************************************************\
	BmMailHeader
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	BmMailHeader( headerText)
		-	constructor
\*------------------------------------------------------------------------------*/
BmMailHeader::BmMailHeader( const BString &headerText, BmMail* mail)
	:	mHeaderString( headerText)
	,	mMail( mail)
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
	IsAddressField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsAddressField( const BString fieldName) {
	BString fname = BString("<") << fieldName << ">";
	return BmAddressFieldNames.IFindFirst( fname) != B_ERROR;
}

/*------------------------------------------------------------------------------*\
	GetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString& BmMailHeader::GetFieldVal( const BString fieldName) {
	return mHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	GetStrippedFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BString BmMailHeader::GetStrippedFieldVal( const BString fieldName) {
	if (IsAddressField( fieldName))
		return mAddrMap[fieldName];
	else
		return mStrippedHeaders[fieldName];
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::SetFieldVal( const BString fieldName, const BString value) {
	mHeaders.Set( fieldName, value);
	mStrippedHeaders.Set( fieldName, StripField( value));
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName] = BmAddressList( mStrippedHeaders[fieldName]);
	}
}

/*------------------------------------------------------------------------------*\
	AddFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::AddFieldVal( const BString fieldName, const BString value) {
	mHeaders.Add( fieldName, value);
	mStrippedHeaders.Add( fieldName, StripField( value));
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName] = BmAddressList( mStrippedHeaders[fieldName]);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveField()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveField( const BString fieldName) {
	mHeaders.Remove( fieldName);
	mStrippedHeaders.Remove( fieldName);
	BmAddrMap::iterator iter = mAddrMap.find( fieldName);
	if (iter != mAddrMap.end())
		mAddrMap.erase( iter);
}

/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMailHeader::ParseHeader( const BString &header) {
	Regexx rxHeaderFields, rxUnfold, rx;
	int32 nm;

	// set default encoding
	mDefaultEncoding = ThePrefs->GetInt("DefaultEncoding");
	// try to determine the mails' encoding by finding a charset given within the header:
	rx.expr( "^Content-Type:\\s*.+?;charset\\s*=[\\s\"]*([^\\s\"]+)");
	rx.str( header.String());
	if (rx.exec( Regexx::nocase | Regexx::newline)) {
		// extract encoding from the charset found:
		BString charset;
		header.CopyInto( charset, rx.match[0].atom[0].start(), rx.match[0].atom[0].Length());
		mDefaultEncoding = CharsetToEncoding( charset);
	}

	// count number of lines in header
	mNumLines = rx.exec( header, "\\n", Regexx::newline | Regexx::global);

	// split header into separate header-fields:
	rxHeaderFields.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxHeaderFields.str( header.String());
	if (!(nm=rxHeaderFields.exec( Regexx::global | Regexx::newline))) {
//		throw BM_mail_format_error( BString("Could not find any header-fields in this header: \n") << header);
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
		AddFieldVal( fieldName, ConvertHeaderPartToUTF8( fieldBody, mDefaultEncoding));

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody);
	}

	if (mMail) {
		// we construct the 'name' for this mail (will go into attribute MAIL:name)
		// by fetching the groupname or phrase of the first FROM-address:
		BmAddressList fromAddrList = mAddrMap["From"];
		if (fromAddrList.IsGroup()) {
			mName = fromAddrList.GroupName();
		} else {
			BmAddress fromAddr = fromAddrList.FirstAddress();
			if (fromAddr.HasPhrase())
				mName = fromAddr.Phrase();
			else
				mName = fromAddr.AddrSpec();
		}
	}
}

/*------------------------------------------------------------------------------*\
	StripField()
		-	
\*------------------------------------------------------------------------------*/
BString BmMailHeader::StripField( BString fieldValue, BString* commentBuffer) {
	BString stripped;
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
				stripped.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder to be 
				// part of the quoted string (and add the missing quote):
				stripped.Append( pos);
				stripped.Append( "\"");
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
				// remains are part of this incomplete comment. We add the missing paranthesis
				// to the comment buffer:
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
			stripped.Append( " ");
			pos += endPos-pos;
		} else {
			// we copy characters until we find the start of a quoted-string,
			// whitespace, or a comment:
			for(  endPos=pos+1; 
					*endPos && *endPos!='"' && *endPos!='(' && *endPos!='\t' && *endPos!=' ';
					++endPos)
				;
			int32 numChars = endPos-pos;
			stripped.Append( pos, numChars);
			pos += numChars;
		}
	}
	return stripped;
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
	BString s = mStrippedHeaders["Name"];
	mailFile.WriteAttr( BM_MAIL_ATTR_NAME, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["Reply-To"];
	mailFile.WriteAttr( BM_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["From"];
	mailFile.WriteAttr( BM_MAIL_ATTR_FROM, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	mailFile.WriteAttr( BM_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, mHeaders["Subject"].String(), mHeaders["Subject"].Length()+1);
	s = mAddrMap["To"];
	mailFile.WriteAttr( BM_MAIL_ATTR_TO, B_STRING_TYPE, 0, s.String(), s.Length()+1);
	s = mAddrMap["Cc"];
	mailFile.WriteAttr( BM_MAIL_ATTR_CC, B_STRING_TYPE, 0, s.String(), s.Length()+1);
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
	mailFile.WriteAttr( BM_MAIL_ATTR_PRIORITY, B_STRING_TYPE, 0, priority.String(), priority.Length()+1);
	//
	time_t t;
	if (ParseDateTime( mHeaders["Resent-Date"], t)) {
		mailFile.WriteAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &t, sizeof(t));
	} else if (ParseDateTime( mHeaders["Date"], t)) {
		mailFile.WriteAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &t, sizeof(t));
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmMailHeader::operator BString() const {
	const BString& header = mStrippedHeaders;
	BM_LOG2( BM_LogMailParse, BString("CONSTRUCTED HEADER: \n------------------\n") << header << "\n------------------");
	return header;
}

