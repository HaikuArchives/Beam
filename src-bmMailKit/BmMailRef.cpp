/*
	BmMailRef.cpp
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


#include <ctype.h>

#include <File.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

#define BM_REFKEY(x) (BmString() << x.st_ino)

	// archival-fieldnames:
const char* const BmMailRef::MSG_ACCOUNT = 	"bm:ac";
const char* const BmMailRef::MSG_ATTACHMENTS= "bm:at";
const char* const BmMailRef::MSG_CC = 			"bm:cc";
const char* const BmMailRef::MSG_CREATED = 	"bm:cr";
const char* const BmMailRef::MSG_ENTRYREF = 	"bm:er";
const char* const BmMailRef::MSG_FROM = 		"bm:fr";
const char* const BmMailRef::MSG_INODE = 		"bm:in";
const char* const BmMailRef::MSG_NAME = 		"bm:nm";
const char* const BmMailRef::MSG_PRIORITY = 	"bm:pr";
const char* const BmMailRef::MSG_REPLYTO = 	"bm:rp";
const char* const BmMailRef::MSG_SIZE = 		"bm:sz";
const char* const BmMailRef::MSG_STATUS = 	"bm:st";
const char* const BmMailRef::MSG_SUBJECT = 	"bm:su";
const char* const BmMailRef::MSG_TO = 			"bm:to";
const char* const BmMailRef::MSG_WHEN = 		"bm:wh";
const int16 BmMailRef::nArchiveVersion = 1;

/*------------------------------------------------------------------------------*\
	CreateInstance( )
		-	static creator-func
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( BmMailRefList* model, entry_ref &eref, 
												  		  struct stat& st) {
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMailRef).name());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		BmString key( BM_REFKEY( st));
		BmRef<BmMailRef> mailRef( dynamic_cast<BmMailRef*>( proxy->FetchObject( key)));
		if (mailRef) {
			// update mailref with new info:
			if (model) {
				// only update list if we really have one (don't clobber existing list
				// with NULL value):
				mailRef->mListModel = model;
			}
			mailRef->EntryRef( eref);
			mailRef->ResyncFromDisk();
			return mailRef;
		}
		BmMailRef* newRef = new BmMailRef( model, eref, st);
		status_t ret;
		if ((ret = newRef->InitCheck()) != B_OK) {
			// item is not of mimetype email, we skip it:
			delete newRef;
			return NULL;
		} else {
			return newRef;
		}
	}
	BM_THROW_RUNTIME( BmString("Could not get proxy for ") << typeid(BmMailRef).name());
	return NULL;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BmMailRefList* model, entry_ref &eref, struct stat& st)
	:	inherited( BM_REFKEY(st), model, (BmListModelItem*)NULL)
	,	mEntryRef( eref)
	,	mInode( st.st_ino)
	,	mInitCheck( B_NO_INIT)
{
	if (ReadAttributes( &st))
		mInitCheck = B_OK;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BMessage* archive, BmMailRefList* model)
	:	inherited( "", model, (BmListModelItem*)NULL)
	,	mInode( 0)
	,	mInitCheck( B_NO_INIT)
{
	try {
		status_t err;
		(err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BmString("BmMailRef: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		Key( BmString() << mInode);

		mAccount = FindMsgString( archive, MSG_ACCOUNT);
		mHasAttachments = FindMsgBool( archive, MSG_ATTACHMENTS);
		mCc = FindMsgString( archive, MSG_CC);
		mCreated = FindMsgInt32( archive, MSG_CREATED);
		mFrom = FindMsgString( archive, MSG_FROM);
		mName = FindMsgString( archive, MSG_NAME);
		mPriority = FindMsgString( archive, MSG_PRIORITY);
		mReplyTo = FindMsgString( archive, MSG_REPLYTO);
		mSize = FindMsgInt64( archive, MSG_SIZE);
		mStatus = FindMsgString( archive, MSG_STATUS);
		mSubject = FindMsgString( archive, MSG_SUBJECT);
		mTo = FindMsgString( archive, MSG_TO);
		mWhen = FindMsgInt32( archive, MSG_WHEN);

		mSizeString = BytesToString( mSize,true);
		mCreatedString = ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
								? TimeToSwatchString( mCreated)
								: TimeToString( mCreated);
		mWhenString = 	ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
								? TimeToSwatchString( mWhen)
								: TimeToString( mWhen);

		mInitCheck = B_OK;
	} catch (exception &e) {
		BM_LOGERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	BmMailRef( mailref)
		-	copy c'tor
\*------------------------------------------------------------------------------*/
/*
BmMailRef::BmMailRef( const BmMailRef& mailRef)
	:	inherited( mailRef)
	,	mEntryRef( mailRef.EntryRef())
	,	mInode( mailRef.Inode())
	,	mAccount( mailRef.Account())
	,	mCc( mailRef.Cc())
	,	mFrom( mailRef.From())
	,	mName( mailRef.Name())
	,	mPriority( mailRef.Priority())
	,	mReplyTo( mailRef.ReplyTo())
	,	mStatus( mailRef.Status())
	,	mSubject( mailRef.Subject())
	,	mTo( mailRef.To())
	,	mWhen( mailRef.When())
	,	mWhenString( mailRef.WhenString())
	,	mCreated( mailRef.Created())
	,	mCreatedString( mailRef.CreatedString())
	,	mSize( mailRef.Size())
	,	mSizeString( mailRef.SizeString())
	,	mHasAttachments( mailRef.HasAttachments())
	,	mInitCheck( mailRef.InitCheck())
{
}
*/

