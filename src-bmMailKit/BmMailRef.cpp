/*
	BmMailRef.cpp
		$Id$
*/

#include <ctype.h>

#include <File.h>

#include "BmLogHandler.h"
#include "BmMailRef.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailRef* BmMailRef::CreateInstance( entry_ref &eref, ino_t node, struct stat& st) {
	BmMailRef* ref = new BmMailRef( eref, node, st);
	status_t ret;
	if ((ret = ref->InitCheck()) != B_OK) {
		// item is not of mimetype email, we skip it:
		delete ref;
		return NULL;
	} else {
		return ref;
	}
}

/*------------------------------------------------------------------------------*\
	BmMailRef( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( entry_ref &eref, ino_t node, struct stat& st)
	:	inherited( BString() << node)
	,	mEntryRef( eref)
	,	mInode( node)
	,	mInitCheck( B_NO_INIT)
{
	status_t err;
	BNode node;
	BString buffer;
	BString filetype;
	char* buf;
	size_t bufsize = 256;

	try {
		node.SetTo( &eref);
		(err = node.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get node for mail-file <") << eref.name << "> \n\nError:" << strerror(err));

		buffer.SetTo( '\0', bufsize);		// preallocate the bufsize we need
		buf = buffer.LockBuffer( 0);
		node.ReadAttr( "BEOS:TYPE", 		B_STRING_TYPE, 0, buf, bufsize);		filetype = buf; 	*buf=0;
		if (!filetype.ICompare("text/x-email")) {
			// file is indeed a mail, we fetch its attributes:
			node.ReadAttr( "MAIL:name", 		B_STRING_TYPE, 0, buf, bufsize);		mName = buf; 		*buf=0;
			node.ReadAttr( "MAIL:account", 	B_STRING_TYPE, 0, buf, bufsize);		mAccount = buf;	*buf=0;
			node.ReadAttr( "MAIL:cc", 			B_STRING_TYPE, 0, buf, bufsize);		mCc = buf;			*buf=0;
			node.ReadAttr( "MAIL:from", 		B_STRING_TYPE, 0, buf, bufsize);		mFrom = buf;		*buf=0;
			node.ReadAttr( "MAIL:priority", 	B_STRING_TYPE, 0, buf, bufsize);		mPriority = buf;	*buf=0;
			node.ReadAttr( "MAIL:reply", 		B_STRING_TYPE, 0, buf, bufsize);		mReplyTo = buf;	*buf=0;
			node.ReadAttr( "MAIL:status", 	B_STRING_TYPE, 0, buf, bufsize);		mStatus = buf;		*buf=0;
			node.ReadAttr( "MAIL:subject", 	B_STRING_TYPE, 0, buf, bufsize);		mSubject = buf;	*buf=0;
			node.ReadAttr( "MAIL:to", 			B_STRING_TYPE, 0, buf, bufsize);		mTo = buf;			*buf=0;
	
			mWhen = 0;
			node.ReadAttr( "MAIL:when", 		B_TIME_TYPE, 0, &mWhen, sizeof(time_t));
			mWhenString = TimeToString( mWhen);
	
			bool att1 = false;
			node.ReadAttr( "MAIL:attachment", B_BOOL_TYPE, 0, &att1, sizeof(att1));
			bool att2 = false;
			node.ReadAttr( "MAIL:attachments", B_BOOL_TYPE, 0, &att2, sizeof(att2));
			int32 att3 = 0;
			node.ReadAttr( "MAIL:has_attachment", B_INT32_TYPE, 0, &att3, sizeof(att3));
			mHasAttachments = att1 || att2 || att3>0;
	
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

			mInitCheck = B_OK;
		}
		buffer.UnlockBuffer( -1);
	} catch( exception &e) {
		buffer.UnlockBuffer( -1);
		BM_SHOWERR( e.what());
		return;
	}
}

/*------------------------------------------------------------------------------*\
	BmMailRef( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailRef::BmMailRef( BMessage* archive)
	:	inherited("")
	,	mInode( 0)
	,	mInitCheck( B_NO_INIT)
{
	try {
		status_t err;
		(err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BString("BmMailRef: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		mKey = BString() << mInode;

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
