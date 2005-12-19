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

#include "BmMailKit.h"

#include <vector>

#include <E-mail.h>
#include <Entry.h>
#include <Mime.h>
#include <Path.h>
#include "BmString.h"

#include "BmBodyPartList.h"
#include "BmDataModel.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"
#include "BmUtil.h"

class BmIdentity;

// mail-attribute types:
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_NAME;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_STATUS;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_PRIORITY;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_TO;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_CC;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_FROM;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_SUBJECT;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_REPLY;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_WHEN;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_FLAGS;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_RECIPIENTS;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_MIME;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_HEADER;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_CONTENT;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_ATTACHMENTS;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_ACCOUNT;
// attributes introduced by MDR
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_CLASSIFICATION;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_RATIO_SPAM;
// Beam's own attributes:
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_IDENTITY;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_MARGIN;
extern IMPEXPBMMAILKIT const char* BM_MAIL_ATTR_WHEN_CREATED;

extern IMPEXPBMMAILKIT const char* BM_FIELD_BCC;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CC;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_TYPE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_DISPOSITION;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_DESCRIPTION;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_LANGUAGE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_TRANSFER_ENCODING;
extern IMPEXPBMMAILKIT const char* BM_FIELD_CONTENT_ID;
extern IMPEXPBMMAILKIT const char* BM_FIELD_DATE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_FROM;
extern IMPEXPBMMAILKIT const char* BM_FIELD_IN_REPLY_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_ARCHIVE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_HELP;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_ID;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_POST;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_SUBSCRIBE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_LIST_UNSUBSCRIBE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_MAIL_FOLLOWUP_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_MAIL_REPLY_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_MAILING_LIST;
extern IMPEXPBMMAILKIT const char* BM_FIELD_MESSAGE_ID;
extern IMPEXPBMMAILKIT const char* BM_FIELD_MIME;
extern IMPEXPBMMAILKIT const char* BM_FIELD_PRIORITY;
extern IMPEXPBMMAILKIT const char* BM_FIELD_REFERENCES;
extern IMPEXPBMMAILKIT const char* BM_FIELD_REPLY_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_BCC;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_CC;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_DATE;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_FROM;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_MESSAGE_ID;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_REPLY_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_SENDER;
extern IMPEXPBMMAILKIT const char* BM_FIELD_RESENT_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_SENDER;
extern IMPEXPBMMAILKIT const char* BM_FIELD_SUBJECT;
extern IMPEXPBMMAILKIT const char* BM_FIELD_TO;
extern IMPEXPBMMAILKIT const char* BM_FIELD_USER_AGENT;
extern IMPEXPBMMAILKIT const char* BM_FIELD_X_LIST;
extern IMPEXPBMMAILKIT const char* BM_FIELD_X_MAILER;
extern IMPEXPBMMAILKIT const char* BM_FIELD_X_PRIORITY;

extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_DRAFT;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_ERROR;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_FORWARDED;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_NEW;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_PENDING;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_READ;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_REDIRECTED;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_REPLIED;
extern IMPEXPBMMAILKIT const char* BM_MAIL_STATUS_SENT;

extern IMPEXPBMMAILKIT const char* BM_MAIL_CLASS_SPAM;
extern IMPEXPBMMAILKIT const char* BM_MAIL_CLASS_TOFU;