/*------------------------------------------------------------------------------*\
	~BmMailRef()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailRef::~BmMailRef() {
	BM_LOG3( BM_LogMailTracking, BmString("destructor of MailRef ") << Key() << " called");
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRef::Archive( BMessage* archive, bool) const {
	status_t ret 
		=  archive->AddString( MSG_ACCOUNT, mAccount.String())
		|| archive->AddBool( MSG_ATTACHMENTS, mHasAttachments)
		|| archive->AddString( MSG_CC, mCc.String())
		|| archive->AddInt32( MSG_CREATED, mCreated)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddString( MSG_FROM, mFrom.String())
		|| archive->AddInt64( MSG_INODE, mInode)
		|| archive->AddString( MSG_NAME, mName.String())
		|| archive->AddString( MSG_PRIORITY, mPriority.String())
		|| archive->AddString( MSG_REPLYTO, mReplyTo.String())
		|| archive->AddInt64( MSG_SIZE, mSize)
		|| archive->AddString( MSG_STATUS, mStatus.String())
		|| archive->AddString( MSG_SUBJECT, mSubject.String())
		|| archive->AddString( MSG_TO, mTo.String())
		|| archive->AddInt32( MSG_WHEN, mWhen);
	return ret;
}

/*------------------------------------------------------------------------------*\
	ReadAttributes()
		-	reads attribute-data from mail-file
\*------------------------------------------------------------------------------*/
bool BmMailRef::ReadAttributes( const struct stat* statInfo) {
	status_t err;
	BNode node;
	BmString filetype;
	bool retval = false;
	struct stat st;
	
	if (statInfo)
		st = *statInfo;

	try {
		for( int i=0; (err = node.SetTo( &mEntryRef)) == B_BUSY; ++i) {
			if (i==200)
				throw BM_runtime_error( BmString("Node is locked too long for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));
			BM_LOG2( BM_LogMailTracking, BmString("Node is locked for mail-file <") << mEntryRef.name << ">. We take a nap and try again...");
			snooze( 10*1000);					// pause for 10ms
		}
		if (err != B_OK)
			throw BM_runtime_error(BmString("Could not get node for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));
		if (!statInfo) {
			if ((err = node.GetStat( &st)) != B_OK)
				throw BM_runtime_error(BmString("Could not get stat-info for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));
		}

		BmReadStringAttr( &node, "BEOS:TYPE", filetype);
		if (!filetype.ICompare("text/x-email") 
		|| !filetype.ICompare("message/rfc822") ) {
			// file is indeed a mail, we fetch its attributes:
			BmReadStringAttr( &node, BM_MAIL_ATTR_NAME, 		mName);
			BmReadStringAttr( &node, BM_MAIL_ATTR_ACCOUNT, 	mAccount);
			BmReadStringAttr( &node, BM_MAIL_ATTR_CC, 		mCc);
			BmReadStringAttr( &node, BM_MAIL_ATTR_FROM, 		mFrom);
			BmReadStringAttr( &node, BM_MAIL_ATTR_PRIORITY, mPriority);
			BmReadStringAttr( &node, BM_MAIL_ATTR_REPLY, 	mReplyTo);
			BmReadStringAttr( &node, BM_MAIL_ATTR_STATUS, 	mStatus);
			BmReadStringAttr( &node, BM_MAIL_ATTR_SUBJECT, 	mSubject);
			BmReadStringAttr( &node, BM_MAIL_ATTR_TO, 		mTo);
	
			mWhen = 0;
			node.ReadAttr( BM_MAIL_ATTR_WHEN, 		B_TIME_TYPE, 0, &mWhen, sizeof(time_t));
			mWhenString = 	ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
									? TimeToSwatchString( mWhen)
									: TimeToString( mWhen);
	
			int32 att1 = 0;					// standard BeOS kind (BMail, Postmaster, Beam)
			node.ReadAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, &att1, sizeof(att1));
			bool att2 = false;				// Scooby kind
			node.ReadAttr( "MAIL:attachment", B_BOOL_TYPE, 0, &att2, sizeof(att2));
			mHasAttachments = att1>0 || att2;
													// please notice that we ignore Mail-It, since
													// it does not give any proper indication 
													// (other than its internal status-attribute,
													// which we really do not want to even look at)
	
			mSize = st.st_size;
			mSizeString = BytesToString( mSize,true);
			mCreated = st.st_ctime;
			mCreatedString = ThePrefs->GetBool( "UseSwatchTimeInRefView", false)
									? TimeToSwatchString( mCreated)
									: TimeToString( mCreated);

			// simplify priority:
			if (!mPriority.Length()) {
				mPriority = "3";				// normal priority
			} else {
				if (isdigit(mPriority[0]))
					mPriority.Truncate(1);
				else {
					if (mPriority.FindFirst("Highest") != B_ERROR)
						mPriority = "1";
					else if (mPriority.FindFirst("High") != B_ERROR)
						mPriority = "2";
					else if (mPriority.FindFirst("Lowest") != B_ERROR)
						mPriority = "5";
					else if (mPriority.FindFirst("Low") != B_ERROR)
						mPriority = "4";
					else
						mPriority = "3";
				}
			}
			retval = true;
		} else 
			BM_LOG2( BM_LogMailTracking, BmString("file <")<<mEntryRef.name<<" is not a mail, ignoring it.");
	} catch( exception &e) {
		BM_LOGERR( e.what());
	}
	return retval;
}

/*------------------------------------------------------------------------------*\
	ResyncFromDisk()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::ResyncFromDisk() { 
	if (ReadAttributes())
		mInitCheck = B_OK;
	TellModelItemUpdated( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	MarkAs()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAs( const char* status) {
	if (InitCheck() != B_OK)
		return;
	try {
		BNode mailNode;
		status_t err;
		mStatus = status;
		(err = mailNode.SetTo( &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Could not create node for current mail-file.\n\n Result: ") << strerror(err));
		mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, status, strlen( status)+1);
		TellModelItemUpdated( UPD_STATUS);
		BmRef<BmListModel> listModel( ListModel());
		BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
		if (refList)
			refList->MarkAsChanged();
	} catch( exception &e) {
		BM_SHOWERR(e.what());
	}
}

#ifdef BM_LOGGING
/*------------------------------------------------------------------------------*\
	ObjectSize()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMailRef::ObjectSize( bool addSizeofThis) const {
	return 	inherited::ObjectSize(false)
		+		(addSizeofThis ? sizeof( *this) : 0)
		+		mAccount.Length()+1
		+		mCc.Length()+1
		+		mFrom.Length()+1
		+		mName.Length()+1
		+		mPriority.Length()+1
		+		mReplyTo.Length()+1
		+		mStatus.Length()+1
		+		mSubject.Length()+1
		+		mTo.Length()+1
		+		mWhenString.Length()+1
		+		mCreatedString.Length()+1
		+		mSizeString.Length()+1;
}
#endif
