/*
	BmMail.h
		$Id$
*/

#ifndef _BmMail_h
#define _BmMail_h

#include <vector>

#include <E-mail.h>
#include <Entry.h>
#include <Mime.h>

#include "BmDataModel.h"
#include "BmUtil.h"

class BmBodyPartList;
class BmMailRef;
class BmMailHeader;

// mail-attribute types:
#define BM_MAIL_ATTR_NAME 			B_MAIL_ATTR_NAME
#define BM_MAIL_ATTR_STATUS		B_MAIL_ATTR_STATUS
#define BM_MAIL_ATTR_PRIORITY		B_MAIL_ATTR_PRIORITY
#define BM_MAIL_ATTR_TO				B_MAIL_ATTR_TO
#define BM_MAIL_ATTR_CC				B_MAIL_ATTR_CC
#define BM_MAIL_ATTR_FROM			B_MAIL_ATTR_FROM
#define BM_MAIL_ATTR_SUBJECT		B_MAIL_ATTR_SUBJECT
#define BM_MAIL_ATTR_REPLY			B_MAIL_ATTR_REPLY
#define BM_MAIL_ATTR_WHEN			B_MAIL_ATTR_WHEN
#define BM_MAIL_ATTR_FLAGS			B_MAIL_ATTR_FLAGS
#define BM_MAIL_ATTR_RECIPIENTS	B_MAIL_ATTR_RECIPIENTS
#define BM_MAIL_ATTR_MIME			B_MAIL_ATTR_MIME
#define BM_MAIL_ATTR_HEADER		B_MAIL_ATTR_HEADER
#define BM_MAIL_ATTR_CONTENT		B_MAIL_ATTR_CONTENT
#define BM_MAIL_ATTR_ATTACHMENTS "MAIL:has_attachment"
#define BM_MAIL_ATTR_ACCOUNT	 	"MAIL:account"

#define BM_FIELD_BCC 						"Bcc"
#define BM_FIELD_CC 							"Cc"
#define BM_FIELD_CONTENT_TYPE 			"Content-Type"
#define BM_FIELD_CONTENT_DISPOSITION 	"Content-Disposition"
#define BM_FIELD_CONTENT_DESCRIPTION 	"Content-Description"
#define BM_FIELD_CONTENT_LANGUAGE 		"Content-Language"
#define BM_FIELD_CONTENT_TRANSFER_ENCODING 	"Content-Transfer-Encoding"
#define BM_FIELD_CONTENT_ID 				"Content-Id"
#define BM_FIELD_DATE 						"Date"
#define BM_FIELD_FROM 						"From"
#define BM_FIELD_MESSAGE_ID				"Message-Id"
#define BM_FIELD_MIME 						"Mime-Version"
#define BM_FIELD_PRIORITY					"Priority"
#define BM_FIELD_REFERENCES				"References"
#define BM_FIELD_REPLY_TO					"Reply-To"
#define BM_FIELD_RESENT_BCC				"Resent-Bcc"
#define BM_FIELD_RESENT_CC					"Resent-Cc"
#define BM_FIELD_RESENT_DATE				"Resent-Date"
#define BM_FIELD_RESENT_FROM				"Resent-From"
#define BM_FIELD_RESENT_SENDER 			"Resent-Sender"
#define BM_FIELD_RESENT_TO					"Resent-To"
#define BM_FIELD_SENDER 					"Sender"
#define BM_FIELD_SUBJECT 					"Subject"
#define BM_FIELD_TO 							"To"
#define BM_FIELD_USER_AGENT				"User-Agent"
#define BM_FIELD_X_MAILER					"X-Mailer"
#define BM_FIELD_X_PRIORITY				"X-Priority"

#define BM_MAIL_STATUS_DRAFT			"Draft"
#define BM_MAIL_STATUS_FORWARDED		"Forwarded"
#define BM_MAIL_STATUS_NEW				"New"
#define BM_MAIL_STATUS_PENDING		"Pending"
#define BM_MAIL_STATUS_READ			"Read"
#define BM_MAIL_STATUS_REDIRECTED	"Redirected"
#define BM_MAIL_STATUS_REPLIED		"Replied"
#define BM_MAIL_STATUS_SENT			"Sent"

/*------------------------------------------------------------------------------*\
	BmMail 
		-	represents a single mail-message in Beam
		-	contains functionality to parse mail-headers and body-parts
		-	can instantiate a mail from text or from a file
		-	can store mail in a file and send it via ESMTP
		-	is a job-model (thus can be controlled by a view [e.g. BmMailView])
\*------------------------------------------------------------------------------*/
class BmMail : public BmJobModel {
	typedef BmJobModel inherited;

public:
	BmMail( bool outbound);
	BmMail( BString &msgText, const BString account=BString());
	BmMail( BmMailRef* ref);
	virtual ~BmMail();

	// native methods:
	bool ConstructRawText( const BString& editableText, int32 encoding,
								  BString smtpAccount);
	void SetTo( const BString &text, const BString account);
	bool Store();
	//
	const BString& GetFieldVal( const BString fieldName);
	bool HasAttachments() const;
	void MarkAs( const char* status);
	void RemoveField( const BString fieldName);
	void SetFieldVal( const BString fieldName, const BString value);
	const BString Status() const;
	
	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	const status_t InitCheck()	const		{ return mInitCheck; }
	const BString& AccountName()			{ return mAccountName; }
	BmBodyPartList* Body() const			{ return mBody.Get(); }
	BmMailHeader* Header() const			{ return mHeader; }
	int32 HeaderLength() const				{ return mHeaderLength; }
	const BString& RawText() const		{ return mText; }
	const bool Outbound() const			{ return mOutbound; }
	uint32 DefaultEncoding()	const;

protected:
	BString CreateBasicFilename();
	void StoreAttributes( BFile& mailFile);

	BmRef<BmMailRef> mMailRef;
	status_t mInitCheck;

private:
	BmMail();
	
	const BString DefaultStatus() const;

	BmMailHeader* mHeader;					// contains header-information
	int32 mHeaderLength;

	BmRef<BmBodyPartList> mBody;			// contains body-information (split into subparts)

	BString mText;								// text of complete message

	BString mAccountName;					// name of account this message came from

	BEntry mEntry;								// filesystem-entry for this mail 

	bool mOutbound;							// true if mail is for sending (as opposed to reveived)

	// Hide copy-constructor and assignment:
	BmMail( const BmMail&);
	BmMail operator=( const BmMail&);
};


#endif
