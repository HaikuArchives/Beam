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

#define BM_FIELD_FROM 						"From"
#define BM_FIELD_TO 							"To"
#define BM_FIELD_SUBJECT 					"Subject"
#define BM_FIELD_CC 							"Cc"
#define BM_FIELD_BCC 						"Bcc"
#define BM_FIELD_DATE 						"Date"
#define BM_FIELD_CONTENT_TYPE 			"Content-Type"
#define BM_FIELD_CONTENT_DISPOSITION 	"Content-Disposition"
#define BM_FIELD_CONTENT_DESCRIPTION 	"Content-Description"
#define BM_FIELD_CONTENT_LANGUAGE 		"Content-Language"
#define BM_FIELD_CONTENT_TRANSFER_ENCODING 	"Content-Transfer-Encoding"
#define BM_FIELD_CONTENT_ID 				"Content-Id"

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
	BmMail( );
	BmMail( BString &msgText, const BString account=BString());
	BmMail( BmMailRef* ref);
	virtual ~BmMail();

	// native methods:
	bool Store();
	//
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

protected:
	BString CreateBasicFilename();
	void SetTo( BString &msgText, const BString account);
	void StoreAttributes( BFile& mailFile);

	BmRef<BmMailRef> mMailRef;
	status_t mInitCheck;

private:
	BmMailHeader* mHeader;					// contains header-information
	int32 mHeaderLength;
	BmRef<BmBodyPartList> mBody;			// contains body-information (split into subparts)

	BString mText;								// text of complete message
	BString mAccountName;					// name of account this message came from

	BEntry mParentEntry;						// filesystem-entry for mailfolder this mail 
													// lives in or should be stored into
};


#endif
