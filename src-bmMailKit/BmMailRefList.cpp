/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Query.h>

#include "regexx.hh"
using namespace regexx;

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"
#include "BmMailRefFilter.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmRosterBase.h"
#include "BmUtil.h"

//******************************************************************************
// #pragma mark -	BmMailRefList
//******************************************************************************
const int16 BmMailRefList::nArchiveVersion = 3;

const char* const BmMailRefList::MSG_FILTER_ARCHIVE = "bm:fila";

/*------------------------------------------------------------------------------*\
	BmMailRefList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRefList::BmMailRefList( BmMailFolder* folder)
	:	BmListModel( BmString("MailRefList_") << folder->Key() 
							<< " (" << folder->Name()<<")", BM_LogMailTracking)
	,	mFolder( folder)
	,	mNeedsCacheUpdate( false)
{
	if (folder) {
		mSettingsFileName = BmString("folder_")
									<< folder->Key()
									<< " ("
									<< folder->Name()
									<< ")";
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailRefList()
		-	d'tor
		-	the mailref-list is stored (the cache is written) before it is deleted
\*------------------------------------------------------------------------------*/
BmMailRefList::~BmMailRefList() {
	StoreAndCleanup();
}

/*------------------------------------------------------------------------------*\
	MarkCacheAsDirty()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::MarkCacheAsDirty() { 
	mNeedsCacheUpdate = true;
	Cleanup();
}

/*------------------------------------------------------------------------------*\
	StoreAndCleanup()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::StoreAndCleanup() { 
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":StoreAndCleanup(): Unable to get lock"
		);
	if (ThePrefs->GetBool("CacheRefsOnDisk") && mNeedsStore 
	&& !mNeedsCacheUpdate)
		Store();
	Cleanup();
}

/*------------------------------------------------------------------------------*\
	IsJobCompleted()
		-	checks if this job has been completed
\*------------------------------------------------------------------------------*/
bool BmMailRefList::IsJobCompleted() const
{
	return !mNeedsCacheUpdate && inherited::IsJobCompleted();
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BmString BmMailRefList::SettingsFileName() {
	BDirectory* mailCacheDir = BeamRoster->MailCacheFolder();
	BEntry entry;
	if (!mailCacheDir || mailCacheDir->GetEntry( &entry)!=B_OK)
		return BmString("");
	BPath path;
	if (entry.GetPath( &path) != B_OK)
		return BmString("");
	return BmString( path.Path()) << "/" << mSettingsFileName;
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores List inside Settings-dir
\*------------------------------------------------------------------------------*/
bool BmMailRefList::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t ret;
	if (mInitCheck != B_OK)
		return true;
	try {
		BmAutolockCheckGlobal lock( ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":Store(): Unable to get lock"
			);
		BMallocIO memIO;
		memIO.SetBlockSize( 1200 * MAX( size(), 1));
			// acquire enough mem for complete archive, avoids realloc()
	
		BmString filename = SettingsFileName();
		if ((ret = cacheFile.SetTo( 
			filename.String(), 
			B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE
		)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not create settings-file\n\t<") 
										<< filename << ">\n\n Result: " << strerror(ret));
		ret = archive.AddInt16( MSG_VERSION, nArchiveVersion);
		if (ret == B_OK && mFilter) {
			BMessage filterArchive;
			ret = mFilter->Archive(&filterArchive);
			if (ret == B_OK)
				ret = archive.AddMessage(MSG_FILTER_ARCHIVE, &filterArchive);
		}
		if (ret == B_OK) {
			ret = archive.AddInt32( BmListModelItem::MSG_NUMCHILDREN, size())
					| archive.Flatten( &memIO);
		}
		if (ret == B_OK) {
			BM_LOG( BM_LogModelController, 
					  BmString("ListModel <") << ModelName() 
					  		<< "> begins to archive...");
			BmModelItemMap::const_iterator iter;
			for( iter = begin(); iter != end() && ret == B_OK; ++iter) {
				BMessage msg;
				ret = iter->second->Archive( &msg, true);
				if (ret == B_OK)
					ret = msg.Flatten( &memIO);
			}
			BM_LOG( BM_LogModelController, 
					  BmString("ListModel <") << ModelName() 
					  		<< "> finished with archive, writing to file...");
			cacheFile.Write( memIO.Buffer(), memIO.BufferLength());
			BM_LOG( BM_LogModelController, 
					  BmString("ListModel <") << ModelName() 
					  		<< "> finished with writing to file");
		}
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefList::StartJob() {
	// try to open cache file...
	status_t err;
	BFile cacheFile;
	
	BmRef<BmMailFolder> folder( mFolder.Get());	
							// hold a ref on the corresponding folder while 
							// we do the job
	if (!folder)
		return false;
		
	if (InitCheck() == B_OK && !mNeedsCacheUpdate)
		return true;
	
	Freeze();									// we shut up for better performance
	try {
		bool cacheFileUpToDate = false;
		BmString filename = SettingsFileName();
		BMessage msg;
		{ // scope for lock
			BmAutolockCheckGlobal lock( ModelLocker());
			if (!lock.IsLocked())
				BM_THROW_RUNTIME( 
					ModelNameNC() << ":AddMailRef(): Unable to get lock"
				);
		
			// flush any pending to-be-stored actions
			mStoredActionManager.Flush();
	
			if (ThePrefs->GetBool("CacheRefsOnDisk")
			&& (err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) != B_OK) {
				// cache-file not found, but we have changed names of cache-files
				// in Nov 2003 (again!), so we check if a cache-file according to 
				// the old name exists (and rename it to match our new regulations):
				BmString oldFilename = BmString("folder_")
												<< folder->Key()
												<< "_" << ThePrefs->MailboxVolume.Device()
												<< " (" << folder->Name() << ")";
				BEntry entry( BeamRoster->MailCacheFolder(), oldFilename.String());
				time_t mtime;
				err = (entry.InitCheck() 
						|| entry.GetModificationTime( &mtime) 
						|| entry.Rename( filename.String()) 
						|| entry.SetModificationTime( mtime));
			}
			if (ThePrefs->GetBool("CacheRefsOnDisk")
			&& (err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
				time_t mtime;
				if ((err = cacheFile.GetModificationTime( &mtime)) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not get mtime \nfor mail-folder <") << Name() 
							<< "> \n\nError:" << strerror(err)
					);
				if (!mNeedsCacheUpdate && !folder->CheckIfModifiedSince( mtime)) {
					// archive up-to-date, but is it the correct format-version?
					msg.Unflatten( &cacheFile);
					int16 version;
					if (msg.FindInt16( MSG_VERSION, &version) == B_OK 
					&& version == nArchiveVersion)
						cacheFileUpToDate = true;
				}
			}
		}
		if (cacheFileUpToDate) {
			// ...ok, cache-file should contain up-to-date info, 
			// we fetch our data from it:
#ifdef __HAIKU__
			// On haiku, this is considerably faster than unflattening from a file.
			// TODO: find out why haiku is much slower than R5 in this!
			off_t sz = 0;
			if ((err = cacheFile.GetSize(&sz)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("couldn't get size for cache-file <") << filename 
						<< "> \n\nError:" << strerror(err)
				);
			sz -= cacheFile.Position();
			std::auto_ptr<char> buf(new char [sz]);
			if ((err = cacheFile.Read(buf.get(), size_t(sz))) < B_OK)
				BM_THROW_RUNTIME( 
					BmString("couldn't read from cache-file <") << filename 
						<< "> \n\nError:" << strerror(err)
				);
			if (err < sz)
				BM_THROW_RUNTIME( 
					BmString("couldn't read ") << sz << " bytes from cache-file <"
						<< filename << ">, read only " << err << " bytes"
				);
			BMemoryIO memIO(buf.get(), size_t(sz));
			InstantiateItemsFromStream( &memIO, &msg);
#else
			InstantiateItemsFromStream( &cacheFile, &msg);
#endif
		} else {
			// ...caching disabled or no cache file found or update 
			// required/requested, we fetch the existing mails from disk...
			InitializeItems();
		}

	} catch (BM_error &e) {
		BM_SHOWERR( e.what());
	}
	Thaw();
	return InitCheck() == B_OK;
}

/*------------------------------------------------------------------------------*\
	AddMailRef()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailRefList::AddMailRef( entry_ref& eref, struct stat& st) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":AddMailRef(): Unable to get lock"
		);
	BmRef<BmMailRef> newMailRef( BmMailRef::CreateInstance( eref, &st));
	if (mInitCheck == B_OK) {
		// ref-list has been read from disk,  so we can add to it:
		if (AddItemToList( newMailRef.Get()))
			return newMailRef;
	} else if (newMailRef) {
		// ref-list has not been read yet, we append info about the added
		// item to the cache:
		BMessage action;
		newMailRef->Archive( &action);
		action.AddInt32( BmMailRef::MSG_OPCODE, B_ENTRY_CREATED);
		BM_LOG( BM_LogMailTracking, 
				  BmString("Storing created-action for ref ") 
				  	<< newMailRef->Key());
		if (StoreAction(&action)) {
			BmRef<BmMailFolder> folder( mFolder.Get());
				// hold a ref on the corresponding folder while we use it
			if (folder)
				folder->BumpMailCount(1);
			return newMailRef;
		}
	}
	return NULL;
}

/*------------------------------------------------------------------------------*\
	RemoveMailRef()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmListModelItem> BmMailRefList::RemoveMailRef( const BmString& key) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":RemoveMailRef(): Unable to get lock"
		);
	BmRef<BmListModelItem> removedRef;
	if (mInitCheck == B_OK) {
		// ref-list has been read from disk, so we can remove from it:
		removedRef = RemoveItemByKey( key);
	} else {
		// ref-list has not been read yet, we append info about the removed
		// item to the cache:
		BMessage action;
		action.AddInt32( BmMailRef::MSG_OPCODE, B_ENTRY_REMOVED);
		action.AddString( MSG_ITEMKEY, key.String());
		BM_LOG( BM_LogMailTracking, 
				  BmString("Storing removed-action for ref ") << key);
		if (StoreAction(&action)) {
			BmRef<BmMailFolder> folder( mFolder.Get());
				// hold a ref on the corresponding folder while we use it
			if (folder)
				folder->BumpMailCount(-1);
		}
	}
	return removedRef;
}

/*------------------------------------------------------------------------------*\
	UpdateMailRef()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::UpdateMailRef( const BmString& key) {
	BmAutolockCheckGlobal lock( ModelLocker());
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":UpdateMailRef(): Unable to get lock"
		);
	if (mInitCheck == B_OK) {
		// ref-list has been read from disk, so we can update (an item of) it:
		BmRef<BmListModelItem> item = FindItemByKey( key);
		BmMailRef* ref = dynamic_cast< BmMailRef*>( item.Get());
		if (ref)
			ref->ResyncFromDisk();
	} else {
		// ref-list has not been read yet, we append info about the changed
		// item to the cache:
		BMessage action;
		action.AddInt32( BmMailRef::MSG_OPCODE, B_ATTR_CHANGED);
		action.AddString( MSG_ITEMKEY, key.String());
		BM_LOG( BM_LogMailTracking, 
				  BmString("Storing update-action for ref ") << key);
		StoreAction(&action);
	}
}

/*------------------------------------------------------------------------------*\
	InitializeItems()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InitializeItems() {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count;
	bool stopped = false;

	BmRef<BmMailFolder> folder( mFolder.Get());	
							// hold a ref on the corresponding folder while we use it
	BM_LOG( BM_LogMailTracking, 
			  BmString("Start of InitializeMailRefs() for folder ") 
			  		<< folder->Name());

	// we create a BDirectory from the given mail-folder...
	mailDir.SetTo( folder->EntryRefPtr());

	BmRef<BmMailRef> newRef;
	// ...and scan through all its entries for mails:
	while (!stopped 
	&& (count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (!stopped && count-- > 0) {
			if (!(!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))) {
				if ((err=mailDir.GetStatFor( dent->d_name, &st)) != B_OK)
					BM_THROW_RUNTIME( 
						BmString("Could not get stat-info for \nmail-file <") 
							<< dent->d_name << "> \n\nError:" << strerror(err)
					);
				if (S_ISREG( st.st_mode)) {
					// we have found a new mail, so we add it to our list:
					BM_LOG3( BM_LogMailTracking, 
								BmString("Mail <") << dent->d_name << "," << dent->d_ino 
									<< "> found ");
					eref.device = dent->d_pdev;
					eref.directory = dent->d_pino;
					eref.set_name( dent->d_name);
					newRef = BmMailRef::CreateInstance( eref, &st);
					AddItemToList( newRef.Get());
				}
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);

			if (!ShouldContinue()) {
				stopped = true;
				BM_LOG2( BM_LogMailTracking, 
							BmString("InitializeMailRefs() stopped for folder ") 
								<< folder->Name());
			}
		}
	}
	BM_LOG( BM_LogMailTracking, 
			  BmString("End of InitializeMailRefs() for folder ") 
			  		<< folder->Name());
	if (stopped) {
		Cleanup();
	} else {
		BmAutolockCheckGlobal lock( ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
		folder->MailCount( ValidCount());
		mNeedsCacheUpdate = false;
		mNeedsStore = true;
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	InstantiateItemsFromStream( dataIO, headerMsg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InstantiateItemsFromStream( BDataIO* dataIO, BMessage* headerMsg) {
	if (!headerMsg)
		return;

	BMessage filterArchive;
	if (headerMsg->FindMessage(MSG_FILTER_ARCHIVE, &filterArchive) == B_OK) {
		BmMailRefFilter* filter = new BmMailRefFilter(&filterArchive);
		SetFilter(filter);
	}

	int32 numChildren 
		= FindMsgInt32( headerMsg, BmListModelItem::MSG_NUMCHILDREN);
	BmRef<BmMailFolder> folder( mFolder.Get());	
							// hold a ref on the corresponding folder while we use it
	BM_LOG( BM_LogMailTracking, 
			  BmString("Start of InstantiateMailRefs() for folder ") 
			  		<< folder->Name());
	bool stopped = false;
	BMessage msg;
	for( int i=0; !stopped && i<numChildren; ++i) {
		if (msg.Unflatten( dataIO) != B_OK) {
			BM_SHOWERR( "Could not read all messages from cache file. "
							"Please recreate cache.");
			break;
		}
		BmRef<BmMailRef> newRef( BmMailRef::CreateInstance( &msg));
		BM_LOG3( BM_LogMailTracking, 
					BmString("MailRef <") << newRef->TrackerName() << "," 
						<< newRef->Key() << "> read");
		AddItemToList( newRef.Get());

		if (!ShouldContinue()) {
			stopped = true;
			BM_LOG2( BM_LogMailTracking, 
						BmString("InstantiateMailRefs() stopped for folder ") 
							<< folder->Name());
		}
	}
	{  // now lock the list, as we must avoid the race condition where the 
		// node monitor appends to the file while we fetch appended items 
		// from it
		BmAutolockCheckGlobal lock( ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( ModelNameNC() << ": Unable to get lock");
		bool needsStore = false;
		if (!stopped) {
			BM_LOG( BM_LogMailTracking, 
					  BmString("Fetching stored actions for folder ")
					  		<< folder->Name());
			if (RestoreAndExecuteActionsFrom(dataIO))
				needsStore = true;
		}
		BM_LOG( BM_LogMailTracking, 
				  BmString("End of InstantiateMailRefs() for folder ") 
				  		<< folder->Name());
		if (stopped) {
			Cleanup();
		} else {
			folder->MailCount( ValidCount());
			mNeedsCacheUpdate = false;
			mNeedsStore = needsStore;
				// overrule changes caused by reading the cache
			mInitCheck = B_OK;
		}
	}
}

/*------------------------------------------------------------------------------*\
	AddItemToList( item, parent)
		-	extends base-method with automatic updating of the corresponding 
			folder's mail-count 
\*------------------------------------------------------------------------------*/
bool BmMailRefList::AddItemToList( BmListModelItem* item, 
											  BmListModelItem* parent) {
	bool res = inherited::AddItemToList( item, parent);
	if (res && !Frozen()) {
		BmRef<BmMailFolder> folder( mFolder.Get());
			// hold a ref on the corresponding folder while we use it
		if (folder && item->IsValid())
			folder->BumpMailCount( 1);
	}
	return res;
}

/*------------------------------------------------------------------------------*\
	RemoveItemFromList( item)
		-	extends base-method with automatic updating of the corresponding 
			folder's mail-count 
\*------------------------------------------------------------------------------*/
void BmMailRefList::RemoveItemFromList( BmListModelItem* item) {
	inherited::RemoveItemFromList( item);
	if (!Frozen()) {
		BmRef<BmMailFolder> folder( mFolder.Get());
			// hold a ref on the corresponding folder while we use it
		if (folder && item->IsValid())
			folder->BumpMailCount( -1);
	}
}

/*------------------------------------------------------------------------------*\
	SetItemValidity( item, b)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::SetItemValidity( BmListModelItem* item, bool isValid) {
	if (!item || item->IsValid() == isValid)
		return;
	inherited::SetItemValidity( item, isValid);
	if (!Frozen()) {
		BmRef<BmMailFolder> folder( mFolder.Get());
			// hold a ref on the corresponding folder while we use it
		if (folder)
			folder->BumpMailCount( isValid ? 1 : -1);
	}
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	deletes DataModel if it has no more controllers and if the 
			list-caching is deactivated
\*------------------------------------------------------------------------------*/
void BmMailRefList::RemoveController( BmController* controller) {
	inherited::RemoveController( controller);
	if (!(ThePrefs->GetBool("CacheRefsInMem") || HasControllers())) {
		// cleanup ref-list in order to free memory:
		StoreAndCleanup();
	}
}

/*------------------------------------------------------------------------------*\
	ExecuteAction( action)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::ExecuteAction(BMessage* action) {
	if (action) {
		int32 op = action->FindInt32( BmMailRef::MSG_OPCODE);
		if (op == B_ENTRY_CREATED) {
			BmRef<BmMailRef> ref( BmMailRef::CreateInstance( action));
			if (ref) {
				BM_LOG2( BM_LogMailTracking, 
						   BmString("ExecuteAction(): "
						   	"Adding MailRef with key") << ref->Key());
				ref->ResyncFromDisk();
				AddItemToList( ref.Get());
			}
		} else if (op == B_ENTRY_REMOVED) {
			BmString key = action->FindString( MSG_ITEMKEY);
			BM_LOG2( BM_LogMailTracking, 
					   BmString("ExecuteAction(): "
					   	"Removing MailRef with key") << key);
			RemoveItemByKey( key);
		} else if (op == B_ATTR_CHANGED) {
			BmString key = action->FindString( MSG_ITEMKEY);
			BM_LOG2( BM_LogMailTracking, 
					   BmString("ExecuteAction(): "
					   	"Updating MailRef with key") << key);
			BmRef<BmListModelItem> item( FindItemByKey( key));
			BmMailRef* ref = dynamic_cast< BmMailRef*>( item.Get());
			if (ref)
				ref->ResyncFromDisk();
		}
	}
}