class BmFilter;
/*------------------------------------------------------------------------------*\
	BmMail 
		-	represents a single mail-message in Beam
		-	contains functionality to parse mail-headers and body-parts
		-	can instantiate a mail from text or from a file
		-	can store mail in a file and send it via ESMTP
		-	is a job-model (thus can be controlled by a view [e.g. BmMailView])
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMail : public BmJobModel {
	typedef BmJobModel inherited;

	typedef map<int32,BmString> BmQuoteLevelMap;
	typedef vector< BmRef< BmMailRef> > BmMailRefVect;

public:
	static BmRef<BmMail> CreateInstance( BmMailRef* ref);
	BmMail( bool outbound);
	BmMail( BmString &msgText, const BmString account=BmString());
	virtual ~BmMail();

	// native methods:
	void ConstructAndStore();
	bool ConstructRawText( const BmString& editableUtf8Text, 
								  const BmString& charset,
								  BmString smtpAccount);
	void SetTo( BmString &text, const BmString account);
	void SetNewHeader( const BmString& headerStr);
	void SetSignatureByName( const BmString sigName);
	void SetupFromIdentityAndRecvAddr( BmIdentity* ident, 
												  const BmString& recvAddr);
	//
	void ApplyOutboundFilters();
	void ApplyInboundFilters();
	bool Send( bool now=true);
	bool Store();
	void StoreIntoFile( BDirectory* destDir, BmString filename, 
							  const BmString& status, bigtime_t whenCreated, 
							  BEntry* backupEntry = NULL);
	void ResyncFromDisk();
	//
	const BmString& GetFieldVal( const BmString fieldName);
	bool HasAttachments() const;
	bool HasComeFromList() const;
	void DetermineRecvAddrAndIdentity( BmString& receivingAddr,
												  BmRef<BmIdentity>& ident);
	void MarkAs( const char* status);
	void RemoveField( const BmString fieldName);
	void SetFieldVal( const BmString fieldName, const BmString value);
	bool IsFieldEmpty( const BmString fieldName);
	const BmString Status() const;
	//
	void RatioSpam( float rs);
	float RatioSpam() const;
	void MarkAsSpam();
	void MarkAsTofu();
	bool IsMarkedAsSpam() const;
	bool IsMarkedAsTofu() const;
	//
	void AddAttachmentFromRef( const entry_ref* ref,
										const BmString& defaultCharset);
	//
	bool SetDestFolderName( const BmString& destFolderName);
	BmRef<BmMailFolder> DestFolder() const;
	//	
	bool MoveToDestFolder();
	//
	void SetBaseMailInfo( BmMailRef* ref, const BmString newStatus);
	void AddBaseMailRef( BmMailRef* ref);

	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	inline const status_t InitCheck() const	
													{ return mInitCheck; }
	inline const BmString& AccountName(){ return mAccountName; }
	BmBodyPartList* Body() const;
	BmMailHeader* Header() const;
	int32 HeaderLength() const;
	inline int32 RightMargin() const		{ return mRightMargin; }
	inline const BmString& RawText() const		
													{ return mText; }
	const BmString& HeaderText() const;
	inline const bool Outbound() const	{ return mOutbound; }
	bool IsRedirect() const;
	BmMailRef* MailRef() const;
	const BmString& DefaultCharset()	const;
	inline BmString SignatureName() const		
													{ return mSignatureName; }
	inline const BmString& IdentityName() const	
													{ return mIdentityName; }

	// setters:
	inline void BumpRightMargin( int32 i)		
													{ mRightMargin = MAX(i,mRightMargin); }
	inline void RightMargin( int32 i)	{ mRightMargin = i; }
	void IsRedirect( bool b);
	inline void Outbound( bool b)			{ mOutbound = b; }
	inline void AccountName( const BmString& s)
													{ mAccountName = s; }
	inline void IdentityName( const BmString& s)
													{ mIdentityName = s; }
	inline void Classification( const BmString& s)
													{ mClassification = s; }
	inline void MoveToTrash( bool b)		{ mMoveToTrash = b; }

	static const int32 BM_READ_MAIL_JOB = 1;

protected:
	BmMail( BmMailRef* ref);

	BmString CreateBasicFilename();
	void StoreAttributes( BNode& mailNode, const BmString& status, 
								 bigtime_t whenCreated);

private:
	void SetDefaultHeaders( const BmString& defaultHeaders);
	BmMail();
	
	const BmString DefaultStatus() const;

	BmRef<BmMailHeader> mHeader;
							// contains header-information
	BmRef<BmBodyPartList> mBody;
							// contains body-information (split into subparts)
	BmString mText;
							// text of complete message
	BmString mAccountName;
							// name of account this message came from/is sent through
	BmString mIdentityName;
							// name of identity this message belongs to
	BmString mClassification;
							// genuine or spam
	float mRatioSpam;
							// 0.00 (genuine) .. 1.0 (spam)
	BEntry mEntry;
							// filesystem-entry for this mail 
	BmRef<BmMailRef> mMailRef;
							// the mail-ref that corresponds to this mail. 
							// This exists if (and only if) a mail lives on disk
	bool mOutbound;
							// true if mail is for sending (as opposed to received)
	int32 mRightMargin;
							// the current right-margin for this mail
	BmMailRefVect mBaseRefVect;
							// the mailref(s) that created us (via forward/reply)
	BmString mNewBaseStatus;
							// new status of base mail (forwarded/replied)
	BmString mSignatureName;
							// name of signature to use in this mail
	BmRef<BmMailHeader> mDefaultHeader;
							// default header-values (comes from identity)
	bool mMoveToTrash;
							// indicates that this mail should be trashed (after being
							// stored)
	mutable BmString mDestFolderName;
							// name of folder where the mail shall be stored into
							// (this may be changed by a mail-filter)
	mutable BmString mDefaultStatus;
							// default status of this mail, only relevant
							// before mail lives on disk
	status_t mInitCheck;

	// Hide copy-constructor and assignment:
	BmMail( const BmMail&);
	BmMail operator=( const BmMail&);
};

#endif
