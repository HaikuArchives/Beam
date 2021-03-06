/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRef_h
#define _BmMailRef_h

#include "BmMailKit.h"

#include <vector>

#include <Entry.h>
#include <Node.h>

#include "BmString.h"
#include "BmDataModel.h"

class BmMail;
class BmMailRefList;
/*------------------------------------------------------------------------------*\
	BmMailRef
		-	class 
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailRef : public BmListModelItem {
	typedef BmListModelItem inherited;

	// archival-fieldnames:
	static const char* const MSG_ACCOUNT;
	static const char* const MSG_ATTACHMENTS;
	static const char* const MSG_CC;
	static const char* const MSG_ENTRYREF;
	static const char* const MSG_FROM;
	static const char* const MSG_INODE;
	static const char* const MSG_NAME;
	static const char* const MSG_PRIORITY;
	static const char* const MSG_WHEN_CREATED;
	static const char* const MSG_REPLYTO;
	static const char* const MSG_SIZE;
	static const char* const MSG_STATUS;
	static const char* const MSG_SUBJECT;
	static const char* const MSG_TO;
	static const char* const MSG_WHEN;
	static const char* const MSG_IDENTITY;
	static const char* const MSG_IS_VALID;
	static const char* const MSG_CLASSIFICATION;
	static const char* const MSG_RATIO_SPAM;
	static const char* const MSG_IMAP_UID;
	static const int16 nArchiveVersion;

public:
	// creator-funcs, c'tors and d'tor:
	static BmRef<BmMailRef> CreateInstance( entry_ref &eref, 
												 		 struct stat* st = NULL);
	static BmRef<BmMailRef> CreateInstance( BMessage* archive);
	virtual ~BmMailRef();

	// native methods:
	void MarkAs( const char* s);
	bool ReadAttributes( const struct stat* statInfo = NULL,
								BmUpdFlags* updFlagsOut = NULL);
	void ResyncFromDisk( entry_ref* newRef = NULL,
								const struct stat* statInfo = NULL);
	void MarkAsSpam();
	void MarkAsTofu();

	// overrides of archivable base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const entry_ref& EntryRef() const 		
													{ return mEntryRef; }
	inline const entry_ref* EntryRefPtr() const	
													{ return &mEntryRef; }
	inline const char* TrackerName() const			
													{ return mEntryRef.name; }
	inline const node_ref& NodeRef() const
													{ return mNodeRef; }
	inline status_t InitCheck()	const	{ return mInitCheck; }
	inline const BmString& ImapUID() const
											 		{ return mImapUID; }
	inline const BmString& Account() const
											 		{ return mAccount; }
	inline const BmString& Cc() const 	{ return mCc; }
	inline const BmString& From() const { return mFrom; }
	inline const BmString& Name() const	{ return mName; }
	inline const BmString& Priority() const
											 		{ return mPriority; }
	inline const BmString& ReplyTo() const
											 		{ return mReplyTo; }
	inline const BmString& Status() const
										 			{ return mStatus; }
	inline const BmString& Subject() const
											 		{ return mSubject; }
	inline const BmString& To() const 	{ return mTo; }
	inline const time_t& When() const 	{ return mWhen; }
	inline const bigtime_t& WhenCreated() const
													{ return mWhenCreated; }
	inline const off_t& Size() const 	{ return mSize; }
	inline const BmString& SizeString() const
												 	{ return mSizeString; }
	inline const bool HasAttachments() const
												 	{ return mHasAttachments; }
	const bool IsSpecial() const;
	inline const BmString& Identity() const
											 		{ return mIdentity; }
	inline const BmString& Classification() const
											 		{ return mClassification; }
	inline float RatioSpam() const		{ return mRatioSpam; }
	inline const BmString& RatioSpamString() const 
													{ return mRatioSpamString; }

	// setters:
	inline void EntryRef( entry_ref &e) { mEntryRef = e; }
	inline void WhenCreated( const bigtime_t& t)
													{ mWhenCreated = t; }
	inline void Classification( const BmString& c)
													{ mClassification = c; }
	void RatioSpam( float rs);

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_ACCOUNT			= 1<<2;
	static const BmUpdFlags UPD_ATTACHMENTS	= 1<<3;
	static const BmUpdFlags UPD_CC				= 1<<4;
	static const BmUpdFlags UPD_FROM				= 1<<5;
	static const BmUpdFlags UPD_NAME				= 1<<6;
	static const BmUpdFlags UPD_PRIORITY		= 1<<7;
	static const BmUpdFlags UPD_WHEN_CREATED	= 1<<8;
	static const BmUpdFlags UPD_REPLYTO			= 1<<9;
	static const BmUpdFlags UPD_SIZE				= 1<<10;
	static const BmUpdFlags UPD_STATUS			= 1<<11;
	static const BmUpdFlags UPD_SUBJECT			= 1<<12;
	static const BmUpdFlags UPD_TO				= 1<<13;
	static const BmUpdFlags UPD_WHEN				= 1<<14;
	static const BmUpdFlags UPD_IDENTITY		= 1<<15;
	static const BmUpdFlags UPD_TRACKERNAME	= 1<<16;
	static const BmUpdFlags UPD_CLASSIFICATION= 1<<17;
	static const BmUpdFlags UPD_RATIO_SPAM		= 1<<18;
	static const BmUpdFlags UPD_IMAP_UID		= 1<<19;

							// indicates whether an item has been added or removed
	static const float UNKNOWN_RATIO;

protected:
	BmMailRef( entry_ref &eref, struct stat& st);
	BmMailRef( entry_ref &eref, const node_ref& nref);
	BmMailRef( BMessage* archive, node_ref& nref);
	void Initialize();

private:
	void MarkAsSpamOrTofu(bool asSpam);

	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	node_ref mNodeRef;
	BmString mImapUID;
	BmString mAccount;
	BmString mCc;
	BmString mFrom;
	BmString mName;
	BmString mPriority;
	BmString mReplyTo;
	BmString mStatus;
	BmString mSubject;
	BmString mTo;
	time_t mWhen;
	bigtime_t mWhenCreated;
							// time (in microseconds) when mail has been received
	off_t mSize;
	BmString mSizeString;
	bool mHasAttachments;
	BmString mIdentity;
	BmString mClassification;				// spam or genuine
	float mRatioSpam;							// 0.00 (genuine) .. 1.0 (spam)
	BmString mRatioSpamString;

	// the following members will not be archived at all:
	status_t mInitCheck;

	// Hide copy-constructor and assignment:
	BmMailRef( const BmMailRef&);
	BmMailRef operator=( const BmMailRef&);
};

typedef vector< BmRef<BmMailRef> > BmMailRefVect;

#endif
