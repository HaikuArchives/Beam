/*
	BmMailFolder.cpp
		$Id$
*/

#include "BmMailFolder.h"

/*------------------------------------------------------------------------------*\
	BmMailFolder( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( entry_ref &eref, ino_t node, BmMailFolder *parent, 
									 time_t &modified)
:	mEntryRef( eref)
,	mInode( node)
,	mLastModified( modified)
,	mParent( parent)
,	mNeedsCacheUpdate( false)
,	mNewMailCount( 0)
{
	if (parent)
		parent->AddSubFolder( this);
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BMessage *archive, BmMailFolder *parent)
:	mInode( 0)
,	mParent( parent)
,	mNeedsCacheUpdate( false)
,	mNewMailCount( 0)
{
	try {
		status_t err;
		(err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BString("BmMailFolder: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		mLastModified = FindMsgInt32( archive, MSG_LASTMODIFIED);
		if (parent)
			parent->AddSubFolder( this);
	} catch (exception &e) {
		ShowAlert( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailFolder()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::~BmMailFolder() {
	BmFolderMap::iterator iter;
	for( iter = mSubFolderMap.begin(); iter != mSubFolderMap.end(); ++iter) {
		delete iter->second;
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailFolder::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddInt64( MSG_INODE, mInode)
		|| archive->AddInt32( MSG_LASTMODIFIED, mLastModified)
		|| archive->AddInt32( MSG_NUMCHILDREN, mSubFolderMap.size());
	if (deep && ret == B_OK) {
		BmFolderMap::const_iterator pos;
		for( pos = mSubFolderMap.begin(); pos != mSubFolderMap.end(); ++pos) {
			if (ret == B_OK) {
				BMessage msg;
				ret = pos->second->Archive( &msg, deep)
						|| archive->AddMessage( MSG_CHILDREN, &msg);
			}
		}
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	CheckIfModifiedSince()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolder::CheckIfModifiedSince() {
	status_t err;
	BDirectory mailDir;

	mailDir.SetTo( this->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not access \nmail-folder <") << Name() << "> \n\nError:" << strerror(err));
	time_t mtime;
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mail-folder <") << Name() << "> \n\nError:" << strerror(err));
	BM_LOG3( BM_LogMailFolders, BString("Mtimes of folder ") << Name() << ": (" << mtime << "<->" << mLastModified << ")");
	return mtime != mLastModified;
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCount()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCount() {
	mNewMailCount++;
	if (mParent)
		mParent->BumpNewMailCountForSubfolders();
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCountForSubfolders()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCountForSubfolders() {
	mNewMailCountForSubfolders++;
	if (mParent)
		mParent->BumpNewMailCountForSubfolders();
}

/*------------------------------------------------------------------------------*\
	AddSubFolder()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddSubFolder( BmMailFolder *child) {
	mSubFolderMap[child->ID()] = child;
}
