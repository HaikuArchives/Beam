/*
	BmMailRef.cpp
		$Id$
*/

#include "BmApp.h"
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
	char* buf;
	size_t bufsize = 256;

	try {
		node.SetTo( &eref);
		(err = node.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get node for mail-file <") << eref.name << "> \n\nError:" << strerror(err));

		buffer.SetTo( '\0', bufsize);		// preallocate the bufsize we need
		buf = buffer.LockBuffer( 0);
		node.ReadAttr( "MAIL:name", 		B_STRING_TYPE, 0, buf, bufsize);		mName = buf; 		*buf=0;
		node.ReadAttr( "MAIL:account", 	B_STRING_TYPE, 0, buf, bufsize);		mAccount = buf;	*buf=0;
		node.ReadAttr( "MAIL:cc", 			B_STRING_TYPE, 0, buf, bufsize);		mCc = buf;			*buf=0;
		node.ReadAttr( "MAIL:from", 		B_STRING_TYPE, 0, buf, bufsize);		mFrom = buf;		*buf=0;
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
		mSizeString = BytesToString(mSize,true);
		mCreated = st.st_ctime;
		mCreatedString = TimeToString( mCreated);

		buffer.UnlockBuffer( -1);
	} catch( exception &e) {
		buffer.UnlockBuffer( -1);
		BM_SHOWERR( e.what());
		return;
	}

	/* TODO: check for mimetype mail and fetch attributes here */
	mInitCheck = B_OK;
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
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRef::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddInt64( MSG_INODE, mInode);
	return ret;
}

