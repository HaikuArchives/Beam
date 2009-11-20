/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <ctype.h>

#include <File.h>
#include <NodeMonitor.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailFolderList.h"
#include "BmMailMonitor.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmRoster.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

static BmString BM_REFKEYSTAT( const struct stat& x) 
{
	return BmString() << x.st_ino;
}

// archival-fieldnames:
const char* const BmMailRef::MSG_ACCOUNT = 	"bm:ac";
const char* const BmMailRef::MSG_ATTACHMENTS= "bm:at";
const char* const BmMailRef::MSG_CC = 			"bm:cc";
const char* const BmMailRef::MSG_ENTRYREF = 	"bm:er";
const char* const BmMailRef::MSG_FROM = 		"bm:fr";
const char* const BmMailRef::MSG_INODE = 		"bm:in";
const char* const BmMailRef::MSG_NAME = 		"bm:nm";
const char* const BmMailRef::MSG_PRIORITY = 	"bm:pr";
const char* const BmMailRef::MSG_WHEN_CREATED = "bm:cr";
const char* const BmMailRef::MSG_REPLYTO = 	"bm:rp";
const char* const BmMailRef::MSG_SIZE = 		"bm:sz";
const char* const BmMailRef::MSG_STATUS = 	"bm:st";
const char* const BmMailRef::MSG_SUBJECT = 	"bm:su";
const char* const BmMailRef::MSG_TO = 			"bm:to";
const char* const BmMailRef::MSG_WHEN = 		"bm:wh";
const char* const BmMailRef::MSG_IDENTITY = 	"bm:id";
const char* const BmMailRef::MSG_IS_VALID = 	"bm:iv";
const char* const BmMailRef::MSG_CLASSIFICATION = 	"bm:cl";
const char* const BmMailRef::MSG_RATIO_SPAM= "bm:rs";
const char* const BmMailRef::MSG_IMAP_UID =	"bm:ui";
const int16 BmMailRef::nArchiveVersion = 6;

const float BmMailRef::UNKNOWN_RATIO = 10.0;
	// just anything outside of [0..1]

