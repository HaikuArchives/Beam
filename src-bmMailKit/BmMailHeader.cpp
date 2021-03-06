/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <algorithm>
#include <ctype.h>

#include <List.h>
#include <NodeInfo.h>

#include "regexx.hh"
using namespace regexx;

#include "BmEncoding.h"
	using namespace BmEncoding;
#include "BmIdentity.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailHeader.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmSmtpAccount.h"

#undef BM_LOGNAME
#define BM_LOGNAME "MailParser"

static BmString BmAddressFieldNames = 
	"<Bcc><Resent-Bcc><Cc><List-Id><Resent-Cc><From><Resent-From><Reply-To>"
	"<Resent-Reply-To><Sender><Resent-Sender><To><Resent-To>";

static BmString BmIdentificationFieldNames = 
	"<Message-ID><In-Reply-To><References>";

static BmString BmNoEncodingFieldNames = 
	"<Received><Message-ID><Resent-Message-ID><In-Reply-To><References><Date>"
	"<Resent-Date>";

static BmString BmNoStrippingFieldNames = 
	"<Received><Subject><UserAgent>";

/********************************************************************************\
	BmAddress
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::BmAddress()
	:	mInitOK( false)
{
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
BmAddress::BmAddress( const BmString& fullText)
	:	mInitOK( false)
{
	SetTo( fullText);
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
bool BmAddress::SetTo( const BmString& fullText) {
	Regexx rx;
	BmString addrText, phraseText;

	// first we check whether the addresstext is separated into phrase 
	// and addrspec:
	if (rx.exec( 
		fullText, 
		"^\\s*(.*?)\\s*<\\s*(?:[^<>]+?:)?([^:<>]*?)\\s*>\\s*$"
	)) {
		// it's a phrase followed by an address (which possibly contains 
		// a source-route).
		fullText.CopyInto( phraseText, rx.match[0].atom[0].start(), 
								 rx.match[0].atom[0].Length());
		fullText.CopyInto( addrText, rx.match[0].atom[1].start(), 
								 rx.match[0].atom[1].Length());
		// strip leading&trailing quotes:
		if (rx.exec( phraseText, "^[\"']+(.+?)[\"']+$")) {
			phraseText.CopyInto( mPhrase, rx.match[0].atom[0].start(), 
										rx.match[0].atom[0].Length());
		} else {
			mPhrase = phraseText;
		}
	} else {
		// it's just a simple address (no <>).
		addrText = fullText;
	}
	// finally strip all leading/trailing whitespace from the address-part:
	if (rx.exec( addrText, "^\\s*(.+?)\\s*$"))
		addrText.CopyInto( mAddrSpec, rx.match[0].atom[0].start(), 
								 rx.match[0].atom[0].Length());
	else
		mAddrSpec = addrText;
	// avoid spurious addrSpecs that only contains whitespace:
	if (rx.exec( mAddrSpec, "^\\s+$"))
		mAddrSpec = "";
	mInitOK = (mAddrSpec.Length() > 0);
	return mInitOK;
}

/*------------------------------------------------------------------------------*\
	QuotedPhrase()
		-	returns the given phrase quoted if neccessary
\*------------------------------------------------------------------------------*/
BmString BmAddress::QuotedPhrase(const BmString& phrase) {
	// quote the phrase if it contains "dangerous" characters:
	BmString unsafeChars(",:;<>()\"");
	size_t lenOk = strcspn( phrase.String(), unsafeChars.String());
	if (lenOk < (size_t)phrase.Length())
		return BmString("\"") << phrase << "\"";
	else
		return phrase;
}

