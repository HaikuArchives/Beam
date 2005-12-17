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
#include <Path.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailFolderList.h"
#include "BmMailMonitor.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

BLocker BmMailFolder::nRefListLocker( "RefListLocker");

const char* const BmMailFolder::MSG_ENTRYREF = 		"bm:eref";
const char* const BmMailFolder::MSG_INODE = 			"bm:inod";
const char* const BmMailFolder::MSG_LASTMODIFIED = "bm:lmod";
const char* const BmMailFolder::MSG_MAILCOUNT = 	"bm:mcnt";
const char* const BmMailFolder::MSG_SELECTED_KEY =	"bm:selk";
const char* const BmMailFolder::MSG_STATEINFO_CONNECTED = "bm:sicn";

//	message component definitions for status-msgs:
const char* const BmMailFolder::MSG_NAME = 			"bm:fname";

// flags indicating which parts are to be updated:
const BmUpdFlags BmMailFolder::UPD_SPECIAL_COUNT 			= 	1<<2;
const BmUpdFlags BmMailFolder::UPD_TOTAL_COUNT 				= 	1<<3;
const BmUpdFlags BmMailFolder::UPD_HAVE_SPECIAL_STATUS	= 	1<<4;

const char* BmMailFolder::DRAFT_FOLDER_NAME		= "draft";
const char* BmMailFolder::IN_FOLDER_NAME			= "in";
const char* BmMailFolder::OUT_FOLDER_NAME			= "out";
const char* BmMailFolder::QUARANTINE_FOLDER_NAME= "quarantine";
const char* BmMailFolder::SPAM_FOLDER_NAME		= "spam";

const int16 BmMailFolder::nArchiveVersion = 4;

/*------------------------------------------------------------------------------*\
	IsSystemFolderPath( path)
\*------------------------------------------------------------------------------*/
bool BmMailFolder::IsSystemFolderPath( const BPath& path)
{
	static BmString cachedMailbox;
	static node_ref cachedMailboxRef;
	//
	BmString mbox = ThePrefs->GetString("MailboxPath");
	if (cachedMailbox != mbox) {
		// cache traversed entry ref for mailbox:
		BEntry mboxEntry(mbox.String(), true);
		BNode mboxNode(&mboxEntry);
		if (mboxEntry.GetNodeRef(&cachedMailboxRef) != B_OK)
			return false;
		cachedMailbox = mbox;
	}
	entry_ref pathRef;
	if (get_ref_for_path(path.Path(), &pathRef) != B_OK)
		return false;
	if (pathRef.device != cachedMailboxRef.device
	|| pathRef.directory != cachedMailboxRef.node)
		return false;
	return (!strcmp(pathRef.name, DRAFT_FOLDER_NAME)
	|| !strcmp(pathRef.name, IN_FOLDER_NAME)
	|| !strcmp(pathRef.name, OUT_FOLDER_NAME)
	|| !strcmp(pathRef.name, QUARANTINE_FOLDER_NAME)
	|| !strcmp(pathRef.name, SPAM_FOLDER_NAME));
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( eref, parent, modified)
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BmMailFolderList* model, entry_ref &eref, 
									 ino_t node, BmMailFolder* parent, time_t &modified)
	:	inherited( BmString() << node, model, parent)
	,	mEntryRef( eref)
	,	mLastModified( modified)
	,	mMailCount( -1)
	,	mRefListStateInfoConnectedToParent( true)
	,	mMailRefList( NULL)
	,	mSpecialMailCountForSubfolders( 0)
	,	mName( eref.name)
{
	mNodeRef.node = node;
	mNodeRef.device = eref.device;
	StartNodeMonitor();
}

