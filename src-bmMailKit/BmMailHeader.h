/*
	BmMailHeader.h
		$Id$
*/
/*************************************************************************/
/*                                                                       */
/*  Beam - BEware Another Mailer                                         */
/*                                                                       */
/*  http://www.hirschkaefer.de/beam                                      */
/*                                                                       */
/*  Copyright (C) 2002 Oliver Tappe <beam@hirschkaefer.de>               */
/*                                                                       */
/*  This program is free software; you can redistribute it and/or        */
/*  modify it under the terms of the GNU General Public License          */
/*  as published by the Free Software Foundation; either version 2       */
/*  of the License, or (at your option) any later version.               */
/*                                                                       */
/*  This program is distributed in the hope that it will be useful,      */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU    */
/*  General Public License for more details.                             */
/*                                                                       */
/*  You should have received a copy of the GNU General Public            */
/*  License along with this program; if not, write to the                */
/*  Free Software Foundation, Inc., 59 Temple Place - Suite 330,         */
/*  Boston, MA  02111-1307, USA.                                         */
/*                                                                       */
/*************************************************************************/


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

private:

	BmString QuotedPhrase() const;

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
		BmHeaderMap::const_iterator begin() const 
													{ return mHeaders.begin(); }
		BmHeaderMap::const_iterator end() const	
													{ return mHeaders.end(); }
		const BmString& operator [] (const BmString& fieldName) const;
		void GetAllValues( BmMsgContext& msgContext) const;

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
	BmString DetermineOriginator( bool bypassReplyTo=false);
	BmString DetermineListAddress( bool bypassSanityTest=false);
	//
	bool ConstructRawText( BmStringOBuf& header, const BmString& charset);

	// overrides of BmRefObj
	const BmString& RefName() const		{ return mKey; }

	// getters:
	void GetAllFieldValues( BmMsgContext& msgContext) const;
	const BmString& GetFieldVal( BmString fieldName);
	inline const BmString& HeaderString() const	
													{ return mHeaderString; }
	inline const int32 HeaderLength() const
													{ return mHeaderString.Length(); }
	inline const BmString& Name() const	{ return mName; }
	inline const bool IsRedirect() const
													{ return mIsRedirect; }
	inline const bool HasParsingErrors() const	
													{ return mHasParsingErrors; }

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

private:
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
	bool mHasParsingErrors;	
							// true if header contains parsing-errors
	static int32 nCounter;
							// counter for message-id

	// Hide copy-constructor and assignment:
	BmMailHeader( const BmMailHeader&);
	BmMailHeader operator=( const BmMailHeader&);
};

#endif

