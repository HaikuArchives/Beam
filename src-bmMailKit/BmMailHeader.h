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

#include <map>
#include <vector>

#include "BmBasics.h"
#include "BmRefManager.h"
#include "BmUtil.h"

class BmMail;
class BmPopAccount;

/*------------------------------------------------------------------------------*\
	mail_format_error
		-	exception to indicate an error in the format of a mail-message
\*------------------------------------------------------------------------------*/
class BM_mail_format_error : public BM_runtime_error {
public:
	BM_mail_format_error (const BString& what_arg): BM_runtime_error (what_arg.String()) { }
	BM_mail_format_error (const char* const what_arg): BM_runtime_error (what_arg) { }
};

/*------------------------------------------------------------------------------*\
	BmAddress
		-	represents a single mail-addresses (parsed and split into its components)
\*------------------------------------------------------------------------------*/
class BmAddress {

public:
	// c'tors and d'tor:
	BmAddress( BString addrText);
	~BmAddress();

	inline bool operator== (const BmAddress& a) {
		return mPhrase == a.Phrase() 
			&&  mAddrSpec == a.AddrSpec();
	}

	// native methods:
	void ConstructRawText( BString& header, int32 encoding, int32 fieldNameLength) const;
	bool IsHandledByAccount( BmPopAccount* acc) const;

	// operators:
	operator BString() const;
						// returns address completely formatted (ready to be sent)
	// getters:
	inline bool InitOK() const				{ return mInitOK; }
	inline bool HasPhrase() const			{ return mPhrase.Length() > 0; }
	inline BString Phrase() const			{ return mPhrase; }
	inline BString AddrSpec() const		{ return mAddrSpec; }

private:
	bool mInitOK;
	BString mPhrase;
	BString mAddrSpec;

};

typedef vector< BString> BmStringList;
typedef vector< BmAddress> BmAddrList;
/*------------------------------------------------------------------------------*\
	BmAddressList
		-	represents an adress-group or (one or more) mail-addresses 
		-	each address is parsed and split into its components
\*------------------------------------------------------------------------------*/
class BmAddressList {

public:
	// c'tors and d'tor:
	BmAddressList();
	BmAddressList( BString strippedFieldVal);
	~BmAddressList();

	// native methods:
	bool Set( BString strippedFieldVal);
	bool Add( BString strippedFieldVal);
	void Remove( BString singleAddress);
	BmStringList SplitIntoAddresses( BString addrList);
	void ConstructRawText( BString& header, int32 encoding, int32 fieldNameLength) const;
	BString FindAddressMatchingAccount( BmPopAccount* acc) const;
	//
	inline BmAddrList::const_iterator begin() const { return mAddrList.begin(); }
	inline BmAddrList::const_iterator end() const	{ return mAddrList.end(); }
	inline size_t size() const				{ return mAddrList.size(); }
	inline bool empty() const				{ return mAddrList.empty(); }

	// operators:
	operator BString() const;
							// returns address-list completely formatted (ready to be sent)
	// getters:
	inline bool InitOK() const				{ return mInitOK; }
	inline bool IsGroup() const			{ return mIsGroup; }
	inline int32 AddrCount() const		{ return mAddrList.size(); }
	inline BString GroupName() const		{ return mGroupName; }
	inline BmAddress FirstAddress() const		{ return mAddrList.size() > 0 ? mAddrList[0] : ""; }

private:
	bool mInitOK;
	bool mIsGroup;
	BString mGroupName;
	BmAddrList mAddrList;

};

/*------------------------------------------------------------------------------*\
	BmMailHeader 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
		- 	implements all mail-specific text-handling like header-parsing, en-/decoding,
			en-/decrypting
\*------------------------------------------------------------------------------*/
class BmMailHeader : public BmRefObj {

	typedef vector< BString> BmValueList;
	typedef map< BString, BmValueList> BmHeaderMap;

	class BmHeaderList {
	public:
		void Set( const BString fieldName, const BString content);
		void Add( const BString fieldName, const BString content);
		void Remove( const BString fieldName);
		BmHeaderMap::const_iterator begin() const { return mHeaders.begin(); }
		BmHeaderMap::const_iterator end() const	{ return mHeaders.end(); }
		const BString& operator [] (const BString fieldName) const;
	private:
		BmHeaderMap mHeaders;
	};

	typedef map< BString, BmAddressList> BmAddrMap;
	
public:
	// c'tors and d'tor:
	BmMailHeader( const BString &headerText, BmMail* mail);
	~BmMailHeader();

	// native methods:
	void StoreAttributes( BFile& mailFile);
							//	the following three take UTF8 as input:
	void SetFieldVal( const BString fieldName, const BString value);
	void AddFieldVal( const BString fieldName, const BString value);
	void RemoveField( const BString fieldName);
	void RemoveAddrFieldVal( const BString fieldName, const BString address);
							// the next always produces US-ASCII (7-bit):
	bool ConstructRawText( BString& header, int32 encoding);

	BString DetermineSender();
	BString DetermineReceivingAddrFor( BmPopAccount* acc);
	const BmAddressList GetAddressList( const BString fieldName);
	bool IsFieldEmpty( const BString fieldName);

	// overrides of BmRefObj
	const BString& RefName() const				{ return mKey; }

	// getters:
	const BString& GetFieldVal( const BString fieldName);
	const BString GetStrippedFieldVal( const BString fieldName);
	inline int32 NumLines() const 				{ return mNumLines; }
	inline const BString& HeaderString() const	{ return mHeaderString; }
	inline const BString& Name() const			{ return mName; }
	inline uint32 DefaultEncoding()	const		{ return mDefaultEncoding; }

	// class-functions:
	static bool IsAddressField( const BString fieldName);
	static bool IsEncodingOkForField( const BString fieldName);

protected:
	void ParseHeader( const BString &header);
	BString ParseHeaderField( BString fieldName, BString fieldValue);
	BString StripField( BString fieldValue, BString* commentBuffer=NULL);

	bool ParseDateTime( const BString& str, time_t& dateTime);

private:
	BString mHeaderString;
							// the complete original mail-header
	BmHeaderList mHeaders;
							// contains all headers as a list of corresponding
							// values (for most fields, this list should only have
							// one item, but for others, e.g. 'Received', the list
							// will have numerous entries.
	BmHeaderList mStrippedHeaders;
							// contains all stripped headers as a list of corresponding
							// values. For simplicity, this map contains even fields for
							// which the stripped value does not make sense, because they
							// aren't structured (e.g. 'Subject'). The stripped versions
							// of these fields' values should not be used, of course.
							// N.B.: 'stripped' actually means that any comments and 
							//       unneccessary whitespace are gone from the field-values.
	BmAddrMap mAddrMap;
							// address-fields with detailed information about all
							// the single address-entries that are contained within
							// each field.
							// In case a complete addresslist is accessed as a BString,
							// it will (in contrast to the stripped-field) deliver a
							// completely parsed and reconstructed version of the address.
							// This results in identical formatting for all addresses
							// (i.e. no '"'s around phrases and the like)
	BmMail* mMail;		
							// The mail these headers belong to
	BString mName;
							// The "name" of the sender of this mail (MAIL:name attribute)
	BString mKey;
							// Since headers have no real key, we generate one from the this-value
	uint32 mDefaultEncoding;
							// charset-encoding to be used by this mail (if not specified otherwise)
	int32 mNumLines;
							// number of lines in this header

	static int32 nCounter;
							// counter for message-id

	// Hide copy-constructor and assignment:
	BmMailHeader( const BmMailHeader&);
	BmMailHeader operator=( const BmMailHeader&);
};

#endif

