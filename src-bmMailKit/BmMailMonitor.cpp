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
#include <MessageQueue.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>

#include <deque>
#include <map>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailMonitor.h"
#include "BmMailRef.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

using std::deque;
using std::map;
using std::pair;

/********************************************************************************\
	BmMailMonitorWorker
\********************************************************************************/
class BmMailMonitorWorker {

public:
	BmMailMonitorWorker();

	void Run();
	void Quit();
	//
	bool IsIdle(uint32 msecs);
	//
	void AddMessage(BMessage* msg);
	//
	void CacheRefToFolder( node_ref& nref, const BmString& fKey);

private:
	//	native methods:
	void MessageLoop();
	void MessageReceived( BMessage* msg);
	//
	void HandleMailMonitorMsg( BMessage* msg);
	void HandleQueryUpdateMsg( BMessage* msg);
	//
	static int32 ThreadEntry(void* data);

	void EntryCreated( BmMailFolder* parent, node_ref& nref,
							 entry_ref& eref, struct stat& st);
	void EntryRemoved( BmMailFolder* parent, node_ref& nref);
	void EntryMoved( BmMailFolder* parent, node_ref& nref,
						  entry_ref& eref, struct stat& st,
						  BmMailFolder* oldParent, entry_ref& erefFrom);
	void EntryChanged( node_ref& nref);

	// When trying to handle B_ATTR_CHANGED events for a mail-ref whose
	// ref-list isn't loaded, the given info isn't enough to find out the 
	// folder this mail-ref lives in. In order to remedy this, we cache
	// mail-ref -> folder entries when we update an attribute ourselves
	// (currently MAIL:status and MAIL:classification):
	struct FolderInfo {
		FolderInfo( BmString fk) : folderKey(fk), usedCount(1) {}
		BmString folderKey;
		int usedCount;
	};
	typedef map<BmString, FolderInfo> CachedRefToFolderMap;
	CachedRefToFolderMap mCachedRefToFolderMap;

	// deque for incoming node-monitor messages:
	typedef deque<BMessage*> MessageList;
	MessageList mMessageList;

	BLocker mLocker;
	bool mShouldRun;
	thread_id mThreadId;
	uint32 mCounter;
	uint32 mIdleTimeInMSecs;

	// Hide copy-constructor and assignment:
	BmMailMonitorWorker( const BmMailMonitorWorker&);
	BmMailMonitorWorker operator=( const BmMailMonitorWorker&);
};

/*------------------------------------------------------------------------------*\
	BmMailMonitorWorker()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailMonitorWorker::BmMailMonitorWorker()
	:	mLocker("MailMonitorWorkerLock")
	,	mShouldRun(false)
	,	mThreadId(-1)
	,	mCounter(0)
	,	mIdleTimeInMSecs(0)
{
}

/*------------------------------------------------------------------------------*\
	Run()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::Run()
{
	mShouldRun = true;
	// start new thread for worker:
	BmString tname( "MailMonitorWorker");
	mThreadId = spawn_thread( BmMailMonitorWorker::ThreadEntry, 
									  tname.String(), B_NORMAL_PRIORITY, this);
	if (mThreadId < 0)
		throw BM_runtime_error("MailMonitor::Run(): Could not spawn thread");
	resume_thread( mThreadId);
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::Quit()
{
	mShouldRun = false;
	status_t exitVal;
	wait_for_thread(mThreadId, &exitVal);
}

/*------------------------------------------------------------------------------*\
	ThreadEntry()
		-	
\*------------------------------------------------------------------------------*/
int32 BmMailMonitorWorker::ThreadEntry(void* data)
{
	BmMailMonitorWorker* worker = static_cast<BmMailMonitorWorker*>(data);
	if (worker)
		worker->MessageLoop();
	return B_OK;
}	

