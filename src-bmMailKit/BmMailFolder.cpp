/*
	BmMailFolder.cpp
		$Id$
*/

#include <Application.h>
#include <Directory.h>
#include <NodeMonitor.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmMailFolder( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node, 
									 BmMailFolder* parent, time_t &modified)
	:	inherited( BString() << node, model, parent)
	,	mEntryRef( eref)
	,	mInode( node)
	,	mLastModified( modified)
	,	mMailRefList( NULL)
	,	mNewMailCount( 0)
	,	mNewMailCountForSubfolders( 0)
	,	mName( eref.name)
{
	StartNodeMonitor();
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BMessage* archive, BmMailFolderList* model, BmMailFolder* parent)
	:	inherited( "", model, parent)
	,	mInode( 0)
	,	mMailRefList( NULL)
	,	mNewMailCount( 0)
	,	mNewMailCountForSubfolders( 0)
{
	try {
		status_t err;
		(err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) == B_OK
													|| BM_THROW_RUNTIME( BString("BmMailFolder: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		mLastModified = FindMsgInt32( archive, MSG_LASTMODIFIED);
		Key( BString() << mInode);
		mName = mEntryRef.name;
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
	watch_node( &nref, B_WATCH_DIRECTORY, BMessenger( TheNodeMonitor));
}

/*------------------------------------------------------------------------------*\
	StartNodeMonitor()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::StopNodeMonitor() {
	node_ref nref;
	nref.device = mEntryRef.device;
	nref.node = mInode;
	watch_node( &nref, B_STOP_WATCHING, BMessenger( TheNodeMonitor));
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
		|| archive->AddInt32( MSG_NUMCHILDREN, size());
	if (deep && ret == B_OK) {
		BmModelItemMap::const_iterator pos;
		for( pos = begin(); pos != end(); ++pos) {
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
	CheckIfModifiedSinceLastTime()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolder::CheckIfModifiedSinceLastTime() {
	return CheckIfModifiedSince( mLastModified, &mLastModified);
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
void BmMailFolder::BumpNewMailCount( int32 offset) {
	mNewMailCount += offset;
	TheMailFolderList->TellModelItemUpdated( this, UPD_NAME);
	if (Parent())
		Parent()->BumpNewMailCountForSubfolders( offset);
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCountForSubfolders()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCountForSubfolders( int32 offset) {
	mNewMailCountForSubfolders += offset;
	TheMailFolderList->TellModelItemUpdated( this, UPD_NAME);
	if (Parent())
		Parent()->BumpNewMailCountForSubfolders( offset);
}

/*------------------------------------------------------------------------------*\
	MailRefList()
		-	
\*------------------------------------------------------------------------------*/
BmMailRefList* BmMailFolder::MailRefList() {
	if (!mMailRefList)
		CreateMailRefList();
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
	mMailRefList = new BmMailRefList( this);
}

/*------------------------------------------------------------------------------*\
	RemoveMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRefList() {
	mMailRefList = NULL;
}

/*------------------------------------------------------------------------------*\
	ReCreateMailRefList()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RecreateCache() {
	if (mMailRefList) {
		mMailRefList->Cleanup();
		mMailRefList->MarkCacheAsDirty();
		mMailRefList->StartJobInNewThread();
	}
}

/*------------------------------------------------------------------------------*\
	AddMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddMailRef( entry_ref& eref, struct stat& st) {
	if (mMailRefList) {
		if (!mMailRefList->AddMailRef( eref, st))
			return;								// mail-ref already exists, we quit
	}
	// if mail-ref is flagged new, we have to tell the mailfolderlist that we own
	// this new-mail and increment our new-mail-counter (causing an update):
	if (TheMailFolderList->NodeIsFlaggedNew( st.st_ino))
		TheMailFolderList->SetFolderForNodeFlaggedNew( st.st_ino, this);
}

/*------------------------------------------------------------------------------*\
	HasMailRef()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolder::HasMailRef( BString key) {
	if (mMailRefList)
		return mMailRefList->FindItemByKey( key);
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRef( ino_t node) {
	if (mMailRefList) {
		if (!mMailRefList->RemoveItemFromList( BString()<<node))
			return;								// mail-ref didn't exist, we quit
	}
	// if mail-ref is flagged new, we have to tell the mailfolderlist that we no 
	// longer own this new-mail and decrement our new-mail-counter (causing an update):
	if (TheMailFolderList->NodeIsFlaggedNew( node))
		TheMailFolderList->SetFolderForNodeFlaggedNew( node, NULL);
}

/*------------------------------------------------------------------------------*\
	CreateSubFolder( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::CreateSubFolder( BString name) {
	BDirectory thisDir( &mEntryRef);
	if (thisDir.InitCheck() == B_OK) {
		thisDir.CreateDirectory( name.String(), NULL);
	}
}

/*------------------------------------------------------------------------------*\
	Rename( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::Rename( BString newName) {
	BEntry entry( EntryRefPtr());
	if (entry.InitCheck()==B_OK) {
		status_t err;
		(err = entry.Rename( newName.String())) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not rename mail-folder\n\nError:") << strerror(err));
	}
}

/*------------------------------------------------------------------------------*\
	MoveToTrash( )
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::MoveToTrash() {
	::MoveToTrash( EntryRefPtr(), 1);
}
