/*
	BmMailRef.cpp
		$Id$
*/

#include <ctype.h>

#include <File.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMail.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmUtil.h"

#define BM_REFKEY(x) (BString() << x.st_ino)

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRef::CreateInstance( BmMailRefList* model, entry_ref &eref, 
												  		  struct stat& st) {
	BmProxy* proxy = BmRefObj::GetProxy( typeid(BmMailRef).name());
	if (proxy) {
		BAutolock lock( &proxy->Locker);
		BString key( BM_REFKEY( st));
		BmRef<BmMailRef> mailRef( dynamic_cast<BmMailRef*>( proxy->FetchObject( key)));
		if (mailRef)
			return mailRef;
	}
	BmMailRef* mailRef = new BmMailRef( model, eref, st);
	status_t ret;
	if ((ret = mailRef->InitCheck()) != B_OK) {
		// item is not of mimetype email, we skip it:
		delete mailRef;
		return NULL;
	} else {
		return mailRef;
	}
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
													|| BM_THROW_RUNTIME( BString("BmMailRef: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		Key( BString() << mInode);

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

		mCreatedString = TimeToString( mCreated);
		mSizeString = BytesToString( mSize,true);
		mWhenString = TimeToString( mWhen);

		mInitCheck = B_OK;
	} catch (exception &e) {
		BM_SHOWERR( e.what());
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
	BM_LOG3( BM_LogMailTracking, BString("destructor of MailRef ") << Key() << " called");
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRef::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		|| archive->AddString( MSG_ACCOUNT, mAccount)
		|| archive->AddBool( MSG_ATTACHMENTS, mHasAttachments)
		|| archive->AddString( MSG_CC, mCc)
		|| archive->AddInt32( MSG_CREATED, mCreated)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddString( MSG_FROM, mFrom)
		|| archive->AddInt64( MSG_INODE, mInode)
		|| archive->AddString( MSG_NAME, mName)
		|| archive->AddString( MSG_PRIORITY, mPriority)
		|| archive->AddString( MSG_REPLYTO, mReplyTo)
		|| archive->AddInt64( MSG_SIZE, mSize)
		|| archive->AddString( MSG_STATUS, mStatus)
		|| archive->AddString( MSG_SUBJECT, mSubject)
		|| archive->AddString( MSG_TO, mTo)
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
	BString buffer;
	BString filetype;
	char* buf;
	size_t bufsize = 256;
	bool retval = false;
	struct stat st;
	
	if (statInfo)
		st = *statInfo;
	else {
		BEntry entry( &mEntryRef);
		if ((err = entry.GetStat( &st)) != B_OK)
			throw BM_runtime_error(BString("Could not get stat-info for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));
	}

	buffer.SetTo( '\0', bufsize);		// preallocate the bufsize we need
	buf = buffer.LockBuffer( 0);

	try {
		for( int i=0; (err = node.SetTo( &mEntryRef)) == B_BUSY; ++i) {
			if (i==100)
				throw BM_runtime_error( BString("Node is locked too long for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));
			BM_LOG2( BM_LogMailTracking, BString("Node is locked for mail-file <") << mEntryRef.name << ">. We take a nap and try again...");
			snooze( 50*1000);
		}
		if (err != B_OK)
			throw BM_runtime_error(BString("Could not get node for mail-file <") << mEntryRef.name << "> \n\nError:" << strerror(err));

		node.ReadAttr( "BEOS:TYPE", 		B_STRING_TYPE, 0, buf, bufsize);		filetype = buf; 	*buf=0;
		if (filetype.ICompare("text/x-email") != 0 
		&& filetype.ICompare("message/rfc822") != 0) {
			// maybe this file has just been created and the mimetype has not yet been
			// set, we give the creator some more time...
			snooze(100*1000);
			node.ReadAttr( "BEOS:TYPE", 	B_STRING_TYPE, 0, buf, bufsize);		filetype = buf; 	*buf=0;
		}
		if (!filetype.ICompare("text/x-email") 
		|| !filetype.ICompare("message/rfc822") ) {
			// file is indeed a mail, we fetch its attributes:
			node.ReadAttrString( BM_MAIL_ATTR_NAME, 		&mName);
			node.ReadAttrString( BM_MAIL_ATTR_ACCOUNT, 	&mAccount);
			node.ReadAttrString( BM_MAIL_ATTR_CC, 			&mCc);
			node.ReadAttrString( BM_MAIL_ATTR_FROM, 		&mFrom);
			node.ReadAttrString( BM_MAIL_ATTR_PRIORITY, 	&mPriority);
			node.ReadAttrString( BM_MAIL_ATTR_REPLY, 		&mReplyTo);
			node.ReadAttrString( BM_MAIL_ATTR_STATUS, 	&mStatus);
			node.ReadAttrString( BM_MAIL_ATTR_SUBJECT, 	&mSubject);
			node.ReadAttrString( BM_MAIL_ATTR_TO, 			&mTo);
	
			mWhen = 0;
			node.ReadAttr( BM_MAIL_ATTR_WHEN, 		B_TIME_TYPE, 0, &mWhen, sizeof(time_t));
			mWhenString = TimeToString( mWhen);
	
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
			mCreatedString = TimeToString( mCreated);

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
			BM_LOG2( BM_LogMailTracking, BString("file <")<<mEntryRef.name<<" is not a mail, ignoring it.");
		buffer.UnlockBuffer( -1);
	} catch( exception &e) {
		buffer.UnlockBuffer( -1);
		BM_SHOWERR( e.what());
	}
	return retval;
}

/*------------------------------------------------------------------------------*\
	ResyncFromDisk()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::ResyncFromDisk() { 
	ReadAttributes();
	TellModelItemUpdated( UPD_ALL);
}

/*------------------------------------------------------------------------------*\
	Status()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRef::Status( const char* s) { 
	mStatus = s;
	TellModelItemUpdated( UPD_STATUS);
}