/*------------------------------------------------------------------------------*\
	MessageLoop()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::MessageLoop()
{
	const int32 snoozeMSecs = 50;
	BMessage* msg = NULL;
	while(mShouldRun) {
		if (mLocker.Lock()) {
			if (!mMessageList.empty()) {
				mIdleTimeInMSecs = 0;
				msg = mMessageList.front();
				mMessageList.pop_front();
			} else
				msg = NULL;
			mLocker.Unlock();
		}
		if (msg) {
			MessageReceived(msg);
			delete msg;
		} else {
			snooze(snoozeMSecs*1000);
			mIdleTimeInMSecs += snoozeMSecs;
		}
	}
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_NODE_MONITOR: {
				if (TheMailFolderList)
					HandleMailMonitorMsg( msg);
				break;
			}
			case B_QUERY_UPDATE: {
				if (TheMailFolderList)
					HandleQueryUpdateMsg( msg);
				break;
			}
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailMonitorWorker: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	AddMessage( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::AddMessage( BMessage* msg) {
	if (mLocker.Lock()) {
		mMessageList.push_back(msg);
		mLocker.Unlock();
	}
}

/*------------------------------------------------------------------------------*\
	IsIdle()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailMonitorWorker::IsIdle(uint32 msecs) {
	bool res = false;
	if (mLocker.LockWithTimeout(20*1000) == B_OK) {
		// Mailmonitor is idle if the message list is empty and if
		// it has been so for the given amount of microseconds:
		res = mMessageList.empty() && mIdleTimeInMSecs > msecs;
		mLocker.Unlock();
	}
	return res;
}

/*------------------------------------------------------------------------------*\
	HandleMailMonitorMsg()
		-	N.B.: we do not currently support mailboxes that spread across devices
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::HandleMailMonitorMsg( BMessage* msg) {
	BM_LOG2( BM_LogMailTracking, 
				BmString("MailMonitorMessage nr.") << ++mCounter << " received.");
	try {
		int32 opcode = msg->FindInt32( "opcode");
		switch( opcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
			case B_ENTRY_MOVED: {
				status_t err;
				node_ref pnref;
				BmRef<BmMailFolder> parent;
				const char *name;
				struct stat st;
				entry_ref eref;
				node_ref nref;
				BmRef<BmMailFolder> folder;
			
				const char* directory 
					= (opcode == B_ENTRY_MOVED)
						? "to directory" 
						: "directory";
				if ((err = msg->FindInt64( directory, &eref.directory)) != B_OK)
					BM_THROW_RUNTIME( BmString("Field '") << directory 
												<< "' not found in msg !?!");
				if ((err = msg->FindInt32( "device", &eref.device)) != B_OK)
					BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				pnref.device = nref.device = eref.device;
				pnref.node = eref.directory;
				if ((err = msg->FindInt64( "node", &nref.node)) != B_OK)
					BM_THROW_RUNTIME( "Field 'node' not found in msg !?!");
				if (opcode != B_ENTRY_REMOVED) {
					BNode aNode;
					if ((err = msg->FindString( "name", &name)) != B_OK)
						BM_THROW_RUNTIME( "Field 'name' not found in msg !?!");
					eref.set_name( name);
					if ((err = aNode.SetTo( &eref)) != B_OK) {
						BM_LOG( 
							BM_LogMailTracking, 
							BmString( "Couldn't create node for parent-node <") 
						  		<< eref.directory << "> and name <" << eref.name 
						  		<< "> \n\nError:" << strerror(err)
						);
						return;
					}
					if ((err = aNode.GetStat( &st)) != B_OK) {
						BM_LOG( 
							BM_LogMailTracking, 
							BmString("Couldn't get stats for node --- parent-node <")
								<< eref.directory << "> and name <" << eref.name 
								<< "> \n\nError:" << strerror(err)
						);
						return;
					}
				}
				if (opcode == B_ENTRY_CREATED) {
					parent = dynamic_cast<BmMailFolder*>( 
						TheMailFolderList->FindItemByKey( BM_REFKEY( pnref)).Get()
					);
					EntryCreated( parent.Get(), nref, eref, st);
				} else if (opcode == B_ENTRY_REMOVED) {
					parent = dynamic_cast<BmMailFolder*>( 
						TheMailFolderList->FindItemByKey( BM_REFKEY( pnref)).Get()
					);
					EntryRemoved( parent.Get(), nref);
				} else if (opcode == B_ENTRY_MOVED) {
					entry_ref erefFrom;
					node_ref opnref;
					BmRef<BmMailFolder> oldParent;
					if ((err = msg->FindInt64( 
						"from directory", &erefFrom.directory
					)) != B_OK)
						BM_THROW_RUNTIME( 
							"Field 'directory' not found in msg !?!"
						);
					if ((err = msg->FindInt32( 
						"device", &erefFrom.device
					)) != B_OK)
						BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
					if ((err = msg->FindString( "name", &name)) != B_OK)
						BM_THROW_RUNTIME( "Field 'name' not found in msg !?!");
					erefFrom.set_name( name);
					opnref.node = erefFrom.directory;
					opnref.device = erefFrom.device;
					oldParent = dynamic_cast<BmMailFolder*>( 
						TheMailFolderList->FindItemByKey( BM_REFKEY( opnref)).Get()
					);
					parent = dynamic_cast<BmMailFolder*>( 
						TheMailFolderList->FindItemByKey( BM_REFKEY( pnref)).Get()
					);
					EntryMoved( parent.Get(), nref, eref, st, 
									oldParent.Get(), erefFrom);
				}
				break;
			}
			case B_STAT_CHANGED:
			case B_ATTR_CHANGED: {
				status_t err;
				node_ref nref;
				if ((err = msg->FindInt64( "node", &nref.node)) != B_OK)
					BM_THROW_RUNTIME( "Field 'node' not found in msg !?!");
				if ((err = msg->FindInt32( "device", &nref.device)) != B_OK)
					BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				EntryChanged( nref);
				break;
			}
		}
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	EntryCreated()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::EntryCreated( BmMailFolder* parent, node_ref& nref,
													 entry_ref& eref, struct stat& st) {
	if (!parent)
		throw BM_runtime_error(
			BmString("Folder with inode <") << eref.directory 
				<< "> is unknown."
		);
	if (S_ISDIR(st.st_mode)) {
		// a new mail-folder has been created, we add 
		// it to our list:
		BM_LOG2( BM_LogMailTracking, 
					BmString("New mail-folder <") << eref.name 
						<< "," << nref.node << "> detected.");
		TheMailFolderList->AddMailFolder( eref, nref.node, parent, st.st_mtime);
	} else {
		// a new mail has been created, we add it to the 
		// parent folder:
		BM_LOG2( BM_LogMailTracking, 
					BmString("New mail <") << eref.name 
						<< "," << nref.node << "> detected.");
		parent->AddMailRef( eref, st);
	}
}

/*------------------------------------------------------------------------------*\
	EntryRemoved()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::EntryRemoved( BmMailFolder* parent, node_ref& nref) {
	BmRef<BmMailFolder> folder;
	// we have no entry that could tell us what kind of item 
	// was removed, so we have to find out by ourselves:
	if (parent) {
		// a folder has been deleted, we remove it from our list:
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( "MailMonitor::EntryRemoved(): Unable to get lock");
		folder = dynamic_cast< BmMailFolder*>( 
			parent->FindItemByKey( BM_REFKEY( nref))
		);
	} else
		folder = NULL;
	if (folder) {
		// a folder has been deleted, we remove it from our list:
		BM_LOG2( BM_LogMailTracking, 
					BmString("Removal of mail-folder <") 
						<< nref.node << "> detected.");
		BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( "MailMonitor::EntryRemoved(): Unable to get lock");
		// adjust special-mail-count accordingly...
		int32 specialMailCount = folder->SpecialMailCount() 
									+ folder->SpecialMailCountForSubfolders();
		parent->BumpSpecialMailCountForSubfolders( -1*specialMailCount);
		// ...and remove folder from list:
		TheMailFolderList->RemoveItemFromList( folder.Get());
	} else {
		// a mail has been deleted, we remove it from the 
		// parent-folder:
		BM_LOG2( BM_LogMailTracking, 
					BmString("Removal of mail <") << nref.node 
						<< "> detected.");
		if (parent)
			parent->RemoveMailRef( nref);
	}
}

/*------------------------------------------------------------------------------*\
	EntryMoved()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::EntryMoved( BmMailFolder* parent, node_ref& nref,
												  entry_ref& eref, struct stat& st,
												  BmMailFolder* oldParent, 
												  entry_ref& erefFrom) {
	// Since we track all mail-folders, we get two entry-moved messages from
	// the node-monitor if a mail is moved inside the mailbox. We only want
	// to handle the event once, so we filter out double messages here:
	static BmMailFolder* last_parent;
	static BmMailFolder* last_oldParent;
	static entry_ref last_eref;
	static entry_ref last_erefFrom;
	if (last_parent == parent && last_oldParent == oldParent
	&& last_eref == eref && last_erefFrom == erefFrom) {
		BM_LOG2( BM_LogMailTracking, 
					BmString("Second move-event of mail/folder <") << eref.name 
						<< "," << nref.node << "> dropped.");
		return;
	}
	last_parent = parent;
	last_oldParent = oldParent;
	last_eref = eref;
	last_erefFrom = erefFrom;
	// ok, now do actual processing:
	if (S_ISDIR(st.st_mode)) {
		// it's a mail-folder, we check for type of change:
		BmRef<BmMailFolder> folder;
		folder = dynamic_cast<BmMailFolder*>( 
			TheMailFolderList->FindItemByKey( BM_REFKEY( nref)).Get()
		);
		if (erefFrom.directory == eref.directory) {
			// rename only, we take the short path:
			BM_LOG2( BM_LogMailTracking, 
						BmString("Rename of mail-folder <") 
							<< eref.name << "," << nref.node 
							<< "> detected.");
			folder->UpdateName( eref);
		} else {
			BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
			if (!lock.IsLocked())
				BM_THROW_RUNTIME( "MailMonitor::EntryMoved(): Unable to get lock");
			// the folder has really changed position within the filesystem-tree:
			if (oldParent && folder && folder->Parent() != oldParent)
				// folder not there anymore (e.g. 2nd msg for move)
				return;	
			if (!folder) {
				// folder was unknown before, probably because it has been moved 
				// from a place outside of the mailbox-substructure inside it.
				// We create the new folder...
				folder = TheMailFolderList->AddMailFolder( 
					eref, 
					nref.node, 
					parent, 
					st.st_mtime
				);
				// ...and scan for potential sub-folders:
				TheMailFolderList->InitializeSubFolders( folder.Get(), 1);
				BM_LOG2( BM_LogMailTracking, 
							BmString("Move of mail-folder <") 
								<< eref.name << "," << nref.node 
								<< "> detected.\nFrom: " 
								<< (oldParent
										? oldParent->Key()
										: BmString("<outside>")) 
								<<" to: " << 
									(parent 
										? parent->Key()
										: BmString( "<outside>")));
			} else {
				// folder exists in our structure
				BM_LOG2( BM_LogMailTracking, 
							BmString("Move of mail-folder <") 
								<< eref.name << "," << nref.node 
								<< "> detected.\nFrom: "
								<< (oldParent
										? oldParent->Key()
										: BmString( "<outside>"))
								<< " to: "
								<< (parent
										? parent->Key()
										: BmString( "<outside>")));
				// adjust special-mail-count accordingly...
				int32 specialMailCount 
					= folder->SpecialMailCount() 
						+ folder->SpecialMailCountForSubfolders();
				if (oldParent)
					oldParent->BumpSpecialMailCountForSubfolders( -1*specialMailCount);
				// ...and remove from folder-list:
				TheMailFolderList->RemoveItemFromList( folder.Get());
				if (parent) {
					// new position is still underneath our mailbox,
					// so we re-add the folder:
					folder->EntryRef( eref);
					TheMailFolderList->AddItemToList( folder.Get(), parent);
					// and adjust the new-mail-count accordingly:
					parent->BumpSpecialMailCountForSubfolders( specialMailCount);
				}
			}
		}
	} else {
		// it's a mail-ref, we remove it from old parent and add
		// it to its new parent:
		BM_LOG2( BM_LogMailTracking, 
					BmString("Move of mail <") << eref.name 
						<< "," << nref.node << "> detected.");
		if (oldParent)
			oldParent->RemoveMailRef( nref);
		if (parent)
			parent->AddMailRef( eref, st);
	}
}


/*------------------------------------------------------------------------------*\
	EntryChanged()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::EntryChanged( node_ref& nref) {
	BM_LOG2( BM_LogMailTracking, 
				BmString("Change of item with node <") 
					<< nref.node << "> detected...");
	// B_ATTR_CHANGED messages only carry the node-ref of the file, from
	// which we can't deduce the corresponding mail-folder. This is bad!
	// In order to remedy the problem somewhat, we use a two-fold approach
	// to fetch the mailref that corresponds to the given node-ref:
	// - whenever Beam itself changes an attribute of a node-ref, it
	//   explicitly puts the parent folder of the mail-ref into a specific
	//   map. So we first search this map for a corresponding entry.
	// - if there is no corresponding entry in the map (which is the case
	//   when the change has been triggered by another program) we try
	//   to find the mail-ref by searching the complete mail-folder-hierarchy.
	//	  This isn't reliable, as the mail-ref may not be loaded, but it 
	//	  is better than nothing.
	BmString key( BM_REFKEY( nref));
	CachedRefToFolderMap::iterator pos = mCachedRefToFolderMap.find( key);
	if (pos != mCachedRefToFolderMap.end()) {
		// mail-ref has a cached entry, we use the specified folder:
		BmRef<BmListModelItem> folderItem 
			= TheMailFolderList->FindItemByKey( pos->second.folderKey);
		BmRef<BmMailFolder> folder 
			= dynamic_cast< BmMailFolder*>( folderItem.Get());
		if (folder)
			folder->UpdateMailRef( nref);
		if (pos->second.usedCount > 1)
			pos->second.usedCount--;
		else
			mCachedRefToFolderMap.erase( pos);
	} else {
		// need to search complete folder-hierarchy for mail-ref:
		BmRef<BmMailRef> ref = TheMailFolderList->FindMailRefByKey( nref);
		if (ref) {
			BM_LOG2( BM_LogMailTracking, 
						"...corresponding ref was found in loaded ref-lists.");
			ref->ResyncFromDisk();
		} else {
			BM_LOG2( BM_LogMailTracking, 
						"...corresponding ref wasn't found in loaded ref-lists.");
		}
	}
}

/*------------------------------------------------------------------------------*\
	CacheRefToFolder()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::CacheRefToFolder( node_ref& nref, 
														  const BmString& fKey) {
	BmAutolockCheckGlobal lock( &mLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( "MailMonitor::CacheRefToFolder(): Unable to get lock");
	BmString key( BM_REFKEY( nref));
	CachedRefToFolderMap::iterator pos = mCachedRefToFolderMap.find( key);
	if (pos != mCachedRefToFolderMap.end())
		pos->second.usedCount++;
	else {
		FolderInfo fInfo( fKey);
		mCachedRefToFolderMap.insert( pair<const BmString, FolderInfo>( key, fInfo));
	}
}

/*------------------------------------------------------------------------------*\
	HandleQueryUpdateMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitorWorker::HandleQueryUpdateMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	status_t err;
	node_ref pnref;
	node_ref nref;
	BM_LOG2( BM_LogMailTracking, 
				BmString("QueryUpdateMessage nr.") << ++mCounter << " received.");
	try {
		switch( opcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED: {
				if ((err = msg->FindInt64( "directory", &pnref.node)) != B_OK)
					BM_THROW_RUNTIME( "Field 'directory' not found in msg !?!");
				if ((err = msg->FindInt32( "device", &nref.device)) != B_OK)
					BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				if ((err = msg->FindInt64( "node", &nref.node)) != B_OK)
					BM_THROW_RUNTIME( "Field 'node' not found in msg !?!");
				pnref.device = nref.device;
				{	// scope for lock
					BmAutolockCheckGlobal lock( TheMailFolderList->ModelLocker());
					if (!lock.IsLocked())
						BM_THROW_RUNTIME( 
							"HandleMailMonitorMsg(): Unable to get lock"
						);
					if (opcode == B_ENTRY_CREATED)
						TheMailFolderList->AddSpecialFlag( pnref, nref);
					else		// opcode == B_ENTRY_REMOVED
						TheMailFolderList->RemoveSpecialFlag( pnref, nref);
				}
				break;
			}
		}
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
	}
}

/********************************************************************************\
	BmMailMonitor
\********************************************************************************/