/*------------------------------------------------------------------------------*\
	BmMailFolder( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailFolder::BmMailFolder( BMessage* archive, BmMailFolderList* model, 
									 BmMailFolder* parent)
	:	inherited( "", model, parent)
	,	mMailRefList( NULL)
	,	mRefListStateInfoConnectedToParent( true)
	,	mSpecialMailCountForSubfolders( 0)
	,	mMailCount( -1)
{
	try {
		int16 version;
		if (archive->FindInt16( MSG_VERSION, &version) != B_OK)
			version = 0;
		status_t err;
		if ((err = archive->FindRef( MSG_ENTRYREF, &mEntryRef)) != B_OK)
			BM_THROW_RUNTIME( BmString("BmMailFolder: Could not find msg-field ") 
										<< MSG_ENTRYREF << "\n\nError:" << strerror(err));
		mNodeRef.node = FindMsgInt64( archive, MSG_INODE);
		if (ThePrefs->MailboxVolume.Device() != mEntryRef.device)
			// mailbox-volume has changed since last session, we update!
			// (in case you wonder, this can happen if the volume of the mailbox
			// has been unmounted and then remounted later, resulting in a 
			// [possibly] new device-number).
			mEntryRef.device = ThePrefs->MailboxVolume.Device();
		mNodeRef.device = mEntryRef.device;
		mLastModified = FindMsgInt32( archive, MSG_LASTMODIFIED);
		Key( BM_REFKEY( mNodeRef));
		mName = mEntryRef.name;
		if (version > 1)
			mMailCount = FindMsgInt32( archive, MSG_MAILCOUNT);
		if (version > 2)
			mSelectedRefKey = FindMsgString( archive, MSG_SELECTED_KEY);
		if (version > 3)
			mRefListStateInfoConnectedToParent 
				= FindMsgBool( archive, MSG_STATEINFO_CONNECTED);
		StartNodeMonitor();
	} catch (BM_error &e) {
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
	WatchNode( &mNodeRef, B_WATCH_DIRECTORY, TheMailMonitor);
}

/*------------------------------------------------------------------------------*\
	StopNodeMonitor()
		-	instructs the node-monitor to stop watching this folder
\*------------------------------------------------------------------------------*/
void BmMailFolder::StopNodeMonitor() {
	WatchNode( &mNodeRef, B_STOP_WATCHING, TheMailMonitor);
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	archives this folder into the given message-archive
\*------------------------------------------------------------------------------*/
status_t BmMailFolder::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep)
		|| archive->AddRef( MSG_ENTRYREF, &mEntryRef)
		|| archive->AddInt64( MSG_INODE, mNodeRef.node)
		|| archive->AddInt32( MSG_LASTMODIFIED, time(NULL))
							// bump time to the last time folder-cache has 
							// been written
		|| archive->AddInt32( MSG_NUMCHILDREN, size())
		|| archive->AddInt32( MSG_MAILCOUNT, mMailCount)
		|| archive->AddString( MSG_SELECTED_KEY, mSelectedRefKey.String())
		|| archive->AddBool( MSG_STATEINFO_CONNECTED, 
									mRefListStateInfoConnectedToParent);
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
	IsOutbound()
		-	returns whether or not this folder is outbound (i.e. is or lives
			underneath the folder 'mailbox/out').
