/*
	BmMail.h
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
//
#define BM_MAIL_ATTR_MARGIN	 	"MAIL:margin"

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
#define BM_FIELD_LIST_ARCHIVE				"List-Archive"
#define BM_FIELD_LIST_HELP					"List-Help"
#define BM_FIELD_LIST_ID					"List-Id"
#define BM_FIELD_LIST_POST					"List-Post"
#define BM_FIELD_LIST_SUBSCRIBE			"List-Subscribe"
#define BM_FIELD_LIST_UNSUBSCRIBE		"List-Unsubscribe"
#define BM_FIELD_MESSAGE_ID				"Message-Id"
#define BM_FIELD_MIME 						"Mime-Version"
#define BM_FIELD_PRIORITY					"Priority"
#define BM_FIELD_REFERENCES				"References"
#define BM_FIELD_REPLY_TO					"Reply-To"
#define BM_FIELD_RESENT_BCC				"Resent-Bcc"
#define BM_FIELD_RESENT_CC					"Resent-Cc"
#define BM_FIELD_RESENT_DATE				"Resent-Date"
#define BM_FIELD_RESENT_FROM				"Resent-From"
#define BM_FIELD_RESENT_MESSAGE_ID		"Resent-Message-Id"
#define BM_FIELD_RESENT_REPLY_TO			"Resent-Reply-To"
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

	typedef map<int32,BString> BmQuoteLevelMap;
	typedef vector< BmRef< BmMailRef> > BmMailRefVect;

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
	void SetSignatureByName( const BString sigName, int32 encoding);
	bool Store();
	void ResyncFromDisk();
	//
	const BString& GetFieldVal( const BString fieldName);
	BString GetStrippedFieldVal( const BString fieldName);
	bool HasAttachments() const;
	bool HasComeFromList() const;
	void MarkAs( const char* status);
	void RemoveField( const BString fieldName);
	void SetFieldVal( const BString fieldName, const BString value);
	inline bool IsFieldEmpty( const BString fieldName) { 
													return mHeader 
														? mHeader->IsFieldEmpty( fieldName) 
														: true; 
	}
	const BString Status() const;
	//
	BmRef<BmMail> CreateAttachedForward();
	BmRef<BmMail> CreateInlineForward( bool withAttachments, 
										  		  const BString selectedText="");
	BmRef<BmMail> CreateReply( int32 replyMode, const BString selectedText="");
	BmRef<BmMail> CreateRedirect();
	BString CreateReplySubjectFor( const BString subject);
	BString CreateForwardSubjectFor( const BString subject);
	BString CreateReplyIntro();
	BString CreateForwardIntro();
	//
	void AddAttachmentFromRef( const entry_ref* ref);
	void AddPartsFromMail( BmRef<BmMail> mail, bool withAttachments,
								  bool isForward,
								  const BString selectedText="");
	
	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	inline const status_t InitCheck() const	{ return mInitCheck; }
	inline const BString& AccountName()			{ return mAccountName; }
	inline BmBodyPartList* Body() const			{ return mBody.Get(); }
	inline BmRef<BmMailHeader> Header() const	{ return mHeader; }
	inline int32 HeaderLength() const			{ return mHeaderLength; }
	inline int32 RightMargin() const				{ return mRightMargin; }
	inline const BString& RawText() const		{ return mText; }
	inline const bool Outbound() const			{ return mOutbound; }
	inline const bool IsRedirect() const		{ return mHeader ? mHeader->IsRedirect() : false; }
	inline BmMailRef* MailRef() const			{ return mMailRef.Get(); }
	uint32 DefaultEncoding()	const;
	inline BString SignatureName() const		{ return mSignatureName; }

	// setters:
	inline void BumpRightMargin( int32 i)		{ mRightMargin = MAX(i,mRightMargin); }
	inline void RightMargin( int32 i)			{ mRightMargin = i; }
	inline void IsRedirect( bool b)				{ if (mHeader) mHeader->IsRedirect( b); }
	inline void AccountName( const BString& s){ mAccountName = s; }

	// static functions that try to reformat & quote a given multiline text
	// in a way that avoids the usual (ugly) quoting-mishaps.
	// the resulting int is the line-length needed to leave the formatting intact.
	static int32 QuoteText( const BString& in, BString& out, BString quote, int maxLen);

	static const int32 BM_READ_MAIL_JOB = 1;

	static const char* const BM_QUOTE_AUTO_WRAP = "Auto Wrap";
	static const char* const BM_QUOTE_SIMPLE = "Simple";
	static const char* const BM_QUOTE_PUSH_MARGIN = "Push Margin";

protected:
	BmMail( BmMailRef* ref);

	BString CreateBasicFilename();
	void StoreAttributes( BFile& mailFile);
	void SetBaseMailInfo( BmMailRef* ref, const BString newStatus);
	void AddBaseMailRef( BmMailRef* ref);

	// static functions used for quote-formatting:
	static int32 QuoteTextWithReWrap( const BString& in, BString& out, 
											    BString quoteString, int maxLineLen);
	static int32 AddQuotedText( const BString& text, BString& out, 
										 const BString& quote, const BString& quoteString,
								 		 int maxTextLen);

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

	int32 mRightMargin;						// the current right-margin for this mail

	BmMailRefVect mBaseRefVect;			// the mailref(s) that created us (via forward/reply)
	BString mNewBaseStatus;					// new status of base mail (forwarded/replied)

	BString mSignatureName;					// name of signature to use in this mail

	// Hide copy-constructor and assignment:
	BmMail( const BmMail&);
	BmMail operator=( const BmMail&);
};

// convenience-defines for AddPartsFromMail()-param isForward:
#define BM_IS_FORWARD true
#define BM_IS_REPLY  false

#endif