/*------------------------------------------------------------------------------*\
	AddrString()
		-	generates UTF8-string for this address
\*------------------------------------------------------------------------------*/
const BmString& BmAddress::AddrString() const {
	mAddrString.Truncate(0);
	if (mPhrase.Length()) {
		mAddrString = QuotedPhrase(mPhrase) + " <" + mAddrSpec + ">";
	} else
		mAddrString = mAddrSpec;
	return mAddrString;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddress::IsHandledByIdentity( BmIdentity* ident, 
												 bool needExactMatch) const {
	if (!ident)
		return false;
	return ident->HandlesAddrSpec( mAddrSpec, needExactMatch);
}

/*------------------------------------------------------------------------------*\
	ConstructRawText()
		-	adds this address to the given header
		-  the address is properly converted and formatted
\*------------------------------------------------------------------------------*/
void BmAddress::ConstructRawText( BmString& header, const BmString& charset, 
											 int32 fieldNameLength) const {
	BmString convertedAddrSpec 
		= ConvertUTF8ToHeaderPart( mAddrSpec, charset, false, fieldNameLength);
	if (mPhrase.Length()) {
		BmString convertedPhrase 
			= ConvertUTF8ToHeaderPart( QuotedPhrase(mPhrase), charset, true, 
												fieldNameLength);
		if (convertedPhrase.Length()+convertedAddrSpec.Length()+3 
				> ThePrefs->GetInt( "MaxLineLen")) {
			header << convertedPhrase << "\r\n <" << convertedAddrSpec << ">";
		} else
			header << convertedPhrase << " <" << convertedAddrSpec << ">";
	} else
		header << convertedAddrSpec;
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
BmAddressList::BmAddressList( BmString strippedFieldVal)
	:	mIsGroup( false) 
{
	Set( strippedFieldVal);
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
bool BmAddressList::Set( BmString strippedFieldVal) {
	mGroupName = "";
	mAddrList.clear();
	mInitOK = Add( strippedFieldVal);
	return mInitOK;
}
	
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmAddressList::Add( BmString strippedFieldVal) {
	BmString addrText;
	Regexx rx;
	bool res = true;

	mAddrString.Truncate(0);
	if (rx.exec( strippedFieldVal, "^\\s*(.+?):\\s*(.+?)?;\\s*$")) {
		// it's a group list:
		mIsGroup = true;
		strippedFieldVal.CopyInto( mGroupName, rx.match[0].atom[0].start(), 
											rx.match[0].atom[0].Length());
		if (rx.match[0].atom.size() > 1)
			strippedFieldVal.CopyInto( addrText, rx.match[0].atom[1].start(), 
												rx.match[0].atom[1].Length());
	} else {
		// simple address (or list of addresses)
		mIsGroup = false;
		addrText = strippedFieldVal;
	}
	BmStringList addrList = SplitIntoAddresses( addrText);
	size_t num = addrList.size();
	for( size_t i=0; i<num; ++i) {
		BmAddress addr( addrList[i]);
		if (addr.InitOK())
			mAddrList.push_back( addr);
		else
			res = false;
	}
	if (!mInitOK) 
		mInitOK = res;
	return res;
}
	
/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmAddressList::Remove( BmString singleAddress) {
	BmAddress rmAddr( singleAddress);
	BmAddrList::iterator pos = find( mAddrList.begin(), mAddrList.end(), rmAddr);
	if (pos != mAddrList.end()) {
		mAddrString.Truncate(0);
		mAddrList.erase( pos);
	}
}
	
/*------------------------------------------------------------------------------*\
	SplitIntoAddresses()
		-	
\*------------------------------------------------------------------------------*/
const BmString& BmAddressList
::FindAddressMatchingIdentity( BmIdentity* ident, bool needExactMatch) const {
	int32 count = mAddrList.size();
	for( int i=0; i<count; ++i) {
		if (mAddrList[i].IsHandledByIdentity( ident, needExactMatch))
			return mAddrList[i].AddrString();
	}
	return BM_DEFAULT_STRING;
}

/*------------------------------------------------------------------------------*\
	ContainsAddrSpec( addrSpec)
		-	
\*------------------------------------------------------------------------------*/
bool BmAddressList::ContainsAddrSpec( const BmString addrSpec) const {
	int32 count = mAddrList.size();
	for( int i=0; i<count; ++i) {
		if (addrSpec.ICompare( mAddrList[i].AddrSpec()) == 0)
			return true;
	}
	return false;
}

/*------------------------------------------------------------------------------*\
	SplitIntoAddresses()
		-	
\*------------------------------------------------------------------------------*/
BmStringList BmAddressList::SplitIntoAddresses( BmString addrListText) {
	mAddrString.Truncate(0);
	BmStringList addrList;
	BmString currAddr;
	const char* pos = addrListText.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			BmString quotedString;
			// quoted-string started, we remove the quotes and unquote 
			// quoted-pairs.
			for( 	endPos=pos+1; 
					*endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string.
				int32 numChars = 1+endPos-pos;
				quotedString.Append( pos+1, numChars-2);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the 
				// remainder to be part of the quoted string:
				quotedString.Append( pos+1);
				pos = endPos;
			}
			// we deescape characters that are escaped by a backslash 
			// (quoted-pairs):
			currAddr.Append( quotedString.CharacterDeescape( '\\'));
		} else if (*pos == '<') {
			// route-address started, we copy it as a block in order to avoid 
			// problems with possibly contained separator-chars (commas).
			for( endPos=pos+1; *endPos && (*endPos!='>'); ++endPos)
				;
			if (*endPos) {
				// found complete route-address.
				int32 numChars = 1+endPos-pos;
				currAddr.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending '>', we assume the remainder 
				// to be  part of the route-address (and append the missing '>'):
				currAddr.Append( pos);
				currAddr.Append( ">");
				pos = endPos;
			}
		} else {
			// we copy characters until we find the start of a quoted string or 
			// the separator char:
			for(  endPos=pos; *endPos && *endPos!='"' && *endPos!=','; ++endPos)
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
const BmString& BmAddressList::AddrString() const {
	mAddrString.Truncate(0);
	if (mIsGroup) {
		mAddrString << mGroupName << ":";
		if (mAddrList.begin() != mAddrList.end())
			// cosmetics: add space only if group actually contains addresses
			mAddrString << " ";
	}
	BmAddrList::const_iterator pos;
	for( pos=mAddrList.begin(); pos!=mAddrList.end(); ++pos) {
		if (pos != mAddrList.begin())
			mAddrString << ", ";
		mAddrString << pos->AddrString();
	}
	if (mIsGroup)
		mAddrString << ";";
	return mAddrString;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmAddressList::ConstructRawText( BmStringOBuf& header, 
												  const BmString& charset, 
												  int32 fieldNameLength) const {
	BmString fieldString;
	if (mIsGroup) {
		fieldString << mGroupName << ":";
		if (mAddrList.begin() != mAddrList.end())
			// cosmetics: add space only if group actually contains addresses
			fieldString << " ";
	}
	BmAddrList::const_iterator pos;
	for( pos=mAddrList.begin(); pos!=mAddrList.end(); ++pos) {
		BmString converted;
		pos->ConstructRawText( converted, charset, fieldNameLength);
		if (pos != mAddrList.begin()) {
			fieldString << ", ";
			if (fieldString.Length() + converted.Length() 
					> ThePrefs->GetInt( "MaxLineLen")) {
				fieldString << "\r\n ";
			}
		}
		fieldString << converted;
	}
	if (mIsGroup)
		fieldString << ";";
	header << fieldString;
}



/********************************************************************************\
	BmHeaderList
\********************************************************************************/

/*------------------------------------------------------------------------------*\
	Set( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Set( const BmString& fieldName, 
												  const BmString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.clear();
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Add( fieldName, value)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Add( const BmString& fieldName, 
												  const BmString value) {
	BmValueList& valueList = mHeaders[fieldName];
	valueList.push_back( value);
}

/*------------------------------------------------------------------------------*\
	Remove( fieldName)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::Remove( const BmString& fieldName) {
	mHeaders.erase( fieldName);
}

/*------------------------------------------------------------------------------*\
	RemoveFieldVal( fieldName, val)
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::RemoveFieldVal( const BmString& fieldName,
																 const BmString& val) {
	BmHeaderMap::iterator pos = mHeaders.find(fieldName);
	if (pos != mHeaders.end()) {
		BmValueList& valueList = pos->second;
		BmValueList::iterator valPos 
			= find(valueList.begin(),valueList.end(),val);
		if (valPos != valueList.end())
			valueList.erase(valPos);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	returns all values found for given fieldName
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList::GetAllValues( BmMsgContext& msgContext) const {
	msgContext.headerInfoCount = mHeaders.size();
	msgContext.headerInfos = new BmHeaderInfo [msgContext.headerInfoCount];
	int i = 0;
	BmHeaderMap::const_iterator iter;
	for( iter=mHeaders.begin(); iter != mHeaders.end(); ++iter, ++i) {
		const BmValueList& valueList = iter->second;
		const char** values = new const char* [valueList.size()+1];
		for( uint32 v=0; v<valueList.size(); ++v)
			values[v] = valueList[v].String();
		values[valueList.size()] = NULL;
		msgContext.headerInfos[i].values = values;
		msgContext.headerInfos[i].fieldName = iter->first;
	}
}

/*------------------------------------------------------------------------------*\
	GetAllNames()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::BmHeaderList
::GetAllNames(vector<BmString>& fieldNamesVect) const {
	fieldNamesVect.clear();
	BmHeaderMap::const_iterator iter;
	for( iter=mHeaders.begin(); iter != mHeaders.end(); ++iter) {
		fieldNamesVect.push_back(iter->first);
	}
}

/*------------------------------------------------------------------------------*\
	CountValuesFor( fieldName)
		-	returns the value-count found for given fieldName
\*------------------------------------------------------------------------------*/
uint32 BmMailHeader::BmHeaderList::CountValuesFor(const BmString& fieldName) const
{
	BmHeaderMap::const_iterator iter = mHeaders.find(fieldName);
	return (iter == mHeaders.end()) ? 0 : iter->second.size();
}

/*------------------------------------------------------------------------------*\
	ValueAt( fieldName, idx)
		-	returns the value no. idx for given fieldName
\*------------------------------------------------------------------------------*/
const BmString& BmMailHeader::BmHeaderList
::ValueAt(const BmString& fieldName, uint32 idx) const 
{
	BmHeaderMap::const_iterator iter = mHeaders.find(fieldName);
	if (iter == mHeaders.end())
		return BM_DEFAULT_STRING;
	const BmValueList& valueList = iter->second;
	if (valueList.size() <= idx)
		return BM_DEFAULT_STRING;
	return valueList[idx];
}

/*------------------------------------------------------------------------------*\
	operator [] ( fieldName)
		-	returns first value found for given fieldName
\*------------------------------------------------------------------------------*/
const BmString& BmMailHeader::BmHeaderList
::operator [] (const BmString& fieldName) const {
	return ValueAt( fieldName, 0);
}



/********************************************************************************\
	BmMailHeader
\********************************************************************************/

int32 BmMailHeader::nCounter = 0;

// split header into separate header-fields:
struct subpart {
	int32 pos;
	int32 len;
#ifdef __POWERPC__
	inline subpart() : pos(0), len(0) {}
#endif
	inline subpart( int32 p, int32 l) : pos(p), len(l) {}
};

/*------------------------------------------------------------------------------*\
	BmMailHeader( headerText)
		-	constructor
\*------------------------------------------------------------------------------*/
BmMailHeader::BmMailHeader( const BmString &headerText, BmMail* mail)
	:	mHeaderString( headerText)
	,	mMail( mail)
	,	mKey( RefPrintHex())
							// generate dummy identifier from our address
	,	mIsRedirect( false)
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
bool BmMailHeader::IsAddressField( BmString fieldName) {
	BmString fname = BmString("<") << fieldName.CapitalizeEachWord() << ">";
	return BmAddressFieldNames.IFindFirst( fname) != B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsIdentificationField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsIdentificationField( BmString fieldName) {
	BmString fname = BmString("<") << fieldName.CapitalizeEachWord() << ">";
	return BmIdentificationFieldNames.IFindFirst( fname) != B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsEncodingOkForField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsEncodingOkForField( BmString fieldName) {
	if (fieldName.ICompare("Content-", 8) == 0)
		return false;
	BmString fname = BmString("<") << fieldName.CapitalizeEachWord() << ">";
	return BmNoEncodingFieldNames.IFindFirst( fname) == B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsStrippingOkForField()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsStrippingOkForField( BmString fieldName) {
	fieldName.CapitalizeEachWord();	
	if (fieldName.Compare( "X-", 2) == 0)
		return false;							// no stripping for unknown fields
	BmString fname = BmString("<") << fieldName << ">";
	return BmNoStrippingFieldNames.IFindFirst( fname) == B_ERROR;
}

/*------------------------------------------------------------------------------*\
	IsFieldEmpty()
	-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::IsFieldEmpty( BmString fieldName) {
	fieldName.CapitalizeEachWord();
	return GetFieldVal( fieldName).Length() == 0;
}

/*------------------------------------------------------------------------------*\
	GetAllFieldValues()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::GetAllFieldValues( BmMsgContext& msgContext) const {
	mHeaders.GetAllValues( msgContext);
}

/*------------------------------------------------------------------------------*\
	GetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
const BmString& BmMailHeader::GetFieldVal( BmString fieldName, uint32 idx) {
	fieldName.CapitalizeEachWord();
	if (IsAddressField( fieldName))
		return mAddrMap[fieldName].AddrString();
	else
		return mHeaders.ValueAt(fieldName, idx);
}

/*------------------------------------------------------------------------------*\
	GetAllFieldNames()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::GetAllFieldNames(vector<BmString>& fieldNamesVect) const {
	mHeaders.GetAllNames( fieldNamesVect);
}

/*------------------------------------------------------------------------------*\
	CountFieldVals()
	-	
\*------------------------------------------------------------------------------*/
uint32 BmMailHeader::CountFieldVals( BmString fieldName) {
	fieldName.CapitalizeEachWord();
	return mHeaders.CountValuesFor(fieldName);
}

/*------------------------------------------------------------------------------*\
	AddressFieldContainsAddrSpec()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::AddressFieldContainsAddrSpec( BmString fieldName, 
																 const BmString addrSpec) {
	fieldName.CapitalizeEachWord();
	if (!IsAddressField( fieldName))
		BM_THROW_RUNTIME( 
			"BmMailHeader.AddressFieldContainsAddrSpec(): Field is not an "
			"address-field."
		);
	return mAddrMap[fieldName].ContainsAddrSpec( addrSpec);
}

/*------------------------------------------------------------------------------*\
	AddressFieldContainsAddress()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::AddressFieldContainsAddress( BmString fieldName, 
																const BmString& address) {
	fieldName.CapitalizeEachWord();
	if (!IsAddressField( fieldName))
		BM_THROW_RUNTIME( 
			"BmMailHeader.AddressFieldContainsAddress(): Field is not an "
			"address-field."
		);
	BmAddress addr( address);
	return mAddrMap[fieldName].ContainsAddrSpec( addr.AddrSpec());
}

/*------------------------------------------------------------------------------*\
	GetAddressList()
		-	
\*------------------------------------------------------------------------------*/
const BmAddressList& BmMailHeader::GetAddressList( BmString fieldName) {
	fieldName.CapitalizeEachWord();
	if (!IsAddressField( fieldName))
		BM_THROW_RUNTIME( 
			"BmMailHeader.GetAddressList(): Field is not an address-field."
		);
	return mAddrMap[fieldName];
}

/*------------------------------------------------------------------------------*\
	SetFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::SetFieldVal( BmString fieldName, const BmString value) {
	fieldName.CapitalizeEachWord();
	BmString strippedVal = IsStrippingOkForField( fieldName)
									? StripField( value)
									: value;
	mHeaders.Set( fieldName, strippedVal);
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName].Set( strippedVal);
	}
}

/*------------------------------------------------------------------------------*\
	AddFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::AddFieldVal( BmString fieldName, const BmString value) {
	fieldName.CapitalizeEachWord();
	BmString strippedVal = IsStrippingOkForField( fieldName)
									? StripField( value)
									: value;
	mHeaders.Add( fieldName, strippedVal);
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we parse the address as well:
		mAddrMap[fieldName].Add( strippedVal);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveFieldVal( BmString fieldName, const BmString& value)
{
	fieldName.CapitalizeEachWord();
	BmString strippedVal = IsStrippingOkForField( fieldName)
									? StripField( value)
									: value;
	mHeaders.RemoveFieldVal( fieldName, strippedVal);
	if (IsAddressField( fieldName)) {
		// field contains an address-spec, we remove the address as well:
		mAddrMap[fieldName].Remove( strippedVal);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveField()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveField( BmString fieldName) {
	fieldName.CapitalizeEachWord();
	mHeaders.Remove( fieldName);
	mAddrMap.erase( fieldName);
}

/*------------------------------------------------------------------------------*\
	RemoveAddrFieldVal()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::RemoveAddrFieldVal(  BmString fieldName, 
													 const BmString value) {
	fieldName.CapitalizeEachWord();
	if (IsAddressField( fieldName))
		mAddrMap[fieldName].Remove( value);
}

/*------------------------------------------------------------------------------*\
	DetermineOriginator()
	-	
\*------------------------------------------------------------------------------*/
BmAddressList BmMailHeader::DetermineOriginator( bool bypassReplyTo) {
	BmAddressList addrList = mAddrMap[BM_FIELD_REPLY_TO];
	if (bypassReplyTo || !addrList.InitOK()) {
		addrList = mAddrMap[BM_FIELD_MAIL_REPLY_TO];
		if (!addrList.InitOK()) {
			addrList = mAddrMap[BM_FIELD_FROM];
			if (!addrList.InitOK()) {
				addrList = mAddrMap[BM_FIELD_SENDER];
			}
		}
	}
	return addrList;
}

/*------------------------------------------------------------------------------*\
	DetermineSender()
		-	
\*------------------------------------------------------------------------------*/
BmString BmMailHeader::DetermineSender() {
	BmAddressList addrList = mAddrMap[BM_FIELD_SENDER];
	if (!addrList.InitOK()) {
		addrList = mAddrMap[BM_FIELD_FROM];
		if (!addrList.InitOK()) {
			BM_LOG( BM_LogMailParse, "Unable to determine sender of mail!");
			return "";
		}
	}
	if (addrList.IsGroup())
		return addrList.GroupName();
	return addrList.FirstAddress().AddrSpec();
}

/*------------------------------------------------------------------------------*\
	DetermineListAddress()
		-	
\*------------------------------------------------------------------------------*/
BmAddressList BmMailHeader::DetermineListAddress( bool bypassSanityTest) {
	BmAddressList listAddr;
	Regexx rx;
	// first, we look into the Reply-To-field (if it exists), as this
	// is required if a list actually redirects replies to another list!
	listAddr = mAddrMap[BM_FIELD_REPLY_TO];
	if (!listAddr.InitOK()) {
		// now we look into the List-Post-field (if it exists)...
		if (rx.exec( mHeaders[BM_FIELD_LIST_POST], "<\\s*mailto:([^?>]+)", 
						 Regexx::nocase | Regexx::newline)) {
			listAddr.SetTo( rx.match[0].atom[0]);
			if (listAddr.InitOK())
				// if an explicit List-Post is present, we want to accept it
				// even if the list-address is nowhere found in the receiver
				// fields (see sanity-check below):
				bypassSanityTest = true;
		}
	}
	if (!listAddr.InitOK()) {
		// ...we try to munge List-Id into a valid address:
		BmString listId = mAddrMap[BM_FIELD_LIST_ID].FirstAddress().AddrSpec();
		listId.ReplaceFirst( ".", "@");
		listAddr.SetTo( listId);
	}
	if (!listAddr.InitOK()) {
		// ...we look in field Mailing-List for the list-address:
		if (rx.exec( mHeaders[BM_FIELD_MAILING_LIST], "^\\s*list\\s*([^;\\s]+)", 
						 Regexx::nocase | Regexx::newline)) {
			listAddr.SetTo( rx.match[0].atom[0]);
		}
	}
	if (!listAddr.InitOK()) {
		// ...we have a look at some other fields (defined by prefs):
		vector<BmString> listFields;
		BmString lfs = ThePrefs->GetString( "ListFields");
		split( BmPrefs::nListSeparator, lfs, listFields);
		int32 numFields = listFields.size();
		for( int i=0; i<numFields; ++i) {
			if (!IsFieldEmpty( listFields[i])) {
				listAddr = mAddrMap[listFields[i]];
				if (listAddr.InitOK())
					break;
			}
		}
	}
	if (!bypassSanityTest && listAddr.AddrCount() == 1) {
		BmAddress firstAddr = listAddr.FirstAddress();
		// Sanity-check: the list-address *has* to be found somewhere within
		// the (From, To, Cc, Bcc)-Headers. 
		// If not, this mail is related to the list, but has not actually been
		// delivered through this list. This probably means that this mail is
		// a list-administrative mail (confirmation-requests and the like).
		if (!(AddressFieldContainsAddrSpec( BM_FIELD_TO, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_CC, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_BCC, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_FROM, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_REPLY_TO, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_RESENT_TO, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_RESENT_CC, firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_RESENT_BCC, 
													firstAddr.AddrSpec())
		|| AddressFieldContainsAddrSpec( BM_FIELD_RESENT_FROM, 
													firstAddr.AddrSpec())))	{
			// We do not want to send any replies to administrative mails back to 
			// the list, so we clear the List-Address:
			listAddr.SetTo("");
		}
	}
	return listAddr;
}

/*------------------------------------------------------------------------------*\
	DetermineReceivingAddrFor()
		-	
\*------------------------------------------------------------------------------*/
BmString 
BmMailHeader::DetermineReceivingAddrFor(const BmIdentityVect& identities,
	BmRef<BmIdentity>* identRefOut) {
	BmString addr;
	bool needExactMatch = true;
	// in the first loop-run, we check whether any of the addresses matches
	// the given identity exactly, in the second loop-run, we accept matches
	// by catch-all-identities, too:
	for( int i=0;  !addr.Length() && i<2;  ++i) {
		BmIdentityVect::const_iterator iter;
		for (iter = identities.begin(); 
			iter != identities.end() && !addr.Length(); ++iter) {
			addr = mAddrMap[BM_FIELD_TO].FindAddressMatchingIdentity( 
				iter->Get(), needExactMatch
			);
			if (!addr.Length()) {
				addr = mAddrMap[BM_FIELD_CC].FindAddressMatchingIdentity( 
					iter->Get(), needExactMatch
				);
			}
			if (!addr.Length()) {
				addr = mAddrMap[BM_FIELD_BCC].FindAddressMatchingIdentity( 
					iter->Get(), needExactMatch
				);
			}
			if (addr.Length() && identRefOut)
				*identRefOut = *iter;
		}
		needExactMatch = false;
	}
	if (!addr.Length()) {
		// nothing found yet - that usually means that none of our mail adresses
		// is contained as part of an address field in the header (the usual
		// case for mailing lists). Let's have a look at the Received headers
		// and try to find a matching address there:
		Regexx rx;
		rx.expr("[-+\\w]+@(?:[-+\\w]+\\.)?(?:[-+\\w]+)");
		uint32 receivedCount = CountFieldVals(BM_FIELD_RECEIVED);
		for (uint32 r = 0; r < receivedCount && !addr.Length(); ++r) {
			BmString receivedVal = GetFieldVal(BM_FIELD_RECEIVED, r);
			rx.str(receivedVal);
			int32 matchCount = rx.exec(Regexx::global);
			for (int32 m = 0; m < matchCount && !addr.Length(); ++m) {
				BmString mailAddr = rx.match[m];
				bool needExactMatch = true;
				for (int i=0; !addr.Length() && i < 2; ++i) {
					BmIdentityVect::const_iterator iter;
					for (iter = identities.begin(); 
						iter != identities.end() && !addr.Length(); ++iter) {
						if ((*iter)->HandlesAddrSpec(mailAddr, needExactMatch)) {
							addr = mailAddr;
							*identRefOut = *iter;
						}
					}
					needExactMatch = false;
				}
			}
		}
	}
	return addr;
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::PlugDefaultHeader( const BmMailHeader* defaultHeader)
{
	if (!defaultHeader)
		return;
	BmHeaderMap::const_iterator iter;
	for(	iter = defaultHeader->mHeaders.begin(); 
			iter != defaultHeader->mHeaders.end(); ++iter) {
		uint32 valCount = iter->second.size();
		for( uint32 v=0; v<valCount; ++v) 
			AddFieldVal( iter->first, (iter->second)[v]);
	}
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::UnplugDefaultHeader( const BmMailHeader* defaultHeader)
{
	if (!defaultHeader)
		return;
	BmHeaderMap::const_iterator iter;
	for(	iter = defaultHeader->mHeaders.begin(); 
			iter != defaultHeader->mHeaders.end(); ++iter) {
		uint32 valCount = iter->second.size();
		for( uint32 v=0; v<valCount; ++v) 
			RemoveFieldVal( iter->first, (iter->second)[v]);
	}
}

/*------------------------------------------------------------------------------*\
	AddParsingError()
	-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::AddParsingError( const BmString& errStr)
{
	if (errStr.Length()) {
		if (mParsingErrors.Length())
			mParsingErrors << "\n";
		mParsingErrors << errStr;
	}
}

/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
		-	we try to determine the mails' charset by finding a charset-string 
			given within the header. Some sending mail-clients cut some corners 
			and incorporate non-ASCII characters unencoded inside any 
			header-fields (the subject comes to mind) [thus relying on the 
			transport system being 8bit-clean].
			In this case we hope that the header contains a content-type 
			specification as well with the charset that was actually used. 
			We feed the charset found into the default charset that is being 
			used for header-field conversion (if and only if nothing else is 
			specified in a header-field)
\*------------------------------------------------------------------------------*/
void BmMailHeader::ParseHeader( const BmString &header) {
	Regexx rxUnfold, rx;

	mParsingErrors.Truncate(0);
	typedef vector< subpart> BmSubpartVect;
	BmSubpartVect subparts;
	int32 pos=-1;
	int32 lastpos = 0;
	for(  int32 offset=0; 
			(pos = header.FindFirst( "\r\n", offset)) != B_ERROR;
			offset = pos+2) {
		if (pos>lastpos && !isspace(header[pos+2])) {
			subparts.push_back( subpart( lastpos, pos-lastpos));
			lastpos = pos+2;
		}
	}
	int32 nm = subparts.size();
	if (!nm && mMail) {
		BM_LOGERR ( 
			BmString("Could not find any header-fields in this header: \n") 
				<< header
		);
	}
	BM_LOG( BM_LogMailParse, "The mail-header");
	BM_LOG3( BM_LogMailParse, BmString(header) << "\n------------------");
	BM_LOG( BM_LogMailParse, BmString("contains ") << nm << " headerfields\n");

	BmSubpartVect::const_iterator i;
	for( i=subparts.begin(); i!=subparts.end(); ++i) {

		// split each headerfield into field-name and field-body:
		BmString fieldName, fieldBody;
		BmString headerField( header.String()+i->pos, i->len);
		int32 pos = headerField.FindFirst( ':');
		if (pos == B_ERROR) { 
			BmString errStr 
				= BmString("Could not determine field-name of "
							  "mail-header-part:\n   ") << headerField 
						<< "\nThis header-field will be ignored.";
			AddParsingError( errStr);
			BM_LOG( BM_LogMailParse, errStr);
			continue;
		}
		fieldName.SetTo( headerField, pos);
		fieldName.RemoveSet( BM_WHITESPACE.String());
		headerField.CopyInto( fieldBody, pos+1, headerField.Length());

		// unfold the field-body and remove leading and trailing whitespace:
		fieldBody = rxUnfold.replace( fieldBody, "(?:\\s*\\r\\n)+\\s*", " ", 
												Regexx::global);
		fieldBody.Trim();

		// insert pair into header-map:
		if (IsEncodingOkForField(fieldName)) {
			bool hadConversionError;
			AddFieldVal( 
				fieldName, 
				ConvertHeaderPartToUTF8( 
					fieldBody, 
					mMail 
						? mMail->DefaultCharset()
						: ThePrefs->GetString("DefaultCharset"),
					hadConversionError
				)
			);
			if (hadConversionError) {
				BmString errStr 
					= BmString("Autodetected charset of header-field '") 
						<< fieldName << "', parts of text may be missing.";
				AddParsingError( errStr);
			}
		} else {
			AddFieldVal(fieldName, fieldBody);
		}

		BM_LOG2( BM_LogMailParse, fieldName << ": " << fieldBody);
	}

	if (mAddrMap[BM_FIELD_RESENT_FROM].InitOK() 
	|| mAddrMap[BM_FIELD_RESENT_SENDER].InitOK())
		IsRedirect( true);

	DetermineName();
}

/*------------------------------------------------------------------------------*\
	DetermineName()
		-	
\*------------------------------------------------------------------------------*/
void BmMailHeader::DetermineName() {
	if (mMail) {
		BmAddressList addrList;
		// we construct the 'name' for this mail (will go into 
		// attribute MAIL:name)...
		if (mMail->Outbound()) {
			// for outbound mails we fetch the groupname or phrase of the 
			// first TO-address:
			addrList = mAddrMap[BM_FIELD_TO];
			if (!addrList.InitOK()) {
				addrList = mAddrMap[BM_FIELD_CC];
				if (!addrList.InitOK())
					addrList = mAddrMap[BM_FIELD_BCC];
			}
		} else {
			// for inbound mails we fetch the groupname or phrase of the 
			// first FROM-address:
			addrList = mAddrMap[BM_FIELD_FROM];
		}
		if (addrList.IsGroup()) {
			mName = addrList.GroupName();
		} else {
			BmAddress addr = addrList.FirstAddress();
			if (addr.HasPhrase())
				mName = addr.Phrase();
			else
				mName = addr.AddrSpec();
		}
	}
}

/*------------------------------------------------------------------------------*\
	StripField()
		-	
\*------------------------------------------------------------------------------*/
BmString BmMailHeader::StripField( BmString fieldValue, 
											  BmString* commentBuffer) {
	BmString stripped;
	const char* pos = fieldValue.String();
	const char* endPos;
	while( *pos) {
		if (*pos == '"') {
			// quoted-string started, we search its end:
			for( 	endPos=pos+1; 
					*endPos && (*endPos!='"' || *(endPos-1)=='\\'); ++endPos)
				;
			if (*endPos) {
				// found complete quoted-string, we copy it:
				int32 numChars = 1+endPos-pos;
				stripped.Append( pos, numChars);
				pos += numChars;
			} else {
				// it seems that there is no ending quote, we assume the remainder
				// to be part of the quoted string (and add the missing quote):
				stripped.Append( pos);
				stripped.Append( "\"");
				pos = endPos;
			}
		} else if (*pos == '(') {
			// comment started, we search its end:
			BmString comment;
			int32 nestLevel=1;
			for(	endPos=pos+1; 
					*endPos && (*endPos!=')' || *(endPos-1)=='\\' || --nestLevel); 
					++endPos) {
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
				// it seems that there is no ending paranthesis, so we assume 
				// that all the remains are part of this incomplete comment. 
				// We add the missing paranthesis to the comment buffer:
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
					*endPos && *endPos!='"' && *endPos!='(' && *endPos!='\t' 
					&& *endPos!=' ';
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
	StoreAttributes()
		-	stores mail-attributes inside a file
\*------------------------------------------------------------------------------*/
void BmMailHeader::StoreAttributes( BFile& mailFile) {
	//
	bool outbound = mMail ? mMail->Outbound() : false;
	BmString recipients;
	//
	BmString s = Name();
	mailFile.WriteAttr( BM_MAIL_ATTR_NAME, B_STRING_TYPE, 0, s.String(), 
							  s.Length()+1);
	//
	s = mAddrMap[BM_FIELD_REPLY_TO].AddrString();
	mailFile.WriteAttr( BM_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, s.String(), 
							  s.Length()+1);
	//
	s = mAddrMap[BM_FIELD_FROM].AddrString();
	mailFile.WriteAttr( BM_MAIL_ATTR_FROM, B_STRING_TYPE, 0, s.String(), 
							  s.Length()+1);
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, 
							  mHeaders[BM_FIELD_SUBJECT].String(), 
							  mHeaders[BM_FIELD_SUBJECT].Length()+1);
	//
	mailFile.WriteAttr( BM_MAIL_ATTR_MIME, B_STRING_TYPE, 0, 
							  mHeaders[BM_FIELD_MIME].String(), 
							  mHeaders[BM_FIELD_MIME].Length()+1);
	//
	s = mAddrMap[BM_FIELD_TO].AddrString();
	mailFile.WriteAttr( BM_MAIL_ATTR_TO, B_STRING_TYPE, 0, s.String(), 
							  s.Length()+1);
	if (outbound && s.Length())
		recipients << s << ",";
	//
	s = mAddrMap[BM_FIELD_CC].AddrString();
	mailFile.WriteAttr( BM_MAIL_ATTR_CC, B_STRING_TYPE, 0, s.String(), 
							  s.Length()+1);
	if (outbound) {
		if (s.Length())
			recipients << s << ",";
		s = mAddrMap[BM_FIELD_BCC].AddrString();
		if (s.Length())
			recipients << s;
	}
	if (outbound && recipients.Length()) {
		// write recipients, if any:
		if (recipients[recipients.Length()-1] == ',')
			recipients.Truncate( recipients.Length()-1);
		mailFile.WriteAttr( BM_MAIL_ATTR_RECIPIENTS, B_STRING_TYPE, 0, 
								  recipients.String(), recipients.Length()+1);
	}
	// we determine the mail's priority, first we look at X-Priority...
	BmString priority = mHeaders[BM_FIELD_X_PRIORITY];
	// ...if that is not defined we check the Priority field:
	if (!priority.Length()) {
		// need to translate from text to number:
		BmString prio = mHeaders[BM_FIELD_PRIORITY];
		if (!prio.ICompare("Highest")) priority = "1";
		else if (!prio.ICompare("High")) priority = "2";
		else if (!prio.ICompare("Normal")) priority = "3";
		else if (!prio.ICompare("Low")) priority = "4";
		else if (!prio.ICompare("Lowest")) priority = "5";
	}
	if (!priority.Length()) {
		priority = "3";						// we default to normal priority
	}
	mailFile.WriteAttr( BM_MAIL_ATTR_PRIORITY, B_STRING_TYPE, 0, 
							  priority.String(), priority.Length()+1);
	// if the message was resent, we take the date of the resending operation,
	// not the original date:
	time_t t;
	if (!ParseDateTime( mHeaders[BM_FIELD_RESENT_DATE], t)
	&& !ParseDateTime( mHeaders[BM_FIELD_DATE], t))
		time( &t);
	mailFile.WriteAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &t, sizeof(t));
}

/*------------------------------------------------------------------------------*\
	()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailHeader::ConstructRawText( BmStringOBuf& msgText,
												 const BmString& charset) {
	mParsingErrors.Truncate(0);
	BmStringOBuf headerIO( 1024, 2.0);
	if (!mAddrMap[BM_FIELD_TO].InitOK() && !mAddrMap[BM_FIELD_CC].InitOK()) {
		if (mAddrMap[BM_FIELD_BCC].InitOK()) {
			// only hidden recipients via use of bcc, we set a dummy-<TO> value:
			SetFieldVal( BM_FIELD_TO, "Undisclosed-Recipients:;");
		}
	}

	// identify ourselves as creator of this mail message (so people know 
	// who to blame >:o)
	BmString agentField = ThePrefs->GetBool( "PreferUserAgentOverX-Mailer", true)
								? BM_FIELD_USER_AGENT : BM_FIELD_X_MAILER;
	if (IsFieldEmpty( agentField)) {
		BmString ourID = BeamRoster->AppNameWithVersion();
		SetFieldVal( agentField, ourID.String());
	}

	if (ThePrefs->GetBool( "GenerateOwnMessageIDs")) {
		// generate our own message-id:
		BmString domain;
		BmString accName = mMail->AccountName();
		if (accName.Length()) {
			BmSmtpAccount* account = dynamic_cast<BmSmtpAccount*>( 
				TheSmtpAccountList->FindItemByKey( accName).Get()
			);
			if (account)
				domain = account->DomainToAnnounce();
		}
		if (!domain.Length()) {
			// no account given or it has an empty domain, we try to 
			// find out manually:
			domain = BeamRoster->OwnFQDN();
			if (!domain.Length()) {
				BM_SHOWERR( "Identity crisis!\nBeam is unable to determine "
								"full-qualified domainname of this computer, "
								"something is seriously wrong with network settings!\n"
								"Beam will use a fake name and continue");
				domain = "bepc.fake.local";
			}
		}
		SetFieldVal( mMail->IsRedirect() 
							? BM_FIELD_RESENT_MESSAGE_ID 
							: BM_FIELD_MESSAGE_ID, 
						 BmString("<") << TimeToString( time( NULL), "%Y%m%d%H%M%S.")
						 				  << find_thread(NULL) << "." << ++nCounter 
						 				  << "@" << domain << ">");
	}

	BmString fieldName;
	try {

		BmHeaderMap::const_iterator iter;
		if (mMail->IsRedirect()) {
			// add Resent-fields first (as suggested by [Johnson, section 2.4.2]):
			for( iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
				fieldName = iter->first;
				BM_LOG2( BM_LogMailParse, 
							BmString( "ConstructRawText(): dealing with field ") 
								<< fieldName);
				if (fieldName.ICompare("Resent-",7) != 0) {
					// just interested in Resent-fields:
					continue;
				}
				if (IsAddressField( fieldName)) {
					headerIO << fieldName << ": ";
					mAddrMap[fieldName].ConstructRawText( headerIO, charset, 
																	  fieldName.Length());
					headerIO << "\r\n";
				} else {
					const BmValueList& valueList = iter->second;
					int count = valueList.size();
					bool encodeIfNeeded = IsEncodingOkForField( fieldName);
					for( int i=0; i<count; ++i) {
						headerIO << fieldName << ": " 
								 	<< ConvertUTF8ToHeaderPart( valueList[i], charset, 
																		 encodeIfNeeded,
																		 fieldName.Length())
									<< "\r\n";
					}
				}
			}
		}
		// add all other fields:
		for( iter = mHeaders.begin(); iter != mHeaders.end(); ++iter) {
			fieldName = iter->first;
			BM_LOG2( BM_LogMailParse, 
						BmString( "ConstructRawText(): dealing with field ") 
							<< fieldName);
			if (fieldName.ICompare("Content-",8) == 0) {
				// do not include MIME-header, since that will be added 
				// by body-part:
				continue;
			}
			if (fieldName.ICompare("Resent-",7) == 0) {
				// do not include Resent-headers again:
				continue;
			}
			if (IsAddressField( fieldName)) {
				headerIO << fieldName << ": ";
				mAddrMap[fieldName].ConstructRawText( headerIO, charset, 
																  fieldName.Length());
				headerIO << "\r\n";
			} else if (IsIdentificationField( fieldName)) {
				headerIO << fieldName << ": \r\n " 
							<< ConvertUTF8ToHeaderPart( iter->second.front(), charset, false, 0)
							<< "\r\n";
			} else {
				const BmValueList& valueList = iter->second;
				int count = valueList.size();
				bool encodeIfNeeded = IsEncodingOkForField( fieldName);
				for( int i=0; i<count; ++i) {
					headerIO << fieldName << ": " 
								<< ConvertUTF8ToHeaderPart( valueList[i], charset, 
																	 encodeIfNeeded,
																	 fieldName.Length())
								<< "\r\n";
				}
			}
		}
		mHeaderString.Adopt( headerIO.TheString());
		msgText << mHeaderString;

		DetermineName();

		return true;
	} catch( BM_text_error& textErr) {
		BmString errText = BmString("The ") << fieldName << "-field "
									<< "contains characters that\n"
										"could not be converted to the selected\n"
										"charset (" 
									<< charset 
									<< ").\n\n"
										"Please select the correct charset or remove "
										"the offending characters.";
		throw BM_text_error( errText, fieldName.String(), textErr.posInText);
	}
	return false;
}
