/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailHeader_h
#define _BmMailHeader_h

#include "BmMailKit.h"

#include <map>
#include <vector>

#include "BmBasics.h"
#include "BmFilterAddon.h"
#include "BmMemIO.h"
#include "BmRefManager.h"
#include "BmUtil.h"

using std::map;
using std::vector;

class BmMail;
class BmIdentity;

/*------------------------------------------------------------------------------*\
	mail_format_error
		-	exception to indicate an error in the format of a mail-message
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BM_mail_format_error : public BM_runtime_error {
public:
	BM_mail_format_error (const BmString& what_arg)
		: BM_runtime_error (what_arg.String()) 
													{ }
	BM_mail_format_error (const char* const what_arg)
		: BM_runtime_error (what_arg) 	{ }
};

/*------------------------------------------------------------------------------*\
	BmAddress
		-	represents a single mail-addresses (parsed and split into 
			its components)
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmAddress {

public:
	// c'tors and d'tor:
	BmAddress();
	BmAddress( const BmString& addrText);
	~BmAddress();

	inline bool operator== (const BmAddress& a) 
													{ return mPhrase == a.Phrase() 
															&&  mAddrSpec == a.AddrSpec(); }

	// native methods:
	bool SetTo( const BmString& addrText);
	void ConstructRawText( BmString& header, const BmString& charset, 
								  int32 fieldNameLength) const;
	bool IsHandledByAccount( BmIdentity* ident, bool needExactMatch=false) const;
	const BmString& AddrString() const;

	// getters:
	inline bool InitOK() const				{ return mInitOK; }
	inline bool HasPhrase() const			{ return mPhrase.Length() > 0; }
	inline const BmString& Phrase() const		
													{ return mPhrase; }
	inline bool HasAddrSpec() const		{ return mAddrSpec.Length() > 0; }
	inline const BmString& AddrSpec() const
													{ return mAddrSpec; }

	static BmString QuotedPhrase(const BmString& phrase);

private:

	bool mInitOK;
	BmString mPhrase;
	BmString mAddrSpec;
	mutable BmString mAddrString;
};

typedef vector< BmString> BmStringList;
typedef vector< BmAddress> BmAddrList;
/*------------------------------------------------------------------------------*\
	BmAddressList
		-	represents an adress-group or (one or more) mail-addresses 
		-	each address is parsed and split into its components
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmAddressList {

public:
	// c'tors and d'tor:
	BmAddressList();
	BmAddressList( BmString strippedFieldVal);
	~BmAddressList();

	// native methods:
	inline bool SetTo( BmString strippedFieldVal) 
													{ return Set(strippedFieldVal); }
	bool Set( BmString strippedFieldVal);
	bool Add( BmString strippedFieldVal);
	void Remove( BmString singleAddress);
	BmStringList SplitIntoAddresses( BmString addrList);
	void ConstructRawText( BmStringOBuf& header, const BmString& charset, 
								  int32 fieldNameLength) const;
	const BmString& FindAddressMatchingAccount( BmIdentity* ident, 
															  bool needExactMatch=false) const;
	bool ContainsAddrSpec( BmString addrSpec) const;
	const BmString& AddrString() const;
	//
	inline BmAddrList::const_iterator begin() const 
													{ return mAddrList.begin(); }
	inline BmAddrList::const_iterator end() const	
													{ return mAddrList.end(); }
	inline size_t size() const				{ return mAddrList.size(); }
	inline bool empty() const				{ return mAddrList.empty(); }

	// getters:
	inline bool InitOK() const				{ return mInitOK; }
	inline bool IsGroup() const			{ return mIsGroup; }
	inline int32 AddrCount() const		{ return mAddrList.size(); }
	inline const BmString& GroupName() const	
													{ return mGroupName; }
	inline BmAddress FirstAddress() const
													{ return mAddrList.size() > 0 
															? mAddrList[0] 
															: BmAddress(); }

private:
	bool mInitOK;
	bool mIsGroup;
	BmString mGroupName;
	BmAddrList mAddrList;
	mutable BmString mAddrString;
};

/*------------------------------------------------------------------------------*\
	BmMailHeader 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
		- 	implements all mail-specific text-handling like header-parsing, 
			en-/decoding, en-/decrypting
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailHeader : public BmRefObj {

public:
	typedef vector< BmString> BmValueList;
	typedef map< BmString, BmValueList> BmHeaderMap;

private:
	class IMPEXPBMMAILKIT BmHeaderList {
	public:
		void Set( const BmString& fieldName, const BmString content);
		void Add( const BmString& fieldName, const BmString content);
		void Remove( const BmString& fieldName);
		void RemoveFieldVal( const BmString& fieldName, const BmString& val);
		BmHeaderMap::const_iterator begin() const 
													{ return mHeaders.begin(); }
		BmHeaderMap::const_iterator end() const	
													{ return mHeaders.end(); }
		uint32 CountValuesFor(const BmString& fieldName) const;
		const BmString& ValueAt(const BmString& fieldName, uint32 idx) const;
		const BmString& operator [] (const BmString& fieldName) const;
		void GetAllValues( BmMsgContext& msgContext) const;
		void GetAllNames(vector<BmString>& fieldNamesVect) const;

	private:
		BmHeaderMap mHeaders;
	};

	typedef map< BmString, BmAddressList> BmAddrMap;
	
public:
	// c'tors and d'tor:
	BmMailHeader( const BmString &headerText, BmMail* mail);
	~BmMailHeader();

	// native methods:
	void StoreAttributes( BFile& mailFile);
	//	these take UTF8 as input:
	void SetFieldVal( BmString fieldName, const BmString value);
	void AddFieldVal( BmString fieldName, const BmString value);
	void RemoveField( BmString fieldName);
	void RemoveFieldVal( const BmString fieldName,
								const BmString& val);
	void RemoveAddrFieldVal( BmString fieldName, const BmString address);
	const BmAddressList& GetAddressList( BmString fieldName);
	bool IsFieldEmpty( BmString fieldName);
	bool AddressFieldContainsAddrSpec( BmString fieldName, 
												  const BmString addrSpec);
	bool AddressFieldContainsAddress( BmString fieldName, 
												 const BmString& address);
	//
	BmString DetermineSender();
	BmString DetermineReceivingAddrFor( BmIdentity* ident);
	BmAddressList DetermineOriginator( bool bypassReplyTo=false);
	BmAddressList DetermineListAddress( bool bypassSanityTest=false);
	//
	void PlugDefaultHeader( const BmMailHeader* defaultHeader);
	void UnplugDefaultHeader( const BmMailHeader* defaultHeader);
	//
	bool ConstructRawText( BmStringOBuf& header, const BmString& charset);
	//
	void GetAllFieldValues( BmMsgContext& msgContext) const;
	const BmString& GetFieldVal( BmString fieldName, uint32 idx=0);
	uint32 CountFieldVals( BmString fieldName);
	void GetAllFieldNames(vector<BmString>& fieldNamesVect) const;

	// overrides of BmRefObj
	const BmString& RefName() const		{ return mKey; }

	// getters:
	inline const BmString& HeaderString() const	
													{ return mHeaderString; }
	inline const int32 HeaderLength() const
													{ return mHeaderString.Length(); }
	inline const BmString& Name() const	{ return mName; }
	inline const bool IsRedirect() const
													{ return mIsRedirect; }
	inline const bool HasParsingErrors() const	
													{ return mParsingErrors.Length()>0; }
	inline const BmString& ParsingErrors() const	
													{ return mParsingErrors; }

	// setters:
	inline void IsRedirect( bool b)		{ mIsRedirect = b; }

	// class-functions:
	static bool IsAddressField( const BmString fieldName);
	static bool IsEncodingOkForField( const BmString fieldName);
	static bool IsStrippingOkForField( const BmString fieldName);

protected:
	void ParseHeader( const BmString &header);
	BmString ParseHeaderField( BmString fieldName, BmString fieldValue);
	BmString StripField( BmString fieldValue, BmString* commentBuffer=NULL);
	void DetermineName();

private:
	void AddParsingError( const BmString& errStr);

	BmString mHeaderString;
							// the complete original mail-header
	BmHeaderList mHeaders;
							// contains all stripped headers as a list of corresponding
							// values. For simplicity, this map contains even fields
							// for which the stripped value does not make sense, 
							// because they aren't structured (e.g. 'Subject'). 
							// The stripped versions of these fields' values will be 
							// the same as the original field-value (i.e. no 
							// stripping takes place).
							// N.B.: 'stripped' actually means that any comments and 
							//       unneccessary whitespace are gone from the 
							//       field-values.
	BmAddrMap mAddrMap;
							// address-fields with detailed information about all
							// the single address-entries that are contained within
							// each field.
							// In case a complete addresslist is accessed as a 
							// BmString, it will (in contrast to the stripped-field) 
							// deliver a completely parsed and reconstructed version 
							// of the address.
							// This results in identical formatting for all addresses
							// (i.e. no '"'s around phrases and the like)
	BmMail* mMail;		
							// The mail these headers belong to
	BmString mName;
							// The "name" of the sender of this mail 
							// (MAIL:name attribute)
	BmString mKey;
							// Since headers have no real key, we generate one 
							// from the this-value
	bool mIsRedirect;	
							// true if header contains redirect-fields 
							// (or will do in near future)
	BmString mParsingErrors;
							// parsing-errors found in header
	static int32 nCounter;
							// counter for message-id

	// Hide copy-constructor and assignment:
	BmMailHeader( const BmMailHeader&);
	BmMailHeader operator=( const BmMailHeader&);
};

#endif

