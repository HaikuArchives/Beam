/*
	BmMail.cpp
		$Id$
*/


#include <regexx/regexx.hh>
using namespace regexx;

#include "BmUtil.h"
#include "BmMail.h"

#undef LOGNAME
#define LOGNAME mAccount

/*------------------------------------------------------------------------------*\
	default constructor
\*------------------------------------------------------------------------------*/
BmMail::BmMail( ) {
}

/*------------------------------------------------------------------------------*\
	constructor( msgText, msgUID)
		-	creates message with given text and unique-ID (as received from server)
\*------------------------------------------------------------------------------*/
BmMail::BmMail( BString &msgText, const BString &msgUID, const BString &account) {
	Set( msgText, msgUID, account);
}
	
/*------------------------------------------------------------------------------*\
	destructor
\*------------------------------------------------------------------------------*/
BmMail::~BmMail() {
}

/*------------------------------------------------------------------------------*\
	Set( msgText, msgUID)
		-	initializes mail-object with given data
		-	the mail-header is extracted from msgText and is parsed
		-	account is the name of the POP/IMAP-account this message was received from
\*------------------------------------------------------------------------------*/
void BmMail::Set( BString &msgText, const BString &msgUID, const BString &account) {
	int32 headerLen = msgText.FindFirst( "\r\n\r\n");
							// STD11: empty-line seperates header from body
	if (headerLen == B_ERROR) {
		throw mail_format_error("BmMail: Could not determine borderline between header and text of message");
	}
	headerLen += 2;							// include cr/nl in header-string

	mText.Adopt( msgText);					// take over the msg-string

	BString header;
	header.SetTo( mText, headerLen);
	mAccount = account;
	ParseHeader( header);
	mUID = msgUID;
}
	
/*------------------------------------------------------------------------------*\
	ParseHeader( header)
		-	parses mail-header and splits it into fieldname/fieldbody - pairs
\*------------------------------------------------------------------------------*/
void BmMail::ParseHeader( const BString &header) {
	Regexx rxx;
	rxx.expr( "^(\\S.+?\\r\\n(?:\\s.+?\\r\\n)*)(?=(\\Z|\\S))");
	rxx.str( header.String());
	int nm=rxx.exec( Regexx::global | Regexx::newline);
	vector<RegexxMatch>::const_iterator i;
	BString str;
	str << nm << " headerfields found\n";
	for( i = rxx.match.begin(); i != rxx.match.end(); ++i) {
		BString s;
		mText.CopyInto( s, i->start(), i->length());
		str << s << "\n------------------\n";
	}
	BmLOG( str);
}