\*------------------------------------------------------------------------------*/
bool BmMailFolder::IsOutbound() {
	BmRef<BmListModelItem> item = this;
	BmRef<BmListModelItem> parent;
	while((parent = item->Parent()) != NULL && parent->Parent() != NULL)
		item = parent;
	// now item should be a folder living underneath 'mailbox'
	BmMailFolder* folder = dynamic_cast<BmMailFolder*>(item.Get());
	return (folder && folder->DisplayKey() == OUT_FOLDER_NAME);
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
	
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - start");
	mailDir.SetTo( this->EntryRefPtr());
	if ((err = mailDir.InitCheck()) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not access \nmail-folder <") << Name() 
									<< "> \n\nError:" << strerror(err));
	time_t mtime;
	BM_LOG3( BM_LogMailTracking, 
				"BmMailFolder::CheckIfModifiedSince() - getting modification time");
	if ((err = mailDir.GetModificationTime( &mtime)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get mtime \nfor mail-folder <") 
									<< Name() << "> \n\nError:" << strerror(err));
	BM_LOG3( BM_LogMailTracking, 
				BmString("checking if ") << Name() << ": (" << mtime << " > " 
					<< when << ")");
	if (mtime > when) {
		BM_LOG2( BM_LogMailTracking, 
					BmString("Mtime of folder <")<<Name()<<"> has changed!");
		if (storeNewModTime)
			*storeNewModTime = mtime;
		hasChanged = true;
	}
	BM_LOG3( BM_LogMailTracking, "BmMailFolder::CheckIfModifiedSince() - end");
	return hasChanged;
}

/*------------------------------------------------------------------------------*\
	AddSpecialFlagForMailRef(key)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddSpecialFlagForMailRef(const BmString& key) {
	int32 oldCount = 	mSpecialMailRefSet.size();
	mSpecialMailRefSet.insert(key);
	int32 offset = mSpecialMailRefSet.size() - oldCount;
	if (offset) {
		BmUpdFlags upd = UPD_SPECIAL_COUNT;
		if (!oldCount)
			// new mail status has changed (was 0 before)
			upd |= UPD_HAVE_SPECIAL_STATUS;
		TheMailFolderList->TellModelItemUpdated( this, upd);
		BmRef<BmMailFolder> parent( dynamic_cast< BmMailFolder*>( Parent().Get()));
		if (parent)
			parent->BumpSpecialMailCountForSubfolders( offset);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveSpecialFlagForMailRef(key)
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveSpecialFlagForMailRef(const BmString& key) {
	int32 oldCount = 	mSpecialMailRefSet.size();
	mSpecialMailRefSet.erase(key);
	int32 offset = mSpecialMailRefSet.size() - oldCount;
	if (offset) {
		BmUpdFlags upd = UPD_SPECIAL_COUNT;
		if (oldCount>0 && mSpecialMailRefSet.empty())
			// new mail status has changed (is 0 now)
			upd |= UPD_HAVE_SPECIAL_STATUS;
		TheMailFolderList->TellModelItemUpdated( this, upd);
		BmRef<BmMailFolder> parent( dynamic_cast< BmMailFolder*>( Parent().Get()));
		if (parent)
			parent->BumpSpecialMailCountForSubfolders( offset);
	}
}

/*------------------------------------------------------------------------------*\
	BumpMailCount( offset)
		-	changes mail-count for this folder
		-	tells that model has been updated
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpMailCount( int32 offset) {
	if (mMailCount < 0)
		return;		// mail-count is unknown, so we can't bump it
	if (offset) {
		mMailCount += offset;
		TellModelItemUpdated( UPD_TOTAL_COUNT);
	}
}

/*------------------------------------------------------------------------------*\
	MailCount( count)
		-	sets mail-count for this folder
		-	tells that model has been updated
\*------------------------------------------------------------------------------*/
void BmMailFolder::MailCount( int32 count) {
	if (mMailCount != count) {
		mMailCount = count;
		TellModelItemUpdated( UPD_TOTAL_COUNT);
	}
}

/*------------------------------------------------------------------------------*\
	BumpSpecialMailCountForSubfolders()
		-	increases this folder's new-mail-in-subfolders counter by the given 
			offset
\*------------------------------------------------------------------------------*/
void BmMailFolder::BumpSpecialMailCountForSubfolders( int32 offset) {
	int32 oldCount = 	mSpecialMailCountForSubfolders;
	mSpecialMailCountForSubfolders += offset;
	if (mSpecialMailCountForSubfolders < 0)
		mSpecialMailCountForSubfolders = 0;
	BmUpdFlags upd = UPD_SPECIAL_COUNT;
	if (oldCount*mSpecialMailCountForSubfolders==0 && offset)
		// new mail status has changed (either was 0 before or is 0 now)
		upd |= UPD_HAVE_SPECIAL_STATUS;
	TheMailFolderList->TellModelItemUpdated( this, upd);
	BmRef<BmMailFolder> parent( dynamic_cast< BmMailFolder*>( Parent().Get()));
	if (parent)
		parent->BumpSpecialMailCountForSubfolders( offset);
}

/*------------------------------------------------------------------------------*\
	MailRefList()
		-	returns the mailref-list for this folder
		-	if the mailref-list does not exist yet, it is created
\*------------------------------------------------------------------------------*/
BmRef<BmMailRefList> BmMailFolder::MailRefList() {
	BmAutolockCheckGlobal lock( nRefListLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( Name() + ":RemoveMailRefList(): Unable to get lock");
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
	BmAutolockCheckGlobal lock( nRefListLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( Name() + ":RemoveMailRefList(): Unable to get lock");
	mMailRefList = NULL;
}

/*------------------------------------------------------------------------------*\
	CleanupForMailRefList( refList)
		-	cleans the given refList from this folder if it still is the current
			mailref-list (otherwise nothing happens)
\*------------------------------------------------------------------------------*/
void BmMailFolder::CleanupForMailRefList( BmMailRefList* refList) {
	BmRef<BmMailRefList> currRefList = MailRefList();
	if (currRefList == refList)
		RemoveMailRefList();
}

/*------------------------------------------------------------------------------*\
	UpdateName( eref)
		-	sets a new name for this folder (is called by node-monitor)
		-	adjusts foreign-keys accordingly
		-	tells that model has been updated
\*------------------------------------------------------------------------------*/
void BmMailFolder::UpdateName( const entry_ref& eref) {
	// ...determine path to folder (but skip root-folder):
	BmString path;
	BmListModelItem* curr;
	for(	curr = Parent().Get(); 
			curr && curr->Parent(); curr = curr->Parent().Get()) {
		if (!path.Length())
			path.Prepend( curr->DisplayKey());
		else		
			path.Prepend( curr->DisplayKey() + "/");
	}
	// set new entry-ref:
	BmString oldName = mName;
	EntryRef( eref);
	TheMailFolderList->TellModelItemUpdated( this, UPD_KEY);
	// adjust foreign keys:
	TheMailFolderList->AdjustForeignKeys( 
		path.Length() ? path + "/" + oldName : oldName,
		path.Length() ? path + "/" + mName : mName
	);
}

/*------------------------------------------------------------------------------*\
	ReCreateMailRefList()
		-	marks this folder's mailref-list as dirty, causing the reflist's cache
			to be recreated
\*------------------------------------------------------------------------------*/
void BmMailFolder::RecreateCache() {
	BmRef<BmMailRefList> refList = MailRefList();
	if (refList) {
		refList->MarkCacheAsDirty();
		refList->StartJobInNewThread();
	}
}

/*------------------------------------------------------------------------------*\
	AddMailRef( eref, st)
		-	adds the mail-ref specified by the given entry_ref to this folder's 
			mailref-list
		-	the param st contains the mail-ref's stat-info
\*------------------------------------------------------------------------------*/
void BmMailFolder::AddMailRef( entry_ref& eref, struct stat& st) {
	node_ref nref;
	nref.node = st.st_ino;
	nref.device = st.st_dev;
	BmString key( BM_REFKEY( nref));
	BM_LOG2( BM_LogMailTracking, Name()+" adding mail-ref " << key);
	BmRef<BmMailRefList> refList = MailRefList();
	if (refList) {
		BmRef<BmMailRef> ref( refList->AddMailRef( eref, st));
		if (ref) {
			if (ref->IsSpecial())
				AddSpecialFlagForMailRef(key);
		} else
			BM_LOG2( BM_LogMailTracking, Name()+" mail-ref already exists.");
	} else
		// ref-list couldn't be created (?!?) we mark the mail-count as unknown:
		mMailCount = -1;
}

/*------------------------------------------------------------------------------*\
	FindMailRefByKey()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmMailFolder::FindMailRefByKey( const BmString& key) {
	BmRef<BmListModelItem> foundRef;
	{ // scope for ref:
		BmRef<BmMailRefList> refList = mMailRefList;
		if (refList)
			foundRef = refList->FindItemByKey( key);
	}
	if (!foundRef) {
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); !foundRef && iter != end(); ++iter) {
			BmMailFolder* subFolder = dynamic_cast< BmMailFolder*>(
				iter->second.Get()
			);
			if (subFolder)
				foundRef = subFolder->FindMailRefByKey( key);
		}
	}
	return foundRef;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef( node)
		-	removes the mail-ref specified by the given node from this folder's
			mailref-list
\*------------------------------------------------------------------------------*/
void BmMailFolder::RemoveMailRef( const node_ref& nref) {
 	BmString key( BM_REFKEY( nref));
 	BM_LOG2( BM_LogMailTracking, Name()+" removing mail-ref " << key);
	BmRef<BmMailRefList> refList = MailRefList();
	if (refList) {
		refList->RemoveMailRef( key);
		RemoveSpecialFlagForMailRef(key);
	} else
		// ref-list couldn't be created (?!?) we mark the mail-count as unknown:
		mMailCount = -1;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef( node)
		-	removes the mail-ref specified by the given node from this folder's
			mailref-list
\*------------------------------------------------------------------------------*/
void BmMailFolder::UpdateMailRef( const node_ref& nref) {
 	BmString key( BM_REFKEY( nref));
 	BM_LOG2( BM_LogMailTracking, Name()+" updating mail-ref " << key);
	BmRef<BmMailRefList> refList = MailRefList();
	if (refList)
		refList->UpdateMailRef( key);
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
		if ((err = entry.Rename( newName.String())) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not rename mail-folder\n\nError:") 
										<< strerror(err));
	}
}

/*------------------------------------------------------------------------------*\
	MoveToTrash()
		-	moves this folder to the trash
\*------------------------------------------------------------------------------*/
void BmMailFolder::MoveToTrash() {
	::MoveToTrash( EntryRefPtr(), 1);
}
