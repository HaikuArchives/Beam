/*
	BmMailRef.h
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


#ifndef _BmMailRef_h
#define _BmMailRef_h

#include <map>


#include <Entry.h>
#include <String.h>

#include "BmDataModel.h"

class BmMail;
class BmMailRefList;
/*------------------------------------------------------------------------------*\
	BmMailRef
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailRef : public BmListModelItem {
	typedef BmListModelItem inherited;

	// archival-fieldnames:
	static const char* const MSG_ACCOUNT = 	"bm:ac";
	static const char* const MSG_ATTACHMENTS= "bm:at";
	static const char* const MSG_CC = 			"bm:cc";
	static const char* const MSG_CREATED = 	"bm:cr";
	static const char* const MSG_ENTRYREF = 	"bm:er";
	static const char* const MSG_FROM = 		"bm:fr";
	static const char* const MSG_INODE = 		"bm:in";
	static const char* const MSG_NAME = 		"bm:nm";
	static const char* const MSG_PRIORITY = 	"bm:pr";
	static const char* const MSG_REPLYTO = 	"bm:rp";
	static const char* const MSG_SIZE = 		"bm:sz";
	static const char* const MSG_STATUS = 		"bm:st";
	static const char* const MSG_SUBJECT = 	"bm:su";
	static const char* const MSG_TO = 			"bm:to";
	static const char* const MSG_WHEN = 		"bm:wh";
	static const int16 nArchiveVersion = 1;

public:
	// creator-funcs, c'tors and d'tor:
	static BmRef<BmMailRef> CreateInstance( BmMailRefList* model, entry_ref &eref, 
												 		 struct stat& st);
	static BmRef<BmMailRef> CreateDummyInstance( BmMailRefList* model, int id);
	BmMailRef( BMessage* archive, BmMailRefList* model);
	virtual ~BmMailRef();

	// native methods:
	void MarkAs( const char* s);
	bool ReadAttributes( const struct stat* statInfo = NULL);
	void ResyncFromDisk();

	// overrides of archivable base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// getters:
	inline const entry_ref& EntryRef() const 		{ return mEntryRef; }
	inline const entry_ref* EntryRefPtr() const	{ return &mEntryRef; }
	inline const char* TrackerName() const			{ return mEntryRef.name; }
	inline const ino_t& Inode() const				{ return mInode; }
	inline status_t InitCheck()	const				{ return mInitCheck; }
	inline const BString& Account() const 			{ return mAccount; }
	inline const BString& Cc() const 				{ return mCc; }
	inline const BString& From() const 				{ return mFrom; }
	inline const BString& Name() const				{ return mName; }
	inline const BString& Priority() const 		{ return mPriority; }
	inline const BString& ReplyTo() const 			{ return mReplyTo; }
	inline const BString& Status() const 			{ return mStatus; }
	inline const BString& Subject() const 			{ return mSubject; }
	inline const BString& To() const 				{ return mTo; }
	inline const time_t& When() const 				{ return mWhen; }
	inline const BString& WhenString() const 		{ return mWhenString; }
	inline const time_t& Created() const 			{ return mCreated; }
	inline const BString& CreatedString() const 	{ return mCreatedString; }
	inline const off_t& Size() const 				{ return mSize; }
	inline const BString& SizeString() const 		{ return mSizeString; }
	inline const bool& HasAttachments() const 	{ return mHasAttachments; }
	inline const bool IsNew() const					{ return mStatus == "New"; }

	// setters:
	inline void EntryRef( entry_ref &e) 			{ mEntryRef = e; }

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_STATUS	= 1<<1;

protected:
	BmMailRef( BmMailRefList* model, entry_ref &eref, struct stat& st);

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
	BString mSizeString;
	bool mHasAttachments;

	// the following members will not be archived at all:
	status_t mInitCheck;

	// Hide copy-constructor and assignment:
	BmMailRef( const BmMailRef&);
	BmMailRef operator=( const BmMailRef&);
};

#endif
