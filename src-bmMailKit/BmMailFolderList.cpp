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
	,	parent( NULL)
	,	lastParentRef( NULL)
	,	oldParent( NULL)
	,	lastOldParentRef( NULL)
	,	counter( 0)
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
	catch( exception &err) {
		// a problem occurred, we tell the user:
		BM_SHOWERR( BmString("MailMonitor: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleMailMonitorMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmMailMonitor::HandleMailMonitorMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	entry_ref eref;
	const char *name;
	struct stat st;
	status_t err;
	node_ref nref;
	BM_LOG2( BM_LogMailTracking, BmString("MailMonitorMessage nr.") << ++counter << " received.");
	BmMailFolder* folder = NULL;
	try {
		switch( opcode) {
			case B_ENTRY_CREATED: 
			case B_ENTRY_REMOVED:
			case B_ENTRY_MOVED: {
				const char* directory = opcode == B_ENTRY_MOVED ? "to directory" : "directory";
				(err = msg->FindInt64( directory, &eref.directory)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field '")<<directory<<"' not found in msg !?!");
				(err = msg->FindInt32( "device", &eref.device)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field 'device' not found in msg !?!"));
				nref.device = eref.device;
				(err = msg->FindInt64( "node", &nref.node)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field 'node' not found in msg !?!"));
				if (opcode != B_ENTRY_REMOVED) {
					BNode aNode;
					BNodeInfo nodeInfo;
					(err = msg->FindString( "name", &name)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field 'name' not found in msg !?!"));
					eref.set_name( name);
					(err = aNode.SetTo( &eref)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Couldn't create node for parent-node <")<<eref.directory<<"> and name <"<<eref.name << "> \n\nError:" << strerror(err));
					(err = nodeInfo.SetTo( &aNode)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Couldn't create node-info for node --- parent-node <")<<eref.directory<<"> and name <"<<eref.name << "> \n\nError:" << strerror(err));
					(err = aNode.GetStat( &st)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Couldn't get stats for node --- parent-node <")<<eref.directory<<"> and name <"<<eref.name << "> \n\nError:" << strerror(err));
				}
				{	// new scope for lock:
					BmAutolock lock( TheMailFolderList->mModelLocker);
					lock.IsLocked() 			|| BM_THROW_RUNTIME( "HandleMailMonitorMsg(): Unable to get lock");
					// check if the parent dir is the same as it was for the last message:
					if (!lastParentRef || lastParentNodeRef.node != eref.directory
					 || lastParentNodeRef.device != eref.device) {
						// try to find new parent dir in our own structure:
						lastParentNodeRef.node = eref.directory;
						lastParentNodeRef.device = eref.device;
						lastParentRef = TheMailFolderList->FindItemByKey( BM_REFKEY( lastParentNodeRef));
						parent = dynamic_cast<BmMailFolder*>( lastParentRef.Get());
					}
	
					if (opcode == B_ENTRY_CREATED) {
						if (!parent)
							BM_THROW_RUNTIME( BmString("Folder with inode <") << eref.directory << "> is unknown.");
						if (S_ISDIR(st.st_mode)) {
							// a new mail-folder has been created, we add it to our list:
							BM_LOG2( BM_LogMailTracking, BmString("New mail-folder <") << eref.name << "," << nref.node << "> detected.");
							TheMailFolderList->AddMailFolder( eref, nref.node, parent, st.st_mtime);
						} else {
							// a new mail has been created, we add it to the parent folder:
							BM_LOG2( BM_LogMailTracking, BmString("New mail <") << eref.name << "," << nref.node << "> detected.");
							parent->AddMailRef( eref, st);
						}
					} else if (opcode == B_ENTRY_REMOVED) {
						// we have no entry that could tell us what kind of item was removed,
						// so we have to find out by ourselves:
						BmListModelItem* item = NULL;
						if (parent)
							item = parent->FindItemByKey( BM_REFKEY( nref));
						if (item) {
							// a folder has been deleted, we remove it from our list:
							BM_LOG2( BM_LogMailTracking, BmString("Removal of mail-folder <") << nref.node << "> detected.");
							TheMailFolderList->RemoveItemFromList( item);
						} else {
							// a mail has been deleted, we remove it from the parent-folder:
							BM_LOG2( BM_LogMailTracking, BmString("Removal of mail <") << nref.node << "> detected.");
							if (parent)
								parent->RemoveMailRef( nref);
						}
					} else if (opcode == B_ENTRY_MOVED) {
						entry_ref erefFrom;
						(err = msg->FindInt64( "from directory", &erefFrom.directory)) == B_OK
														|| BM_THROW_RUNTIME( BmString("Field 'directory' not found in msg !?!"));
						(err = msg->FindInt32( "device", &erefFrom.device)) == B_OK
														|| BM_THROW_RUNTIME( BmString("Field 'device' not found in msg !?!"));
						(err = msg->FindString( "name", &name)) == B_OK
														|| BM_THROW_RUNTIME( BmString("Field 'name' not found in msg !?!"));
						erefFrom.set_name( name);
						// check if the old-parent dir is the same as it was for the last message:
						if (!lastOldParentRef || lastOldParentNodeRef.node != erefFrom.directory
						|| lastOldParentNodeRef.device != erefFrom.device) {
							// try to find new old-parent dir in our own structure:
							lastOldParentNodeRef.node = erefFrom.directory;
							lastOldParentNodeRef.device = erefFrom.device;
							lastOldParentRef = TheMailFolderList->FindItemByKey( BM_REFKEY( lastOldParentNodeRef));
							oldParent = dynamic_cast<BmMailFolder*>( lastOldParentRef.Get());
						}
						if (S_ISDIR(st.st_mode)) {
							// it's a mail-folder, we check for type of change:
							if (oldParent)
								folder = dynamic_cast<BmMailFolder*>( oldParent->FindItemByKey( BM_REFKEY( nref)));
							else
								folder = dynamic_cast<BmMailFolder*>( TheMailFolderList->FindItemByKey( BM_REFKEY( nref)).Get());
							if (erefFrom.directory == eref.directory) {
								// rename only, we take the short path:
								BM_LOG2( BM_LogMailTracking, BmString("Rename of mail-folder <") << eref.name << "," << nref.node << "> detected.");
								folder->EntryRef( eref);
								TheMailFolderList->TellModelItemUpdated( folder, UPD_KEY);
							} else {
								// the folder has really changed position within filesystem-tree:
								if (oldParent && folder && folder->Parent() != oldParent)
									return;			// folder not there anymore (e.g. in 2nd msg for move?)
								if (!folder) {
									// folder was unknown before, probably because it has been moved from
									// a place outside the /boot/home/mail substructure inside it.
									// We create the new folder...
									folder = TheMailFolderList->AddMailFolder( eref, nref.node, parent, st.st_mtime);
									// ...and scan for potential sub-folders:
									TheMailFolderList->doInitializeMailFolders( folder, 1);
									BM_LOG2( BM_LogMailTracking, BmString("Move of mail-folder <") << eref.name << "," << nref.node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
								} else {
									// folder exists in our structure
									BM_LOG2( BM_LogMailTracking, BmString("Move of mail-folder <") << eref.name << "," << nref.node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
									// we have to keep a reference to our folder around, otherwise the
									// folder might just disappear in RemoveItemFromList():
									BmRef<BmListModelItem> folderRef( folder);
									TheMailFolderList->RemoveItemFromList( folder);
									if (parent) {
										// new position is still underneath /boot/home/mail, we simply move the folder:
										folder->EntryRef( eref);
										TheMailFolderList->AddItemToList( folder, parent);
									}
								}
							}
						} else {
							// it's a mail-ref, we remove it from old parent and add it
							// to its new parent:
							BM_LOG2( BM_LogMailTracking, BmString("Move of mail <") << eref.name << "," << nref.node << "> detected.");
							if (oldParent)
								oldParent->RemoveMailRef( nref);
							if (parent)
								parent->AddMailRef( eref, st);
						}
					}
				}
				break;
			}
		}
	} catch( exception &e) {
		BM_LOGERR( e.what());
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
	BM_LOG2( BM_LogMailTracking, BmString("QueryUpdateMessage nr.") << ++counter << " received.");
	try {
		switch( opcode) {
			case B_ENTRY_CREATED: {
				(err = msg->FindInt64( "directory", &pnref.node)) == B_OK
													|| BM_THROW_RUNTIME( "Field 'directory' not found in msg !?!");
				(err = msg->FindInt32( "device", &nref.device)) == B_OK
													|| BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				(err = msg->FindInt64( "node", &nref.node)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field 'node' not found in msg !?!"));
				pnref.device = nref.device;
				TheMailFolderList->AddNewFlag( pnref, nref);
				break;
			}
			case B_ENTRY_REMOVED: {
				(err = msg->FindInt64( "directory", &pnref.node)) == B_OK
													|| BM_THROW_RUNTIME( "Field 'directory' not found in msg !?!");
				(err = msg->FindInt32( "device", &nref.device)) == B_OK
													|| BM_THROW_RUNTIME( "Field 'device' not found in msg !?!");
				(err = msg->FindInt64( "node", &nref.node)) == B_OK
													|| BM_THROW_RUNTIME( BmString("Field 'node' not found in msg !?!"));
				pnref.device = nref.device;
				TheMailFolderList->RemoveNewFlag( pnref, nref);
				break;
			}
		}
	} catch( exception &e) {
		BM_LOGERR( e.what());
	}
}



/********************************************************************************\
	BmMailFolderList
\********************************************************************************/

BmRef< BmMailFolderList> BmMailFolderList::theInstance( NULL);

const char* const BmMailFolderList::MSG_MAILBOXMTIME = "bm:mboxmtime";
const int16 BmMailFolderList::nArchiveVersion = 1;

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
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::StartJob() {
	if (inherited::StartJob()) {
		QueryForNewMails();
		return true;
	} else
		return false;
}

/*------------------------------------------------------------------------------*\
	AddNewFlag()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddNewFlag( const node_ref& pnref, const node_ref& nref) {
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
void BmMailFolderList::RemoveNewFlag( const node_ref& pnref, const node_ref& nref) {
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
void BmMailFolderList::SetFolderForNodeFlaggedNew( const node_ref& nref, BmMailFolder* folder) {
	BmMailFolder* oldFolder = mNewMailNodeMap[ BM_REFKEY( nref)];
	if (oldFolder != folder) {
		mNewMailNodeMap[ BM_REFKEY( nref)] = folder;
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
BmMailFolder* BmMailFolderList::GetFolderForNodeFlaggedNew( const node_ref& nref) {
	return mNewMailNodeMap[ BM_REFKEY( nref)];
}

/*------------------------------------------------------------------------------*\
	NodeIsFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::NodeIsFlaggedNew( const node_ref& nref) {
	return mNewMailNodeMap.find( BM_REFKEY( nref)) != mNewMailNodeMap.end();
}

/*------------------------------------------------------------------------------*\
	AddMailFolder()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddMailFolder( entry_ref& eref, int64 node, 
															  BmMailFolder* parent, time_t mtime) {
	BmMailFolder* newFolder = new BmMailFolder( this, eref, node, parent, mtime);
	AddItemToList( newFolder, parent);
	return newFolder;
}

/*------------------------------------------------------------------------------*\
	InitializeItems()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InitializeItems() {
	BDirectory mailDir;
	entry_ref eref;
	status_t err;
	BmString mailDirName( ThePrefs->GetString("MailboxPath"));
	time_t mtime;

	BM_LOG2( BM_LogMailTracking, "Start of initFolders");

	mailDir.SetTo( mailDirName.String());
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not get mtime \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	BEntry entry;
	(err = mailDir.GetEntry( &entry)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not get entry-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	entry.GetRef( &eref);
	node_ref nref;
	(err = mailDir.GetNodeRef( &nref)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not get node-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));

	{	
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelNameNC() << ":InitializeMailFolders(): Unable to get lock");
		BM_LOG3( BM_LogMailTracking, BmString("Top-folder <") << eref.name << "," << nref.node << "> found");
		mTopFolder = AddMailFolder( eref, nref.node, NULL, mtime);

		// now we process all subfolders of the top-folder recursively:
#ifdef BM_LOGGING
		int numDirs = 1 + doInitializeMailFolders( mTopFolder.Get(), 1);
#else
		doInitializeMailFolders( mTopFolder.Get(), 1);
#endif
		BM_LOG2( BM_LogMailTracking, BmString("End of initFolders (") << numDirs << " folders found)");
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInitializeMailFolders( BmMailFolder* folder, int level) {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count, dirCount = 0;
	BmString mailDirName;

	// we create a BDirectory from the given mail-folder...
	mailDirName = folder->Name();
	mailDir.SetTo( folder->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not access \nmail-dir <") << mailDirName << "> \n\nError:" << strerror(err));

	// ...and scan through all its entries for other mail-folders:
	while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs
			mailDir.GetStatFor( dent->d_name, &st) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not get stat-info for \nmail-dir <") << dent->d_name << "> \n\nError:" << strerror(err));
			if (S_ISDIR( st.st_mode)) {
				// we have found a new mail-folder, so we add it as a child of the current folder:
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				BM_LOG3( BM_LogMailTracking, BmString("Mail-folder <") << dent->d_name << "," << dent->d_ino << "> found at level " << level);
				BmMailFolder* newFolder = AddMailFolder( eref, dent->d_ino, folder, st.st_mtime);
				dirCount++;
				// now we process the new sub-folder first:
				dirCount += doInitializeMailFolders( newFolder, level+1);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	return dirCount;
}

/*------------------------------------------------------------------------------*\
	InstantiateItems( archive)
		-	(re-)creates top mail-folder from given archive and proceeds with all
			subfolders recursively
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InstantiateItems( BMessage* archive) {
	status_t err;
	BM_LOG2( BM_LogMailTracking, BmString("Starting to read folder-cache"));
	BMessage msg;
	(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, &msg)) == B_OK
												|| BM_THROW_RUNTIME(BmString("BmMailFolderList: Could not find msg-field <") << BmListModelItem::MSG_CHILDREN << "> \n\nError:" << strerror(err));
	{
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelNameNC() << ":InstantiateMailFolders(): Unable to get lock");
		mTopFolder = new BmMailFolder( &msg, this, NULL);
		AddItemToList( mTopFolder.Get());
		BM_LOG3( BM_LogMailTracking, BmString("Top-folder <") << mTopFolder->EntryRef().name << "," << mTopFolder->Key() << "> read");
		if (mTopFolder->CheckIfModifiedSinceLastTime()) {
			doInitializeMailFolders( mTopFolder.Get(), 1);
		} else {
			doInstantiateMailFolders( mTopFolder.Get(), &msg, 1);
		}
		BM_LOG2( BM_LogMailTracking, BmString("End of reading folder-cache (") << size() << " folders found)");
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	doInstantiateMailFolders( archive)
		-	recursively (re-)creates all mail-folders from given archive
\*------------------------------------------------------------------------------*/
void BmMailFolderList::doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive,
																 int level) {
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmMailFolder::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmMailFolder::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BmString("Could not find mailfolder-child nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmMailFolder* newFolder = new BmMailFolder( &msg, this, folder);
		AddItemToList( newFolder, folder);
		BM_LOG3( BM_LogMailTracking, BmString("Mail-folder <") << newFolder->EntryRef().name << "," << newFolder->Key() << "> read");
		if (newFolder->CheckIfModifiedSinceLastTime()) {
			doInitializeMailFolders( newFolder, level+1);
		} else {
			doInstantiateMailFolders( newFolder, &msg, level+1);
		}
	}
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

	BmAutolock lock( mModelLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelNameNC() << ":QueryForNewMails(): Unable to get lock");

	BM_LOG2( BM_LogMailTracking, "Start of newMail-query");
	(err = mNewMailQuery.SetVolume( &TheResources->MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BmString("SetVolume(): ") << strerror(err));
	(err = mNewMailQuery.SetPredicate( "MAIL:status == 'New'")) == B_OK
													|| BM_THROW_RUNTIME( BmString("SetPredicate(): ") << strerror(err));
	(err = mNewMailQuery.SetTarget( BMessenger( TheMailMonitor))) == B_OK
													|| BM_THROW_RUNTIME( BmString("QueryForNewMails(): could not set query target.\n\nError:") << strerror(err));
	(err = mNewMailQuery.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BmString("Fetch(): ") << strerror(err));
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
	for( iter=foldersWithNewMail.begin(); iter!=foldersWithNewMail.end(); ++iter) {
		if (*iter)
			TellModelItemUpdated( *iter, BmMailFolder::UPD_NEW_STATUS);
	}
	BM_LOG2( BM_LogMailTracking, BmString("End of newMail-query (") << newCount << " new mails found)");
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