/*------------------------------------------------------------------------------*\
	CreateInstance( )
		-	static creator-func
		-	N.B.: In here, we lock the GlobalLocker manually (*not* BmAutolock),
			because otherwise we may risk deadlocks
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( entry_ref &eref, 
												  		  struct stat* st) {
	BmString key;
	node_ref nref;
	if (st)
		key = BM_REFKEYSTAT( *st);
	else {
		BEntry entry(&eref);
		if (entry.GetNodeRef(&nref) != B_OK)
			return NULL;
		key = BM_REFKEY(nref);
	}
	GlobalLocker()->Lock();
	if (!GlobalLocker()->IsLocked()) {
		BM_SHOWERR("BmMailRef::CreateInstance(): Could not acquire global lock!");
		return NULL;
	}
	BmRef<BmMailRef> mailRef( 
		dynamic_cast<BmMailRef*>( 
			BmRefObj::FetchObject( typeid(BmMailRef).name(), key)
		)
	);
	GlobalLocker()->Unlock();
	if (mailRef) {
		mailRef->ResyncFromDisk( &eref, st);
		return mailRef;
	} else {
		if (st)
			mailRef = new BmMailRef( eref, *st);
		else
			mailRef = new BmMailRef( eref, nref);
		mailRef->Initialize();
		return mailRef;
	}
}

/*------------------------------------------------------------------------------*\
	CreateInstance( )
		-	static creator-func
		-	N.B.: In here, we lock the GlobalLocker manually (*not* BmAutolock),
			because otherwise we may risk deadlocks
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( BMessage* archive) {
	status_t err;
	node_ref nref;
	if ((err = archive->FindInt64( MSG_INODE, &nref.node)) != B_OK) {
		BM_LOGERR( BmString("BmMailRef: Could not find msg-field ") 
							<< MSG_INODE << "\n\nError:" << strerror(err));
		return NULL;
	}
	nref.device = ThePrefs->MailboxVolume.Device();
	BmString key( BM_REFKEY( nref));
	GlobalLocker()->Lock();
	if (!GlobalLocker()->IsLocked()) {
		BM_SHOWERR("BmMailRef::CreateInstance(): Could not acquire global lock!");
		return NULL;
	}
	BmRef<BmMailRef> mailRef( 
		dynamic_cast<BmMailRef*>( 
			BmRefObj::FetchObject( typeid(BmMailRef).name(), key)
		)
	);
	GlobalLocker()->Unlock();
	if (mailRef)
		return mailRef;
	else {
		mailRef = new BmMailRef( archive, nref);
		mailRef->Initialize();
		return mailRef;
	}
}

/*------------------------------------------------------------------------------*\
	BmMailRef( eref, nref)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( entry_ref &eref, const node_ref& nref)
	:	inherited( BM_REFKEY(nref), NULL, (BmListModelItem*)NULL)
	,	mEntryRef( eref)
	,	mWhen( 0)
	,	mWhenCreated( 0)
	,	mSize( 0)
	,	mHasAttachments( false)
	,	mRatioSpam( UNKNOWN_RATIO)
	,	mInitCheck( B_NO_INIT)
{
	mNodeRef = nref;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( eref, st)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( entry_ref &eref, struct stat& st)
	:	inherited( BM_REFKEYSTAT(st), NULL, (BmListModelItem*)NULL)
	,	mEntryRef( eref)
	,	mWhen( 0)
	,	mWhenCreated( 0)
	,	mSize( 0)
	,	mHasAttachments( false)
	,	mRatioSpam( UNKNOWN_RATIO)
	,	mInitCheck( B_NO_INIT)
{
	mNodeRef.device = st.st_dev;
	mNodeRef.node = st.st_ino;
}

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BMessage* archive, node_ref& nref)
	:	inherited( "", NULL, (BmListModelItem*)NULL)
	,	mWhen( 0)
	,	mWhenCreated( 0)
	,	mSize( 0)
	,	mHasAttachments( false)
	,	mNodeRef( nref)
	,	mRatioSpam( UNKNOWN_RATIO)
	,	mInitCheck( B_NO_INIT)
{
	try {
		status_t err;
		if ((err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( BmString("BmMailRef: Could not find msg-field ") 
										<< MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mEntryRef.device = nref.device;
		Key( BM_REFKEY( mNodeRef));

		int16 version = 0;
		archive->FindInt16( MSG_VERSION, &version);

		mAccount = FindMsgString( archive, MSG_ACCOUNT);
		mHasAttachments = FindMsgBool( archive, MSG_ATTACHMENTS);
		mCc = FindMsgString( archive, MSG_CC);
		mFrom = FindMsgString( archive, MSG_FROM);
		mName = FindMsgString( archive, MSG_NAME);
		mPriority = FindMsgString( archive, MSG_PRIORITY);
		mReplyTo = FindMsgString( archive, MSG_REPLYTO);
		mSize = FindMsgInt64( archive, MSG_SIZE);
		mStatus = FindMsgString( archive, MSG_STATUS);
		mSubject = FindMsgString( archive, MSG_SUBJECT);
		mTo = FindMsgString( archive, MSG_TO);
		mWhen = FindMsgInt32( archive, MSG_WHEN);

		if (version >= 2)
			mIdentity = FindMsgString( archive, MSG_IDENTITY);

		if (version >= 3)
			mWhenCreated = FindMsgInt64( archive, MSG_WHEN_CREATED);
		else
			mWhenCreated = static_cast<int64>(
				FindMsgInt32( archive, MSG_WHEN_CREATED)
			)*(1000*1000);

		if (version >= 4)
			mIsValid = FindMsgBool( archive, MSG_IS_VALID);

		if (version >= 5) {
			mClassification = FindMsgString( archive, MSG_CLASSIFICATION);
			mRatioSpam = FindMsgFloat( archive, MSG_RATIO_SPAM);
		}

		if (version >= 6) {
			mImapUID = FindMsgString( archive, MSG_IMAP_UID);
		}

		mSizeString = BytesToString( mSize,true);
		if (mRatioSpam != UNKNOWN_RATIO)
			mRatioSpamString << mRatioSpam;

		mInitCheck = B_OK;
	} catch (BM_error &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailRef()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailRef::~BmMailRef() {
	BM_LOG3( BM_LogMailTracking, 
				BmString("destructor of MailRef ") << Key() << " called");
	WatchNode( &mNodeRef, B_STOP_WATCHING, TheMailMonitor);
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRef::Archive( BMessage* archive, bool) const {
	status_t ret 
		= archive->AddInt16( MSG_VERSION, nArchiveVersion)
		|| archive->AddBool( MSG_IS_VALID, mIsValid)
		|| archive->AddString( MSG_ACCOUNT, mAccount.String())
		|| archive->AddBool( MSG_ATTACHMENTS, mHasAttachments)
		|| archive->AddString( MSG_CC, mCc.String())
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddString( MSG_FROM, mFrom.String())
		|| archive->AddInt64( MSG_INODE, mNodeRef.node)
		|| archive->AddString( MSG_NAME, mName.String())
		|| archive->AddString( MSG_PRIORITY, mPriority.String())
		|| archive->AddInt64( MSG_WHEN_CREATED, mWhenCreated)
		|| archive->AddString( MSG_REPLYTO, mReplyTo.String())
		|| archive->AddInt64( MSG_SIZE, mSize)
		|| archive->AddString( MSG_STATUS, mStatus.String())
		|| archive->AddString( MSG_SUBJECT, mSubject.String())
		|| archive->AddString( MSG_TO, mTo.String())
		|| archive->AddString( MSG_IDENTITY, mIdentity.String())
		|| archive->AddInt32( MSG_WHEN, mWhen)
		|| archive->AddString( MSG_CLASSIFICATION, mClassification.String())
		|| archive->AddFloat( MSG_RATIO_SPAM, mRatioSpam)
		|| archive->AddString( MSG_IMAP_UID, mImapUID.String());
	return ret;
}

/*------------------------------------------------------------------------------*\
	Initialize()
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
void BmMailRef::Initialize() {
	WatchNode( &mNodeRef, B_WATCH_STAT | B_WATCH_ATTR, TheMailMonitor);
	if (mInitCheck != B_OK) {
		if (ReadAttributes())
			mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	ReadAttributes()
		-	reads attribute-data from mail-file
\*------------------------------------------------------------------------------*/
bool BmMailRef::ReadAttributes( const struct stat* statInfo, 
										  BmUpdFlags* updFlagsOut) {
	status_t err;
	BNode node;
	BmString filetype;
	struct stat st;
	BmUpdFlags updFlags = 0;

	for( int i=0; (err = node.SetTo( &mEntryRef)) == B_BUSY; ++i) {
		if (i==200)
			break;
		BM_LOG2( BM_LogMailTracking, 
					BmString("Node is locked for mail-ref <") << mEntryRef.name 
						<< ">. We take a nap and try again...");
		snooze( 10*1000);					// pause for 10ms
	}
	if (err != B_OK) {
		BM_LOG2(
			BM_LogMailTracking, 
			BmString("Could not get node for mail-ref <") 
				<< mEntryRef.name << "> \n\nError:" << strerror(err)
		);
	}
	if (err == B_OK) {
		if (statInfo)
			st = *statInfo;
		else {
			if ((err = node.GetStat( &st)) != B_OK)
				BM_LOGERR(
					BmString("Could not get stat-info for mail-ref <") 
						<< mEntryRef.name << "> \n\nError:" << strerror(err)
				);
		}
	}

	BmReadStringAttr( &node, "BEOS:TYPE", filetype);
	if (err == B_OK && BeamRoster->IsSupportedEmailMimeType( filetype)) {
		// file is indeed a mail, we fetch its attributes:
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_NAME, 	mName))
			updFlags |= UPD_NAME;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_IMAP_UID, mImapUID))
			updFlags |= UPD_IMAP_UID;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_ACCOUNT, mAccount))
			updFlags |= UPD_ACCOUNT;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_CC, 		mCc))
			updFlags |= UPD_CC;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_FROM, 	mFrom))
			updFlags |= UPD_FROM;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_REPLY, 	mReplyTo))
			updFlags |= UPD_REPLYTO;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_STATUS, 	mStatus))
			updFlags |= UPD_STATUS;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_SUBJECT, mSubject))
			updFlags |= UPD_SUBJECT;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_TO, 		mTo))
			updFlags |= UPD_TO;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_IDENTITY, mIdentity))
			updFlags |= UPD_IDENTITY;
		if (BmReadStringAttr( &node, BM_MAIL_ATTR_CLASSIFICATION, mClassification))
			updFlags |= UPD_CLASSIFICATION;
		BmString priority;
		BmReadStringAttr( &node, BM_MAIL_ATTR_PRIORITY, priority);

		time_t when;
		node.ReadAttr( BM_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, 
							&when, sizeof(time_t));
		if (when != mWhen) {
			mWhen = when;
			updFlags |= UPD_WHEN;
		}

		float ratio = UNKNOWN_RATIO;
		ssize_t sz = node.ReadAttr( BM_MAIL_ATTR_RATIO_SPAM, B_FLOAT_TYPE, 0, 
											 &ratio, sizeof(float));
		if (sz != sizeof(float)) {
			RatioSpam(UNKNOWN_RATIO);
			updFlags |= UPD_RATIO_SPAM;
		} else {
			if (ratio != mRatioSpam) {
				RatioSpam(ratio);
				updFlags |= UPD_RATIO_SPAM;
			}
		}

		int32 att1 = 0;
						// standard BeOS kind (BMail, Postmaster, Beam)
		node.ReadAttr( BM_MAIL_ATTR_ATTACHMENTS, B_INT32_TYPE, 0, 
							&att1, sizeof(att1));
		bool att2 = false;
						// Scooby kind
		node.ReadAttr( "MAIL:attachment", B_BOOL_TYPE, 0, &att2, sizeof(att2));
		if (mHasAttachments != (att1>0 || att2)) {
			mHasAttachments = (att1>0 || att2);
							// please notice that we ignore Mail-It, since
							// it does not give any proper indication 
							// (other than its internal status-attribute,
							// which we really do not want to look at...)
			updFlags |= UPD_ATTACHMENTS;
		}

		if (mSize != st.st_size) {
			mSize = st.st_size;
			mSizeString = BytesToString( mSize,true);
			updFlags |= UPD_SIZE;
		}

		bigtime_t whenCreated;
		if (node.ReadAttr( BM_MAIL_ATTR_WHEN_CREATED, B_UINT64_TYPE, 0, 
								 &whenCreated, sizeof(bigtime_t)) < 0) {
			// corresponding attribute doesn't exist, we fetch it from the
			// file's modification time (which is just time_t instead of bigtime_t):
			whenCreated = static_cast<int64>(	st.st_mtime)*(1000*1000);
		}
		if (whenCreated != mWhenCreated) {
			mWhenCreated = whenCreated;
			updFlags |= UPD_WHEN_CREATED;
		}

		// simplify priority:
		if (!priority.Length()) {
			priority = "3";				// normal priority
		} else {
			if (isdigit(priority[0]))
				priority.Truncate(1);
			else {
				if (priority.IFindFirst("Highest") != B_ERROR)
					priority = "1";
				else if (priority.IFindFirst("High") != B_ERROR)
					priority = "2";
				else if (priority.IFindFirst("Lowest") != B_ERROR)
					priority = "5";
				else if (priority.IFindFirst("Low") != B_ERROR)
					priority = "4";
				else
					priority = "3";
			}
		}
		if (priority != mPriority) {
			mPriority = priority;
			updFlags |= UPD_PRIORITY;
		}
			
		IsValid( true);
	} else {
		// item is no mail, we mark it as invalid:
		mName = "";
		mImapUID = "";
		mAccount = "";
		mCc = "";
		mFrom = "";
		mPriority = "";
		mReplyTo = "";
		mStatus = "";
		mSubject = "";
		mTo = "";
		mIdentity = "";
		mWhen = 0;
		mWhenCreated = 0;
		mHasAttachments = false;
		mSize = 0;
		mSizeString = "";
		mPriority = "";
		mClassification = "";
		mRatioSpam = UNKNOWN_RATIO;
		mRatioSpamString = "";

		BM_LOG2( BM_LogMailTracking, 
					BmString("file <") << mEntryRef.name 
						<< " is not a mail, invalidating it.");
		IsValid( false);
	}
	if (updFlagsOut)
		*updFlagsOut = updFlags;
	return err == B_OK;
}

