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
#include <Path.h>
#include "BmString.h"

#include "BmDataModel.h"
#include "BmUtil.h"

class BmBodyPartList;
class BmIdentity;
class BmMailHeader;
class BmMailRef;

// mail-attribute types:
extern const char* BM_MAIL_ATTR_NAME;
extern const char* BM_MAIL_ATTR_STATUS;
extern const char* BM_MAIL_ATTR_PRIORITY;
extern const char* BM_MAIL_ATTR_TO;
extern const char* BM_MAIL_ATTR_CC;
extern const char* BM_MAIL_ATTR_FROM;
extern const char* BM_MAIL_ATTR_SUBJECT;
extern const char* BM_MAIL_ATTR_REPLY;
extern const char* BM_MAIL_ATTR_WHEN;
extern const char* BM_MAIL_ATTR_FLAGS;
extern const char* BM_MAIL_ATTR_RECIPIENTS;
extern const char* BM_MAIL_ATTR_MIME;
extern const char* BM_MAIL_ATTR_HEADER;
extern const char* BM_MAIL_ATTR_CONTENT;
extern const char* BM_MAIL_ATTR_ATTACHMENTS;
extern const char* BM_MAIL_ATTR_ACCOUNT;
// Beam's own attributes:
extern const char* BM_MAIL_ATTR_IDENTITY;
extern const char* BM_MAIL_ATTR_MARGIN;
extern const char* BM_MAIL_ATTR_WHEN_CREATED;

extern const char* BM_FIELD_BCC;
extern const char* BM_FIELD_CC;
extern const char* BM_FIELD_CONTENT_TYPE;
extern const char* BM_FIELD_CONTENT_DISPOSITION;
extern const char* BM_FIELD_CONTENT_DESCRIPTION;
extern const char* BM_FIELD_CONTENT_LANGUAGE;
extern const char* BM_FIELD_CONTENT_TRANSFER_ENCODING;
extern const char* BM_FIELD_CONTENT_ID;
extern const char* BM_FIELD_DATE;
extern const char* BM_FIELD_FROM;
extern const char* BM_FIELD_IN_REPLY_TO;
extern const char* BM_FIELD_LIST_ARCHIVE;
extern const char* BM_FIELD_LIST_HELP;
extern const char* BM_FIELD_LIST_ID;
extern const char* BM_FIELD_LIST_POST;
extern const char* BM_FIELD_LIST_SUBSCRIBE;
extern const char* BM_FIELD_LIST_UNSUBSCRIBE;
extern const char* BM_FIELD_MAIL_FOLLOWUP_TO;
extern const char* BM_FIELD_MAIL_REPLY_TO;
extern const char* BM_FIELD_MAILING_LIST;
extern const char* BM_FIELD_MESSAGE_ID;
extern const char* BM_FIELD_MIME;
extern const char* BM_FIELD_PRIORITY;
extern const char* BM_FIELD_REFERENCES;
extern const char* BM_FIELD_REPLY_TO;
extern const char* BM_FIELD_RESENT_BCC;
extern const char* BM_FIELD_RESENT_CC;
extern const char* BM_FIELD_RESENT_DATE;
extern const char* BM_FIELD_RESENT_FROM;
extern const char* BM_FIELD_RESENT_MESSAGE_ID;
extern const char* BM_FIELD_RESENT_REPLY_TO;
extern const char* BM_FIELD_RESENT_SENDER;
extern const char* BM_FIELD_RESENT_TO;
extern const char* BM_FIELD_SENDER;
extern const char* BM_FIELD_SUBJECT;
extern const char* BM_FIELD_TO;
extern const char* BM_FIELD_USER_AGENT;
extern const char* BM_FIELD_X_LIST;
extern const char* BM_FIELD_X_MAILER;
extern const char* BM_FIELD_X_PRIORITY;

extern const char* BM_MAIL_STATUS_DRAFT;
extern const char* BM_MAIL_STATUS_FORWARDED;
extern const char* BM_MAIL_STATUS_NEW;
extern const char* BM_MAIL_STATUS_PENDING;
extern const char* BM_MAIL_STATUS_READ;
extern const char* BM_MAIL_STATUS_REDIRECTED;
extern const char* BM_MAIL_STATUS_REPLIED;
extern const char* BM_MAIL_STATUS_SENT;

extern const char* BM_MAIL_FOLDER_DRAFT;
extern const char* BM_MAIL_FOLDER_IN;
extern const char* BM_MAIL_FOLDER_OUT;

class BmFilter;
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
	void ApplyFilter( bool storeIfNeeded=false);
	bool Store();
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
	void AddAttachmentFromRef( const entry_ref* ref,
										const BmString& defaultCharset);
	//
	bool SetDestFoldername( const BmString& destFoldername);
	const BPath& DestFolderpath() const;
	//	
	bool MoveToDestFolderpath();
	//
	void SetBaseMailInfo( BmMailRef* ref, const BmString newStatus);
	void AddBaseMailRef( BmMailRef* ref);

	// overrides of jobmodel base:
	bool StartJob();

	// getters:
	inline const status_t InitCheck() const	
													{ return mInitCheck; }
	inline const BmString& AccountName(){ return mAccountName; }
	inline BmBodyPartList* Body() const	{ return mBody.Get(); }
	inline BmMailHeader* Header() const	{ return mHeader.Get(); }
	int32 HeaderLength() const;
	inline int32 RightMargin() const		{ return mRightMargin; }
	inline const BmString& RawText() const		
													{ return mText; }
	const BmString& HeaderText() const;
	inline const bool Outbound() const	{ return mOutbound; }
	bool IsRedirect() const;
	inline BmMailRef* MailRef() const	{ return mMailRef.Get(); }
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
	inline void MoveToTrash( bool b)		{ mMoveToTrash = b; }

	static const int32 BM_READ_MAIL_JOB = 1;

protected:
	BmMail( BmMailRef* ref);

	BmString CreateBasicFilename();
	void StoreAttributes( BFile& mailFile, const BmString& status, 
								 bigtime_t whenCreated);

private:
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
	bool mMoveToTrash;
							// indicates that this mail should be trashed (after being
							// stored)
	mutable BPath mDestFolderpath;
							// path to folder where the mail shall be stored in
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
