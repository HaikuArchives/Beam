/*
	BmMailFolder.cpp
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

BLocker BmMailFolder::nRefListLocker( "RefListLocker");

/*------------------------------------------------------------------------------*\
	CreateDummyInstance()
		-	static creator-func, creates dummy mail-folder (used in prefs)
\*------------------------------------------------------------------------------*/
BmRef<BmMailFolder> BmMailFolder::CreateDummyInstance() {
	BMessage archive;
	entry_ref eref;
	eref.set_name("dummy");
	archive.AddRef( MSG_ENTRYREF, &eref);
	archive.AddInt64( MSG_INODE, 0);
	archive.AddInt32( MSG_LASTMODIFIED, time(NULL));
	archive.AddInt32( MSG_NUMCHILDREN, 0);
	return new BmMailFolder( &archive, NULL, NULL);
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node, 
									 BmMailFolder* parent, time_t &modified)
	:	inherited( BmString() << node, model, parent)
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
													|| BM_THROW_RUNTIME( BmString("BmMailFolder: Could not find msg-field ") << MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mInode = FindMsgInt64( archive, MSG_INODE);
		mLastModified = FindMsgInt32( archive, MSG_LASTMODIFIED);
		Key( BmString() << mInode);
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
		-	instructs the node-monitor to watch this folder
\*------------------------------------------------------------------------------*/
void BmMailFolder::StartNodeMonitor() {
	node_ref nref;
	nref.device = mEntryRef.device;
	nref.node = mInode;
	watch_node( &nref, B_WATCH_DIRECTORY, BMessenger( TheMailMonitor));
}

/*------------------------------------------------------------------------------*\
	StopNodeMonitor()
		-	instructs the node-monitor to stop watching this folder
\*------------------------------------------------------------------------------*/
void BmMailFolder::StopNodeMonitor() {
	node_ref nref;
	nref.device = mEntryRef.device;
	nref.node = mInode;
	watch_node( &nref, B_STOP_WATCHING, BMessenger( TheMailMonitor));
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	archives this folder into the given message-archive
\*------------------------------------------------------------------------------*/
status_t BmMailFolder::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddInt64( MSG_INODE, mInode)
//		|| archive->AddInt32( MSG_LASTMODIFIED, mLastModified)
		|| archive->AddInt32( MSG_LASTMODIFIED, time(NULL))
							// bump time to the last time folder-cache has been written
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
		-	determines if this mail-folder has been written to since the last time
			Beam ran.
\*------------------------------------------------------------------------------*/
bool BmMailFolder::CheckIfModifiedSinceLastTime() {
	return CheckIfModifiedSince( mLastModified, &mLastModified);
}

/*------------------------------------------------------------------------------*\
	CheckIfModifiedSince( when, storeNewModTime)
		-	determines if this mail-folder has been written to after the time given
		-	if param storeNewModTime is provided it is filled with the actual
			modification time of this folder
\*------------------------------------------------------------------------------*/
bool BmMailFolder::CheckIfModifiedSince( time_t when, time_t* storeNewModTime) {
	status_t err;
	BDirectory mailDir;
	bool hasChanged = false;
	
	if (!mInode)
		return false;							// dummy folder has never modified

	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - start");
	mailDir.SetTo( this->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not access \nmail-folder <") << Name() << "> \n\nError:" << strerror(err));
	time_t mtime;
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - getting modification time");
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not get mtime \nfor mail-folder <") << Name() << "> \n\nError:" << strerror(err));
	BM_LOG3( BM_LogMailTracking, BmString("checking if ") << Name() << ": (" << mtime << " > " << when << ")");
	if (mtime > when) {
		BM_LOG2( BM_LogMailTracking, BmString("Mtime of folder <")<<Name()<<"> has changed!");
		if (storeNewModTime)
			*storeNewModTime = mtime;
		hasChanged = true;
	}
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - end");
	return hasChanged;
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCount( offset)
		-	increases this folder's new-mail counter by the given offset
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCount( int32 offset) {
	mNewMailCount += offset;
	if (mNewMailCount < 0)
		mNewMailCount = 0;
	TheMailFolderList->TellModelItemUpdated( this, UPD_NEW_STATUS);
	BmMailFolder* parent( dynamic_cast< BmMailFolder*>( Parent()));
	if (parent)
		parent->BumpNewMailCountForSubfolders( offset);
}

/*------------------------------------------------------------------------------*\
	BumpNewMailCountForSubfolders()
		-	increases this folder's new-mail-in-subfolders counter by the given 
			offset
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpNewMailCountForSubfolders( int32 offset) {
	mNewMailCountForSubfolders += offset;
	if (mNewMailCount < 0)
		mNewMailCount = 0;
	TheMailFolderList->TellModelItemUpdated( this, UPD_NEW_STATUS);
	BmMailFolder* parent( dynamic_cast< BmMailFolder*>( Parent()));
	if (parent)
		parent->BumpNewMailCountForSubfolders( offset);
}

/*------------------------------------------------------------------------------*\
	MailRefList()
		-	returns the mailref-list for this folder
		-	if the mailref-list does not exist yet, it is created
\*------------------------------------------------------------------------------*/
BmRef<BmMailRefList> BmMailFolder::MailRefList() {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":RemoveMailRefList(): Unable to get lock");
	if (!mMailRefList)
		CreateMailRefList();
	return mMailRefList;
}

/*------------------------------------------------------------------------------*\
	CreateMailRefList()
		-	creates the mailref-list for this folder
\*------------------------------------------------------------------------------*/
void BmMailFolder::CreateMailRefList() {
	mMailRefList = new BmMailRefList( this);
}

/*------------------------------------------------------------------------------*\
	RemoveMailRefList()
		-	destroys this folder's mailref-list, freeing some memory
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRefList() {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":RemoveMailRefList(): Unable to get lock");
	mMailRefList = NULL;
}

/*------------------------------------------------------------------------------*\
	CleanupForMailRefList( refList)
		-	cleans the given refList from this folder if it still is the current
			mailref-list (otherwise nothing happens)
\*------------------------------------------------------------------------------*/
void BmMailFolder::CleanupForMailRefList( BmMailRefList* refList) {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":CleanupForMailRefList(): Unable to get lock");
	if (mMailRefList == refList)
		RemoveMailRefList();
}

/*------------------------------------------------------------------------------*\
	ReCreateMailRefList()
		-	marks this folder's mailref-list as dirty, causing the reflist's cache
			to be recreated
\*------------------------------------------------------------------------------*/
void BmMailFolder::RecreateCache() {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":RecreateCache(): Unable to get lock");
	CreateMailRefList();
	if (mMailRefList)
		mMailRefList->MarkCacheAsDirty();
}

/*------------------------------------------------------------------------------*\
	AddMailRef( eref, st)
		-	adds the mail-ref specified by the given entry_ref to this folder's mailref-
			list
		-	the param st contains the mail-ref's stat-info
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddMailRef( entry_ref& eref, struct stat& st) {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":AddMailRef(): Unable to get lock");
	BM_LOG2( BM_LogMailTracking, Name()+" adding mail-ref " << st.st_ino);
	if (mMailRefList) {
		if (!mMailRefList->AddMailRef( eref, st)) {
			BM_LOG2( BM_LogMailTracking, Name()+" mail-ref already exists.");
		}
	}
	// if mail-ref is flagged new, we have to tell the mailfolderlist that we own
	// this new-mail and increment our new-mail-counter (causing an update):
	if (TheMailFolderList->NodeIsFlaggedNew( st.st_ino))
		TheMailFolderList->SetFolderForNodeFlaggedNew( st.st_ino, this);
}

/*------------------------------------------------------------------------------*\
	HasMailRef( key)
		-	determines if the mail-ref specified by the given key exists within
			this folders mailref-list
\*------------------------------------------------------------------------------*/
bool BmMailFolder::HasMailRef( BmString key) {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":HasMailRef(): Unable to get lock");
	if (mMailRefList)
		return mMailRefList->FindItemByKey( key);
	else
		return false;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef( node)
		-	removes the mail-ref specified by the given node from this folder's
			mailref-list
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRef( ino_t node) {
	BmAutolock lock( nRefListLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( Name() + ":RemoveMailRef(): Unable to get lock");
	BM_LOG2( BM_LogMailTracking, Name()+" removing mail-ref "<<node);
	if (mMailRefList) {
		if (!mMailRefList->RemoveItemFromList( BmString()<<node)) {
			BM_LOG2( BM_LogMailTracking, Name()+" mail-ref doesn't exist.");
		}
	}
	// if mail-ref is flagged new, we have to tell the mailfolderlist that we no 
	// longer own this new-mail and decrement our new-mail-counter (causing an update):
	if (TheMailFolderList->NodeIsFlaggedNew( node))
		TheMailFolderList->SetFolderForNodeFlaggedNew( node, NULL);
}

/*------------------------------------------------------------------------------*\
	CreateSubFolder( name)
		-	creates a new sub-folder of the given name in this mail-folder
\*------------------------------------------------------------------------------*/
void BmMailFolder::CreateSubFolder( BmString name) {
	BDirectory thisDir( &mEntryRef);
	if (thisDir.InitCheck() == B_OK) {
		thisDir.CreateDirectory( name.String(), NULL);
	}
}

/*------------------------------------------------------------------------------*\
	Rename( newName)
		-	rename this folder to the given name
\*------------------------------------------------------------------------------*/
void BmMailFolder::Rename( BmString newName) {
	BEntry entry( EntryRefPtr());
	if (entry.InitCheck()==B_OK) {
		status_t err;
		(err = entry.Rename( newName.String())) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not rename mail-folder\n\nError:") << strerror(err));
	}
}

/*------------------------------------------------------------------------------*\
	MoveToTrash()
		-	moves this folder to the trash
\*------------------------------------------------------------------------------*/
void BmMailFolder::MoveToTrash() {
	::MoveToTrash( EntryRefPtr(), 1);
}
