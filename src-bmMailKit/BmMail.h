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
#include "BmMailHeader.h"
#include "BmUtil.h"

class BmBodyPartList;
class BmMailRef;

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
#define BM_FIELD_IN_REPLY_TO				"In-Reply-To"
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
	static BmRef<BmMail> CreateInstance( BmMailRef* ref);
	BmMail( bool outbound);
	BmMail( BString &msgText, const BString account=BString());
	virtual ~BmMail();

	// native methods:
	bool ConstructRawText( const BString& editableText, int32 encoding,
								  BString smtpAccount);
	void SetTo( const BString &text, const BString account);
	void SetNewHeader( const BString& headerStr);
	bool Store();
	//
	const BString& GetFieldVal( const BString fieldName);
	BString GetStrippedFieldVal( const BString fieldName);
	bool HasAttachments() const;
	void MarkAs( const char* status);
	void RemoveField( const BString fieldName);
	void SetFieldVal( const BString fieldName, const BString value);
	const BString Status() const;
	//
	BmRef<BmMail> CreateAttachedForward();
	BmRef<BmMail> CreateInlineForward( bool withAttachments, 
										  		  const BString selectedText="");
	BmRef<BmMail> CreateReply( bool replyToAll, const BString selectedText="");
	BmRef<BmMail> CreateResend();
	
	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	inline const status_t InitCheck() const	{ return mInitCheck; }
	inline const BString& AccountName()			{ return mAccountName; }
	inline BmBodyPartList* Body() const			{ return mBody.Get(); }
	inline BmRef<BmMailHeader> Header() const	{ return mHeader; }
	inline int32 HeaderLength() const			{ return mHeaderLength; }
	inline const BString& RawText() const		{ return mText; }
	inline const bool Outbound() const			{ return mOutbound; }
	inline BmMailRef* MailRef() const			{ return mMailRef.Get(); }
	uint32 DefaultEncoding()	const;

	// static function that tries to reformat & quote a given multiline text
	// in a way that avoids the usual (ugly) quoting-mishaps:
	void QuoteText( const BString& in, BString& out, BString quote, int maxLen);

	static const int32 BM_READ_MAIL_JOB = 1;

protected:
	BmMail( BmMailRef* ref);

	BString CreateBasicFilename();
	void StoreAttributes( BFile& mailFile);
	BString CreateReplySubjectFor( const BString subject);
	BString CreateForwardSubjectFor( const BString subject);
	BString CreateReplyIntro();
	BString CreateForwardIntro();
	void SetBaseMailInfo( BmMailRef* ref, const BString newStatus);

	BmRef<BmMailRef> mMailRef;
	status_t mInitCheck;

private:
	BmMail();
	
	const BString DefaultStatus() const;

	BmRef<BmMailHeader> mHeader;			// contains header-information
	int32 mHeaderLength;

	BmRef<BmBodyPartList> mBody;			// contains body-information (split into subparts)

	BString mText;								// text of complete message

	BString mAccountName;					// name of account this message came from

	BEntry mEntry;								// filesystem-entry for this mail 

	bool mOutbound;							// true if mail is for sending (as opposed to reveived)

	BmRef<BmMailRef> mBaseMailRef;		// the mailref that created us (via forward/reply)
	BString mNewBaseStatus;					// new status of base mail (forwarded/replied)

	// Hide copy-constructor and assignment:
	BmMail( const BmMail&);
	BmMail operator=( const BmMail&);
};


#endif
