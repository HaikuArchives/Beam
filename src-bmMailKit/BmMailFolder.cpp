/*
	BmMailFolder.cpp
		$Id$
*/

#include <Directory.h>

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRefList.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmMailFolder( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( entry_ref &eref, ino_t node, BmMailFolder* parent, 
									 time_t &modified)
	:	inherited( BString() << node, parent)
	,	mEntryRef( eref)
	,	mInode( node)
	,	mLastModified( modified)
	,	mMailRefList( NULL)
	,	mNeedsCacheUpdate( false)
	,	mNewMailCount( 0)
	,	mNewMailCountForSubfolders( 0)
	,	mName( eref.name)
{
	if (parent)
		parent->AddSubItem( this);
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BMessage* archive, BmMailFolder* parent)
	:	inherited( "", parent)
	,	mInode( 0)
	,	mMailRefList( NULL)
	,	mNeedsCacheUpdate( false)
	,	mNewMailCount( 0)
	,	mNewMailCountForSubfolders( 0)
{
	try {
		status_t err;
		(err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BString("BmMailFolder: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		mLastModified = FindMsgInt32( archive, MSG_LASTMODIFIED);
		mKey = BString() << mInode;
		mName = mEntryRef.name;
		if (parent)
			parent->AddSubItem( this);
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailFolder()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::~BmMailFolder() {
	delete mMailRefList;
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
		|| archive->AddInt32( MSG_NUMCHILDREN, mSubItemMap.size());
	if (deep && ret == B_OK) {
		BmModelItemMap::const_iterator pos;
		for( pos = mSubItemMap.begin(); pos != mSubItemMap.end(); ++pos) {
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

	BM_LOG3( BM_LogMailFolders, "BmMailFolder::CheckIfModifiedSince() - start");
	mailDir.SetTo( this->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not access \nmail-folder <") << Name() << "> \n\nError:" << strerror(err));
	time_t mtime;
	BM_LOG3( BM_LogMailFolders, "BmMailFolder::CheckIfModifiedSince() - getting modification time");
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mail-folder <") << Name() << "> \n\nError:" << strerror(err));
	BM_LOG3( BM_LogMailFolders, BString("Mtimes of folder ") << Name() << ": (" << mtime << "<->" << mLastModified << ")");
	BM_LOG3( BM_LogMailFolders, "BmMailFolder::CheckIfModifiedSince() - end");
	return mtime != mLastModified;
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCount()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCount() {
	mNewMailCount++;
	if (Parent())
		Parent()->BumpNewMailCountForSubfolders();
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCountForSubfolders()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCountForSubfolders() {
	mNewMailCountForSubfolders++;
	if (Parent())
		Parent()->BumpNewMailCountForSubfolders();
}

/*------------------------------------------------------------------------------*\
	MailRefList()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefList* BmMailFolder::MailRefList() {
	if (!mMailRefList) {
		CreateMailRefList();
	}
	return mMailRefList;
}

/*------------------------------------------------------------------------------*\
	CreateMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::CreateMailRefList() {
	if (!mMailRefList) {
		mMailRefList = new BmMailRefList( this, mNeedsCacheUpdate);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRefList() {
	delete mMailRefList;
	mMailRefList = NULL;
}
