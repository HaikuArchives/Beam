/*
	BmMail.h
		$Id$
*/

#ifndef _BmMail_h
#define _BmMail_h

#include <map>

#include <String.h>

/*------------------------------------------------------------------------------*\
	mail_format_error
		-	exception to indicate an error in the format of a mail-message
\*------------------------------------------------------------------------------*/
class mail_format_error : public runtime_error {
public:
  mail_format_error (const BString& what_arg): runtime_error (what_arg.String()) { }
  mail_format_error (char *const what_arg): runtime_error (what_arg) { }
};

/*------------------------------------------------------------------------------*\
	BmMail 
		-	represents a single mail-message in Beam
		-	contains functionality to read/write mails from/to files
		- 	implements all mail-specific text-handling like header-parsing, en-/decoding,
			en-/decrypting
\*------------------------------------------------------------------------------*/
class BmMail {

public:
	BmMail( );
	BmMail( BString &msgText, const BString &msgUID, const BString &account);
	virtual ~BmMail();

private:
	typedef map<BString, BString> HeaderMap;
	HeaderMap mHeaders;						// contains complete headers as fieldname/fieldbody-pairs
	BString mText;								// text of complete message
	BString mUID;								// unique-ID of this message
	BString mAccount;							// name of account this message came from or goes to

	void ParseHeader( const BString &header);
	void Set( BString &msgText, const BString &msgUID, const BString &account);	
};

#endif