/*------------------------------------------------------------------------------*\
	ResyncFromDisk()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::ResyncFromDisk( entry_ref* newRef, 
										  const struct stat* statInfo) {
	BmUpdFlags updFlags = 0;
	if (newRef) {
		if (strcmp( mEntryRef.name, newRef->name) != 0)
			updFlags |= UPD_TRACKERNAME;
		mEntryRef = *newRef;
	}
	if (ReadAttributes( statInfo, &updFlags))
		mInitCheck = B_OK;
	if (updFlags)
		// update only if anything has changed:
		TellModelItemUpdated( updFlags);
	BmRef<BmListModel> listModel( ListModel());
	BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
	if (refList)
		refList->MarkAsChanged();
}

/*------------------------------------------------------------------------------*\
	IsSpecial()
		-	
\*------------------------------------------------------------------------------*/
const bool BmMailRef::IsSpecial() const {
	return mStatus == BM_MAIL_STATUS_NEW || mStatus == BM_MAIL_STATUS_PENDING;
}

/*------------------------------------------------------------------------------*\
	MarkAs()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAs( const char* status) {
	if (InitCheck() != B_OK || mStatus == status)
		return;
	try {
		BNode mailNode;
		status_t err;
		mStatus = status;
		if ((err = mailNode.SetTo( &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString( "Could not create node for current mail-file.\n\n"
							 " Result: ") 
					<< strerror(err)
			);
		// in order to allow proper handling of B_ATTR_CHANGED events, we
		// tell the MailMonitor, which folder this mail-ref lives in:
		node_ref folderNodeRef;
		folderNodeRef.node = mEntryRef.directory;
		folderNodeRef.device = mEntryRef.device;
		BmString folderKey( BM_REFKEY( folderNodeRef));
		TheMailMonitor->CacheRefToFolder( mNodeRef, folderKey);
		// As there seems to be a problem with multiple Status-attributes
		// (perhaps a bug in BFS?) we try to remove the attribute before we
		// write it. Let's see if that helps...
		mailNode.RemoveAttr( BM_MAIL_ATTR_STATUS);
		mailNode.WriteAttr( BM_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, 
								  status, strlen( status)+1);
		TellModelItemUpdated( UPD_STATUS);
		BmRef<BmListModel> listModel( ListModel());
		BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
		if (refList)
			refList->MarkAsChanged();
	} catch( BM_error &e) {
		BM_SHOWERR(e.what());
	}
}

/*------------------------------------------------------------------------------*\
	RatioSpam()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::RatioSpam(float rs) {
	mRatioSpam = rs;
	mRatioSpamString = "";
	if (mRatioSpam != UNKNOWN_RATIO)
		mRatioSpamString << mRatioSpam;
}

/*------------------------------------------------------------------------------*\
	MarkAsSpam()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAsSpam() {
	MarkAsSpamOrTofu(true);
}

/*------------------------------------------------------------------------------*\
	MarkAsSpam()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAsTofu() {
	MarkAsSpamOrTofu(false);
}

/*------------------------------------------------------------------------------*\
	MarkAsSpamOrTofu()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::MarkAsSpamOrTofu( bool asSpam) {
	if (InitCheck() != B_OK)
		return;
	try {
		BNode mailNode;
		status_t err;
		mClassification = asSpam ? BM_MAIL_CLASS_SPAM : BM_MAIL_CLASS_TOFU;
		if ((err = mailNode.SetTo( &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( 
				BmString( "Could not create node for current mail-file.\n\n"
							 " Result: ") 
					<< strerror(err)
			);
		// in order to allow proper handling of B_ATTR_CHANGED events, we
		// tell the MailMonitor, which folder this mail-ref lives in:
		node_ref folderNodeRef;
		folderNodeRef.node = mEntryRef.directory;
		folderNodeRef.device = mEntryRef.device;
		BmString folderKey( BM_REFKEY( folderNodeRef));
		TheMailMonitor->CacheRefToFolder( mNodeRef, folderKey);
		// As there seems to be a problem with multiple attributes
		// (perhaps a bug in BFS?) we try to remove the attribute before we
		// write it. Let's see if that helps...
		mailNode.RemoveAttr( BM_MAIL_ATTR_CLASSIFICATION);
		mailNode.WriteAttr( BM_MAIL_ATTR_CLASSIFICATION, B_STRING_TYPE, 0, 
								  mClassification.String(), mClassification.Length()+1);
		TellModelItemUpdated( UPD_CLASSIFICATION);
		BmRef<BmListModel> listModel( ListModel());
		BmMailRefList* refList = dynamic_cast< BmMailRefList*>( listModel.Get());
		if (refList)
			refList->MarkAsChanged();
	} catch( BM_error &e) {
		BM_SHOWERR(e.what());
	}
}