BmMailMonitor* BmMailMonitor::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmMailMonitor* BmMailMonitor::CreateInstance() {
	if (!theInstance)
		theInstance = new BmMailMonitor();
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	BmMailMonitor()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailMonitor::BmMailMonitor()
	:	BLooper("MailMonitor", B_DISPLAY_PRIORITY, 500)
{
	mWorker = new BmMailMonitorWorker;
	mWorker->Run();
	Run();
}

/*------------------------------------------------------------------------------*\
	BmMailMonitor()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailMonitor::~BmMailMonitor()
{
	delete mWorker;
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::MessageReceived( BMessage* msg) {
	switch( msg->what) {
		case B_NODE_MONITOR:
		case B_QUERY_UPDATE: {
			DetachCurrentMessage();
			mWorker->AddMessage(msg);
			break;
		}
		default:
			inherited::MessageReceived( msg);
	}
}

/*------------------------------------------------------------------------------*\
	CacheRefToFolder()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::CacheRefToFolder( node_ref& nref, const BmString& fKey) {
	mWorker->CacheRefToFolder(nref, fKey);
}

/*------------------------------------------------------------------------------*\
	IsIdle()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailMonitor::IsIdle(uint32 msecs) {
	return MessageQueue()->IsEmpty() && mWorker->IsIdle(msecs);
}

/*------------------------------------------------------------------------------*\
	Quit()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::Quit() {
	mWorker->Quit();
	inherited::Quit();
}
