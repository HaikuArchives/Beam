/*
	BmMailReceived.h
		$Id$
*/

#ifndef _BmMailReceived_h
#define _BmMailReceived_h

#include <map>

#include <String.h>

#include "BmUtil.h"

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
	BmMailReceived 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
		- 	implements all mail-specific text-handling like header-parsing, en-/decoding,
			en-/decrypting
\*------------------------------------------------------------------------------*/
class BmMailReceived {

public:
	BmMailReceived( );
	BmMailReceived( BString &msgText, const BString &msgUID, const BString &account);
	virtual ~BmMailReceived();

	// file-related:
	bool Store();
	void StoreAttributes( BFile& mailFile);

protected:
	void Set( BString &msgText, const BString &msgUID, const BString &account);	
	BString CreateBasicFilename();

	void ParseHeader( const BString &header);
	BString ParseHeaderField( BString fieldName, BString fieldValue);

	bool ParseDateTime( const BString& str, time_t& dateTime);
//	bool ParseAddress( const BString& str, BString& addr);
//	bool ParseMultiAddress( const BString& str, BList& addrs);
//	bool ParseMsgID( const BString& str, BString& msgID);
//	bool ParseMailbox( const BString& str, BString& mailbox);

private:
	typedef map<BString, BString> HeaderMap;
	HeaderMap mHeaders;						// contains complete headers as fieldname/fieldbody-pairs
	BString mText;								// text of complete message
	BString mUID;								// unique-ID of this message
	BString mAccountName;					// name of POP-account this message came from
	BString mStatus;							// status of this mail (client-status that is, e.g. "Read" or "New")
	bool mHasAttachments;					// flag indicating the presence of attachments

	BEntry mParentEntry;						// filesystem-entry for mailfolder this mail currently lives in
	BEntry mMailEntry;						// filesystem-entry for this mail (N.B. the entry may
													// be set although the mail does not yet exist on disk)

};

#endif

