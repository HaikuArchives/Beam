/*
	BmMailFolderList.cpp
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


#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <NodeInfo.h>
#include <NodeMonitor.h>
#include <Path.h>

#include "BmApp.h"
#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmMailRef.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmStorageUtil.h"
#include "BmUtil.h"

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
	,	mCounter( 0)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::MessageReceived( BMessage* msg) {
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
			default:
				inherited::MessageReceived( msg);
		}
	}
	catch( BM_error &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailMonitor: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleMailMonitorMsg()
		-	N.B.: we do not currently support mailboxes that spread across devices
\*------------------------------------------------------------------------------*/
void BmMailMonitor::HandleMailMonitorMsg( BMessage* msg) {
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
void BmMailMonitor::EntryCreated( BmMailFolder* parent, node_ref& nref,
											 entry_ref& eref, struct stat& st) {
	if (!parent)
		throw BM_runtime_error(
			BmString("Folder with inode <") << eref.directory 
				<< "> is unknown."
		);
	if (S_ISDIR(st.st_mode)) {
		BmAutolockCheckGlobal lock( TheMailFolderList->mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( "MailMonitor::EntryCreated(): Unable to get lock");
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
void BmMailMonitor::EntryRemoved( BmMailFolder* parent, node_ref& nref) {
	BmRef<BmMailFolder> folder;
	// we have no entry that could tell us what kind of item 
	// was removed, so we have to find out by ourselves:
	if (parent) {
		// a folder has been deleted, we remove it from our list:
		BmAutolockCheckGlobal lock( TheMailFolderList->mModelLocker);
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
		BmAutolockCheckGlobal lock( TheMailFolderList->mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( "MailMonitor::EntryRemoved(): Unable to get lock");
		// adjust new-mail-count accordingly...
		int32 newMailCount = folder->NewMailCount() 
									+ folder->NewMailCountForSubfolders();
		parent->BumpNewMailCountForSubfolders( -1*newMailCount);
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
void BmMailMonitor::EntryMoved( BmMailFolder* parent, node_ref& nref,
										  entry_ref& eref, struct stat& st,
										  BmMailFolder* oldParent, entry_ref& erefFrom) {
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
			BmAutolockCheckGlobal lock( TheMailFolderList->mModelLocker);
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
				TheMailFolderList->doInitializeMailFolders( folder.Get(), 1);
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
				// adjust new-mail-count accordingly...
				int32 newMailCount = folder->NewMailCount() 
											+ folder->NewMailCountForSubfolders();
				if (oldParent)
					oldParent->BumpNewMailCountForSubfolders( -1*newMailCount);
				// ...and remove from folder-list:
				TheMailFolderList->RemoveItemFromList( folder.Get());
				if (parent) {
					// new position is still underneath our mailbox,
					// so we re-add the folder:
					folder->EntryRef( eref);
					TheMailFolderList->AddItemToList( folder.Get(), parent);
					// and adjust the new-mail-count accordingly:
					parent->BumpNewMailCountForSubfolders( newMailCount);
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
void BmMailMonitor::EntryChanged( node_ref& nref) {
	BM_LOG2( BM_LogMailTracking, 
				BmString("Change of item with node <") 
					<< nref.node << "> detected...");
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

/*------------------------------------------------------------------------------*\
	HandleQueryUpdateMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::HandleQueryUpdateMsg( BMessage* msg) {
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
				if (opcode == B_ENTRY_CREATED) {
					BmAutolockCheckGlobal lock( TheMailFolderList->mModelLocker);
					if (!lock.IsLocked())
						BM_THROW_RUNTIME( 
							"HandleMailMonitorMsg(): Unable to get lock"
						);
					TheMailFolderList->AddNewFlag( pnref, nref);
				} else {							// opcode == B_ENTRY_REMOVED
					TheMailFolderList->RemoveNewFlag( pnref, nref);
				}
				break;
			}
		}
	} catch( BM_error &e) {
		BM_SHOWERR( e.what());
	}
}



/********************************************************************************\
	BmMailFolderList
\********************************************************************************/

BmRef< BmMailFolderList> BmMailFolderList::theInstance( NULL);

const char* const BmMailFolderList::MSG_MAILBOXMTIME = "bm:mboxmtime";
const int16 BmMailFolderList::nArchiveVersion = 2;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmMailFolderList* BmMailFolderList::CreateInstance() {
	if (!theInstance)
		theInstance = new BmMailFolderList();
	return theInstance.Get();
}

/*------------------------------------------------------------------------------*\
	BmMailFolderList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::BmMailFolderList()
	:	BmListModel( "MailFolderList")
	,	mTopFolder( NULL)
	,	mMailboxPathHasChanged( false)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailFolderList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::~BmMailFolderList() {
	theInstance = NULL;
}

/*------------------------------------------------------------------------------*\
	AddNewFlag()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddNewFlag( const node_ref& pnref, 
														  const node_ref& nref) {
#ifdef BM_REF_DEBUGGING
	BM_ASSERT( ModelLocker().IsLocked());
#endif
	BmRef<BmListModelItem> parentRef = FindItemByKey( BM_REFKEY( pnref));
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	mNewMailNodeMap[BM_REFKEY( nref)] = parent;
	if (parent)
		parent->BumpNewMailCount();
	return parent;
}

/*------------------------------------------------------------------------------*\
	RemoveNewFlag()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveNewFlag( const node_ref& pnref, 
												  const node_ref& nref) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":RemoveNewFlag(): Unable to get lock"
		);
	BmRef<BmListModelItem> parentRef = FindItemByKey( BM_REFKEY( pnref));
	mNewMailNodeMap.erase( BM_REFKEY( nref));
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	if (parent)
		parent->BumpNewMailCount( -1);
}

/*------------------------------------------------------------------------------*\
	SetFolderForNodeFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::SetFolderForNodeFlaggedNew( const node_ref& nref, 
																	BmMailFolder* folder) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":SetFolderForNodeFlaggedNew(): Unable to get lock"
		);
	BmString refKey( BM_REFKEY( nref));
	BmMailFolder* oldFolder = mNewMailNodeMap[refKey];
	if (oldFolder != folder) {
		mNewMailNodeMap[ refKey] = folder;
		if (oldFolder)
			oldFolder->BumpNewMailCount( -1);
		if (folder)
			folder->BumpNewMailCount();
	}
}

/*------------------------------------------------------------------------------*\
	GetFolderForNodeFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList
::GetFolderForNodeFlaggedNew( const node_ref& nref) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":GetFolderForNodeFlaggedNew(): Unable to get lock"
		);
	return mNewMailNodeMap[ BM_REFKEY( nref)];
}

/*------------------------------------------------------------------------------*\
	NodeIsFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::NodeIsFlaggedNew( const node_ref& nref) {
	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":NodeIsFlaggedNew(): Unable to get lock"
		);
	return mNewMailNodeMap.find( BM_REFKEY( nref)) != mNewMailNodeMap.end();
}

/*------------------------------------------------------------------------------*\
	AddMailFolder()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddMailFolder( entry_ref& eref, int64 node, 
															  BmMailFolder* parent, 
															  time_t mtime) {
	BmMailFolder* newFolder = new BmMailFolder( this, eref, node, parent, mtime);
	AddItemToList( newFolder, parent);
	return newFolder;
}

/*------------------------------------------------------------------------------*\
	FindMailRefByKey()
		-	
\*------------------------------------------------------------------------------*/
BmRef<BmMailRef> BmMailFolderList::FindMailRefByKey( const node_ref& nref) {
	BmRef<BmMailRef> ref;
	if (mTopFolder) {	
		BmString key = BM_REFKEY( nref);
		BmRef<BmListModelItem> item = mTopFolder->FindMailRefByKey( key);
		ref = dynamic_cast< BmMailRef*>( item.Get());
	}
	return ref;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::StartJob() {
	if (inherited::StartJob()) {
		if (!mNewMailQuery.IsLive())
			QueryForNewMails();
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	InitializeItems()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InitializeItems() {
	BDirectory mailDir;
	BEntry entry;
	entry_ref eref;
	node_ref nref;
	status_t err;
	BmString mailDirName( ThePrefs->GetString("MailboxPath"));
	time_t mtime;

	BM_LOG( BM_LogMailTracking, "Start of initFolders");

	mailDir.SetTo( mailDirName.String());
	if ((err = mailDir.GetModificationTime( &mtime)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get mtime \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = mailDir.GetEntry( &entry)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get entry \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = entry.GetRef( &eref)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get entry-ref \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));
	if ((err = mailDir.GetNodeRef( &nref)) != B_OK)
		BM_THROW_RUNTIME( BmString("Could not get node-ref \nfor mailbox-dir <") 
									<< mailDirName << "> \n\nError:" << strerror(err));

	{	// scope for autolocker
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":InitializeMailFolders(): Unable to get lock"
			);
		BM_LOG3( BM_LogMailTracking, 
					BmString("Top-folder <") << eref.name << "," << nref.node 
						<< "> found");
		mTopFolder = AddMailFolder( eref, nref.node, NULL, mtime);

		// now we process all subfolders of the top-folder recursively:
#ifdef BM_LOGGING
		int folderCount = 1 + doInitializeMailFolders( mTopFolder.Get(), 1);
#else
		doInitializeMailFolders( mTopFolder.Get(), 1);
#endif
		BM_LOG( BM_LogMailTracking, 
				  BmString("End of initFolders (") << folderCount 
				  		<< " folders found)");
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInitializeMailFolders( BmMailFolder* folder, 
															  int level) {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count, folderCount = 0;
	BmString mailDirName;

	// we create a BDirectory from the given mail-folder...
	mailDirName = folder->Name();
	mailDir.SetTo( folder->EntryRefPtr());
	if ((err = mailDir.InitCheck()) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("Could not access \nmail-dir <") << mailDirName 
				<< "> \n\nError:" << strerror(err)
		);

	// ...and scan through all its entries for other mail-folders:
	while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs
			if ((err = mailDir.GetStatFor( dent->d_name, &st)) != B_OK)
				BM_THROW_RUNTIME( 
					BmString("Could not get stat-info for \nmail-dir <") 
						<< dent->d_name << "> \n\nError:" << strerror(err)
				);
			if (S_ISDIR( st.st_mode)) {
				// we have found a new mail-folder, so we add it as a child
				// of the current folder:
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				BM_LOG3( BM_LogMailTracking, 
							BmString("Mail-folder <") << dent->d_name << "," 
								<< dent->d_ino << "> found at level " << level);
				BmMailFolder* newFolder = AddMailFolder( 
					eref, dent->d_ino, folder, st.st_mtime
				);
				folderCount++;
				// now we process the new sub-folder first:
				folderCount += doInitializeMailFolders( newFolder, level+1);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	return folderCount;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	(re-)creates top mail-folder from given archive and proceeds with all
			subfolders recursively
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InstantiateItems( BMessage* archive) {
	status_t err;
	BM_LOG( BM_LogMailTracking, BmString("Starting to read folder-cache"));
	BMessage msg;
	if ((err = archive->FindMessage( 
		BmListModelItem::MSG_CHILDREN, &msg
	)) != B_OK)
		BM_THROW_RUNTIME( BmString("BmMailFolderList: Could not find msg-field <") 
									<< BmListModelItem::MSG_CHILDREN 
									<< "> \n\nError:" << strerror(err));
	{	// scope for autolock
		int32 folderCount = 1;
		BmAutolockCheckGlobal lock( mModelLocker);
		if (!lock.IsLocked())
			BM_THROW_RUNTIME( 
				ModelNameNC() << ":InstantiateMailFolders(): Unable to get lock"
			);
		mTopFolder = new BmMailFolder( &msg, this, NULL);
		BNode node( ThePrefs->GetString( "MailboxPath").String());
		node_ref nref;
		node.GetNodeRef( &nref);
		if (mTopFolder->NodeRef().node != nref.node) {
			// mailbox-folder has changed since last session, we start afresh:
			InitializeItems();
			return;
		}		
		AddItemToList( mTopFolder.Get());
		BM_LOG3( BM_LogMailTracking, 
					BmString("Top-folder <") << mTopFolder->EntryRef().name << "," 
						<< mTopFolder->Key() << "> read");
		if (mTopFolder->CheckIfModifiedSinceLastTime()) {
			folderCount += doInitializeMailFolders( mTopFolder.Get(), 1);
		} else {
			folderCount += doInstantiateMailFolders( mTopFolder.Get(), &msg, 1);
		}
		BM_LOG( BM_LogMailTracking, 
				  BmString("End of reading folder-cache (") << folderCount 
				  		<< " folders found)");
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInstantiateMailFolders( archive)
		-	recursively (re-)creates all mail-folders from given archive
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInstantiateMailFolders( BmMailFolder* folder, 
																BMessage* archive,
																int level) {
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmMailFolder::MSG_NUMCHILDREN);
	int32 folderCount = numChildren;
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		if ((err = archive->FindMessage( 
			BmMailFolder::MSG_CHILDREN, i, &msg
		)) != B_OK)
			BM_THROW_RUNTIME( BmString("Could not find mailfolder-child nr. ") 
										<< i+1 << " \n\nError:" << strerror(err));
		BmMailFolder* newFolder = new BmMailFolder( &msg, this, folder);
		AddItemToList( newFolder, folder);
		BM_LOG3( BM_LogMailTracking, 
					BmString("Mail-folder <") << newFolder->EntryRef().name 
						<< "," << newFolder->Key() << "> read");
		if (newFolder->CheckIfModifiedSinceLastTime()) {
			folderCount += doInitializeMailFolders( newFolder, level+1);
		} else {
			folderCount += doInstantiateMailFolders( newFolder, &msg, level+1);
		}
	}
	return folderCount;
}

/*------------------------------------------------------------------------------*\
	QueryForNewMails()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::QueryForNewMails() {
	int32 count, newCount=0;
	status_t err;
	dirent* dent;
	node_ref pnref;
	node_ref nref;
	char buf[4096];

	typedef set<BmMailFolder*> BmFolderSet;
	BmFolderSet foldersWithNewMail;

	BmAutolockCheckGlobal lock( mModelLocker);
	if (!lock.IsLocked())
		BM_THROW_RUNTIME( 
			ModelNameNC() << ":QueryForNewMails(): Unable to get lock"
		);

	BM_LOG( BM_LogMailTracking, "Start of newMail-query");
	if ((err = mNewMailQuery.SetVolume( &ThePrefs->MailboxVolume)) != B_OK)
		BM_THROW_RUNTIME( BmString("SetVolume(): ") << strerror(err));
	if ((err = mNewMailQuery.SetPredicate( "MAIL:status == 'New'")) != B_OK)
		BM_THROW_RUNTIME( BmString("SetPredicate(): ") << strerror(err));
	if ((err = mNewMailQuery.SetTarget( BMessenger( TheMailMonitor))) != B_OK)
		BM_THROW_RUNTIME( 
			BmString("QueryForNewMails(): could not set query target.\n\nError:") 
				<< strerror(err)
		);
	if ((err = mNewMailQuery.Fetch()) != B_OK)
		BM_THROW_RUNTIME( BmString("Fetch(): ") << strerror(err));
	Freeze();
	while ((count = mNewMailQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			newCount++;
			pnref.device = dent->d_pdev;
			pnref.node = dent->d_pino;
			nref.device = dent->d_dev;
			nref.node = dent->d_ino;
			
			foldersWithNewMail.insert( AddNewFlag( pnref, nref));
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	Thaw();
	BmFolderSet::const_iterator iter;
	for(	iter=foldersWithNewMail.begin(); 
			iter!=foldersWithNewMail.end(); ++iter) {
		if (*iter)
			TellModelItemUpdated( *iter, BmMailFolder::UPD_NEW_STATUS);
	}
	BM_LOG( BM_LogMailTracking, 
			  BmString("End of newMail-query (") << newCount 
			  		<< " new mails found)");
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	stores the current state inside cache-file
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveController( BmController* controller) {
	inherited::RemoveController( controller);
	Store();
	if (mMailboxPathHasChanged) {
		// the user has selected a new mailbox, we remove all cache-files:
		BEntry folderCache( SettingsFileName().String());
		folderCache.Remove();
		// remove mailref-caches:
		BDirectory* mailCacheDir = TheResources->MailCacheFolder();
		mailCacheDir->Rewind();
		BEntry mailCache;
		while( mailCacheDir->GetNextEntry( &mailCache) == B_OK) {
			mailCache.Remove();
		}
		// remove state-info caches for mailref- & folder-listview:
		BDirectory* stateCacheDir = TheResources->StateInfoFolder();
		stateCacheDir->Rewind();
		BEntry stateCache;
		while( stateCacheDir->GetNextEntry( &stateCache) == B_OK) {
			char name[B_FILE_NAME_LENGTH+1];
			if (stateCache.GetName( name) == B_OK) {
				if (strncmp( name, "MailFolderView_", 15)==0 
				|| strncmp( name, "MailRefView_", 12)==0)
					stateCache.Remove();
			}
		}
	}
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BmString BmMailFolderList::SettingsFileName() {
	return BmString( TheResources->SettingsPath.Path()) << "/" << "Folder Cache";
}
