/*
	BmMailHeader.h
		$Id$
*/

#ifndef _BmMailHeader_h
#define _BmMailHeader_h

#include <map>
#include <vector>

#include "BmBasics.h"
#include "BmUtil.h"


class BmMail;

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
	// operators:
	operator BString() const;
						// returns address completely formatted (ready to be sent)
	// getters:
	bool InitOK() const						{ return mInitOK; }
	bool HasPhrase() const					{ return mPhrase.Length() > 0; }
	BString Phrase() const					{ return mPhrase; }
	BString AddrSpec() const				{ return mAddrSpec; }

private:
	bool mInitOK;
	BString mPhrase;
	BString mAddrSpec;
};

typedef vector< BString> BmStringList;
/*------------------------------------------------------------------------------*\
	BmAddressList
		-	represents an adress-group or (one or more) mail-addresses 
		-	each address is parsed and split into its components
\*------------------------------------------------------------------------------*/
class BmAddressList {
	typedef vector< BmAddress> BmAddrList;

public:
	// c'tors and d'tor:
	BmAddressList();
	BmAddressList( BString strippedFieldVal);
	~BmAddressList();

	// native methods:
	bool Set( BString strippedFieldVal);
	BmStringList SplitIntoAddresses( BString addrList);
	
	// operators:
	operator BString() const;
							// returns address-list completely formatted (ready to be sent)
	// getters:
	bool InitOK() const						{ return mInitOK; }
	bool IsGroup() const						{ return mIsGroup; }
	int32 AddrCount() const					{ return mAddrList.size(); }
	BString GroupName() const				{ return mGroupName; }
	BmAddress FirstAddress() const		{ return mAddrList[0]; }

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
class BmMailHeader {

	class BmHeaderList {
		typedef vector< BString> BmValueList;
		typedef map< BString, BmValueList> BmHeaderMap;
	public:
		void Set( const BString fieldName, const BString content);
		void Add( const BString fieldName, const BString content);
		void Remove( const BString fieldName);
		BString& operator [] (const BString fieldName);
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
	void SetFieldVal( const BString fieldName, const BString value);
	void AddFieldVal( const BString fieldName, const BString value);
	void RemoveField( const BString fieldName);

	// getters:
	const BString& GetFieldVal( const BString fieldName);
	const BString GetEnhancedFieldVal( const BString fieldName);
	int32 NumLines() const 					{ return mNumLines; }
	const BString& HeaderString() const { return mHeaderString; }

protected:
	void ParseHeader( const BString &header);
	BString ParseHeaderField( BString fieldName, BString fieldValue);
	BString EnhanceField( BString fieldValue, BString* commentBuffer=NULL);

	bool ParseDateTime( const BString& str, time_t& dateTime);

private:
	BString mHeaderString;
							// the complete original mail-header
	BmHeaderList mHeaders;
							// contains all headers as a list of corresponding
							// values (for most fields, this list should only have
							// one item, but for others, e.g. 'Received', the list
							// will have numerous entries.
	BmHeaderList mEnhancedHeaders;
							// contains all stripped headers as a list of corresponding
							// values. For simplicity, this map contains even fields for
							// which the stripped value does not make sense, because they
							// aren't structured (e.g. 'Subject'). The stripped versions
							// of these fields' values should not be used, of course.
							// N.B.: 'stripped' actually means that any comments and 
							//       unneccessary whitespace are gone from the field-values.
	BmAddrMap mAddrMap;
							// address-fields with detailed (parsed) information
	BmMail* mMail;		
							// The mail these headers belong to
	int32 mDefaultEncoding;
							// encoding to be used by this mail (if not specified otherwise)
	int32 mNumLines;
							// number of lines in this header
};

#endif

