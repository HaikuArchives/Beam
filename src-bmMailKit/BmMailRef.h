/*
	BmMailRef.h
		$Id$
*/

#ifndef _BmMailRef_h
#define _BmMailRef_h

#include <map>


#include <Archivable.h>
#include <Entry.h>
#include <Node.h>
#include <String.h>

#include "BmDataModel.h"

/*------------------------------------------------------------------------------*\
	BmMailRef
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailRef : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	//
	static BmMailRef* CreateInstance( entry_ref &eref, ino_t node, struct stat& st);
	BmMailRef( entry_ref &eref, ino_t node, struct stat& st);
	BmMailRef( BMessage* archive);
	virtual ~BmMailRef();
	
	// archival stuff:
	static BArchivable* Instantiate( BMessage* archive);
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const entry_ref& EntryRef() const 		{ return mEntryRef; }
	const entry_ref* EntryRefPtr() const	{ return &mEntryRef; }
	const BString Name() const					{ return mEntryRef.name; }
	const ino_t& Inode() const					{ return mInode; }
	status_t InitCheck()							{ return mInitCheck; }
	const BString& Account() const 			{ return mAccount; }
	const BString& Cc() const 					{ return mCc; }
	const BString& From() const 				{ return mFrom; }
	const BString& Priority() const 			{ return mPriority; }
	const BString& ReplyTo() const 			{ return mReplyTo; }
	const BString& Status() const 			{ return mStatus; }
	const BString& Subject() const 			{ return mSubject; }
	const BString& To() const 					{ return mTo; }
	const time_t& When() const 				{ return mWhen; }
	const BString& WhenString() const 		{ return mWhenString; }
	const time_t& Created() const 			{ return mCreated; }
	const BString& CreatedString() const 	{ return mCreatedString; }
	const off_t& Size() const 					{ return mSize; }
	const bool& HasAttachments() const 		{ return mHasAttachments; }

	// setters:
	void EntryRef( entry_ref &e) 				{ mEntryRef = e; }

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF = 	"bm:eref";
	static const char* const MSG_INODE = 		"bm:inod";

	//	message component definitions for status-msgs:
	static const char* const MSG_ACCOUNT = 	"bm:acc";
	static const char* const MSG_CC = 			"bm:cc";
	static const char* const MSG_CREATED = 	"bm:creat";
	static const char* const MSG_FROM = 		"bm:from";
	static const char* const MSG_HASATTACHMENTS = 	"bm:att";
	static const char* const MSG_NAME = 		"bm:name";
	static const char* const MSG_PRIORITY = 	"bm:prior";
	static const char* const MSG_REPLYTO = 	"bm:reply";
	static const char* const MSG_SIZE = 		"bm:size";
	static const char* const MSG_STATUS = 		"bm:status";
	static const char* const MSG_SUBJECT = 	"bm:subject";
	static const char* const MSG_TO = 			"bm:to";
	static const char* const MSG_WHEN = 		"bm:when";

private:
	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	ino_t mInode;

	BString mAccount;
	BString mCc;
	BString mFrom;
	BString mName;
	BString mPriority;
	BString mReplyTo;
	BString mStatus;
	BString mSubject;
	BString mTo;
	time_t mWhen;
	BString mWhenString;
	time_t mCreated;
	BString mCreatedString;
	off_t mSize;
	bool mHasAttachments;

	// the following members will not be archived at all:
	status_t mInitCheck;
};

#endif
