/*
	BmMailFolder.cpp
		$Id$
*/

#include <Application.h>
#include <Directory.h>
#include <NodeMonitor.h>

#include "BmBasics.h"
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
	if (CheckIfModifiedSince( mLastModified, &mLastModified))
		mNeedsCacheUpdate = true;
	StartNodeMonitor();
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
		if (CheckIfModifiedSince( mLastModified, &mLastModified))
			mNeedsCacheUpdate = true;
		StartNodeMonitor();
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailFolder()
		-	d'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::~BmMailFolder() {
	StopNodeMonitor();
	RemoveMailRefList();
}

/*------------------------------------------------------------------------------*\
	StartNodeMonitor()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::StartNodeMonitor() {
	node_ref nref;
	nref.device = mEntryRef.device;
	nref.node = mInode;
	watch_node( &nref, B_WATCH_DIRECTORY, be_app_messenger);
}

/*------------------------------------------------------------------------------*\
	StartNodeMonitor()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::StopNodeMonitor() {
	node_ref nref;
	nref.device = mEntryRef.device;
	nref.node = mInode;
	watch_node( &nref, B_STOP_WATCHING, be_app_messenger);
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
bool BmMailFolder::CheckIfModifiedSince( time_t when, time_t* storeNewModTime) {
	status_t err;
	BDirectory mailDir;
	bool hasChanged = false;

	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - start");
	mailDir.SetTo( this->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not access \nmail-folder <") << Name() << "> \n\nError:" << strerror(err));
	time_t mtime;
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - getting modification time");
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mail-folder <") << Name() << "> \n\nError:" << strerror(err));
	BM_LOG3( BM_LogMailTracking, BString("checking if ") << Name() << ": (" << mtime << " > " << when << ")");
	if (mtime > when) {
		BM_LOG2( BM_LogMailTracking, BString("Mtime of folder <")<<Name()<<"> has changed!");
		if (storeNewModTime)
			*storeNewModTime = mtime;
		hasChanged = true;
	}
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - end");
	return hasChanged;
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
	if (!mMailRefList || mNeedsCacheUpdate) {
		CreateMailRefList();
	}
	return mMailRefList.Get();
}

/*------------------------------------------------------------------------------*\
	CreateMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::CreateMailRefList() {
	if (mMailRefList) {
		RemoveMailRefList();
	}
	mMailRefList = new BmMailRefList( this, mNeedsCacheUpdate);
	mNeedsCacheUpdate = false;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRefList() {
	mMailRefList = NULL;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::MoveMailsHere( BList& refs) {
	static uint32 counter = 1;
	BDirectory thisDir( &mEntryRef);
	int32 count = refs.CountItems();
	// move each mailref into this folder:
	for( int32 i=0; i<count; ++i) {
		entry_ref* ref = static_cast<entry_ref*>( refs.ItemAt(i));
		BEntry entry( ref);
		if (entry.InitCheck() == B_OK) {
			if (entry.MoveTo( &thisDir) == B_FILE_EXISTS) {
				// increment counter until we have found a unique name:
				while ( entry.MoveTo( &thisDir, (BString(ref->name)<<"_"<<counter++).String()) == B_FILE_EXISTS)
					;
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	AddMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddMailRef( entry_ref& eref, int64 node, struct stat& st) {
	if (mMailRefList)
		mMailRefList->AddMailRef( eref, node, st);
	else
		mNeedsCacheUpdate = true;
}

/*------------------------------------------------------------------------------*\
	HasMailRef()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolder::HasMailRef( BString key) {
	if (mMailRefList)
		return mMailRefList->FindItemByKey( key) != NULL;
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRef( BString key) {
	if (mMailRefList)
		mMailRefList->RemoveItemFromList( key);
	else
		mNeedsCacheUpdate = true;
}

