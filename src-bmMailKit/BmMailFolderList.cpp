/*
	BmMailFolderList.cpp
		$Id$
*/

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <NodeMonitor.h>
#include <Path.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/********************************************************************************\
	BmNodeMonitor
\********************************************************************************/

BmNodeMonitor* BmNodeMonitor::theInstance = NULL;

/*------------------------------------------------------------------------------*\
	CreateInstance()
		-	creator-func
\*------------------------------------------------------------------------------*/
BmNodeMonitor* BmNodeMonitor::CreateInstance() {
	if (!theInstance)
		theInstance = new BmNodeMonitor();
	return theInstance;
}

/*------------------------------------------------------------------------------*\
	BmNodeMonitor()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmNodeMonitor::BmNodeMonitor()
	:	BLooper("NodeMonitor", B_DISPLAY_PRIORITY, 500)
	,	parent( NULL)
	,	lastParentInode( 0)
	,	lastParentRef( NULL)
	,	oldParent( NULL)
	,	lastOldParentInode( 0)
	,	lastOldParentRef( NULL)
	,	counter( 0)
{
	Run();
}

/*------------------------------------------------------------------------------*\
	MessageReceived( msg)
		-	
\*------------------------------------------------------------------------------*/
void BmNodeMonitor::MessageReceived( BMessage* msg) {
	try {
		switch( msg->what) {
			case B_NODE_MONITOR: {
				if (TheMailFolderList)
					HandleNodeMonitorMsg( msg);
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
		BM_SHOWERR( BString("NodeMonitor: ") << err.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleNodeMonitorMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmNodeMonitor::HandleNodeMonitorMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	entry_ref eref;
	const char *name;
	BEntry entry;
	struct stat st;
	status_t err;
	ino_t node;
	BM_LOG2( BM_LogMailTracking, BString("NodeMonitorMessage nr.") << ++counter << " received.");
	BmMailFolder* folder = NULL;
	try {
		switch( opcode) {
			case B_ENTRY_CREATED: 
			case B_ENTRY_REMOVED:
			case B_ENTRY_MOVED: {
				const char* directory = opcode == B_ENTRY_MOVED ? "to directory" : "directory";
				(err = msg->FindInt64( directory, &eref.directory)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field '")<<directory<<"' not found in msg !?!");
				(err = msg->FindInt32( "device", &eref.device)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'device' not found in msg !?!"));
				if (opcode == B_ENTRY_REMOVED) {
					(err = msg->FindInt64( "node", &node)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'node' not found in msg !?!"));
				} else {
					node_ref nref;
					(err = msg->FindString( "name", &name)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'name' not found in msg !?!"));
					eref.set_name( name);
					(err = entry.SetTo( &eref)) == B_OK
													|| BM_THROW_RUNTIME( BString("Couldn't create entry for parent-node <")<<eref.directory<<"> and name <"<<eref.name << "> \n\nError:" << strerror(err));
					(err = entry.GetStat( &st)) == B_OK
													|| BM_THROW_RUNTIME( BString("Couldn't get stat for entry.") << " \n\nError:" << strerror(err));
					(err = entry.GetNodeRef( &nref)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get node-ref for entry.") << "> \n\nError:" << strerror(err));
					node = nref.node;
				}
				{	// new scope for lock:
					BmAutolock lock( TheMailFolderList->mModelLocker);
					lock.IsLocked() 						|| BM_THROW_RUNTIME( "HandleNodeMonitorMsg(): Unable to get lock");
					// check if the parent dir is the same as it was for the last message:
					if (!lastParentRef || lastParentInode != eref.directory) {
						// try to find new parent dir in our own structure:
						lastParentRef = TheMailFolderList->FindItemByKey(BString()<<eref.directory);
						lastParentInode = eref.directory;
						parent = dynamic_cast<BmMailFolder*>( lastParentRef.Get());
					}
	
					if (opcode == B_ENTRY_CREATED) {
						if (!parent)
							BM_THROW_RUNTIME( BString("Folder with inode <") << eref.directory << "> is unknown.");
						if (S_ISDIR(st.st_mode)) {
							// a new mail-folder has been created, we add it to our list:
							BM_LOG2( BM_LogMailTracking, BString("New mail-folder <") << eref.name << "," << node << "> detected.");
							TheMailFolderList->AddMailFolder( eref, node, parent, st.st_mtime);
						} else {
							// a new mail has been created, we add it to the parent folder:
							BM_LOG2( BM_LogMailTracking, BString("New mail <") << eref.name << "," << node << "> detected.");
							parent->AddMailRef( eref, st);
						}
					} else if (opcode == B_ENTRY_REMOVED) {
						// we have no entry that could tell us what kind of item was removed,
						// so we have to find out by ourselves:
						BmListModelItem* item = NULL;
						if (parent)
							item = parent->FindItemByKey(BString()<<node);
						if (item) {
							// a folder has been deleted, we remove it from our list:
							BM_LOG2( BM_LogMailTracking, BString("Removal of mail-folder <") << node << "> detected.");
							TheMailFolderList->RemoveItemFromList( item);
						} else {
							// a mail has been deleted, we remove it from the parent-folder:
							BM_LOG2( BM_LogMailTracking, BString("Removal of mail <") << node << "> detected.");
							if (parent)
								parent->RemoveMailRef( node);
						}
					} else if (opcode == B_ENTRY_MOVED) {
						entry_ref erefFrom;
						(err = msg->FindInt64( "from directory", &erefFrom.directory)) == B_OK
														|| BM_THROW_RUNTIME( BString("Field 'directory' not found in msg !?!"));
						(err = msg->FindInt32( "device", &erefFrom.device)) == B_OK
														|| BM_THROW_RUNTIME( BString("Field 'device' not found in msg !?!"));
						(err = msg->FindString( "name", &name)) == B_OK
														|| BM_THROW_RUNTIME( BString("Field 'name' not found in msg !?!"));
						erefFrom.set_name( name);
						// check if the old-parent dir is the same as it was for the last message:
						if (!lastOldParentRef || lastOldParentInode != erefFrom.directory) {
							// try to find new old-parent dir in our own structure:
							lastOldParentRef = TheMailFolderList->FindItemByKey(BString()<<erefFrom.directory);
							lastOldParentInode = erefFrom.directory;
							oldParent = dynamic_cast<BmMailFolder*>( lastOldParentRef.Get());
						}
						if (S_ISDIR(st.st_mode)) {
							// it's a mail-folder, we check for type of change:
							if (oldParent)
								folder = dynamic_cast<BmMailFolder*>( oldParent->FindItemByKey(BString()<<node));
							else
								folder = dynamic_cast<BmMailFolder*>( TheMailFolderList->FindItemByKey(BString()<<node).Get());
							if (erefFrom.directory == eref.directory) {
								// rename only, we take the short path:
								BM_LOG2( BM_LogMailTracking, BString("Rename of mail-folder <") << eref.name << "," << node << "> detected.");
								folder->EntryRef( eref);
								TheMailFolderList->TellModelItemUpdated( folder, BmMailFolder::UPD_NAME);
							} else {
								// the folder has really changed position within filesystem-tree:
								if (oldParent && folder && folder->Parent() != oldParent)
									return;			// folder not there anymore (e.g. in 2nd msg for move?)
								if (!folder) {
									// folder was unknown before, probably because it has been moved from
									// a place outside the /boot/home/mail substructure inside it.
									// We create the new folder...
									folder = TheMailFolderList->AddMailFolder( eref, node, parent, st.st_mtime);
									// ...and scan for potential sub-folders:
									TheMailFolderList->doInitializeMailFolders( folder, 1);
									BM_LOG2( BM_LogMailTracking, BString("Move of mail-folder <") << eref.name << "," << node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
								} else {
									// folder exists in our structure
									BM_LOG2( BM_LogMailTracking, BString("Move of mail-folder <") << eref.name << "," << node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
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
							BM_LOG2( BM_LogMailTracking, BString("Move of mail <") << eref.name << "," << node << "> detected.");
							if (oldParent)
								oldParent->RemoveMailRef( node);
							if (parent)
								parent->AddMailRef( eref, st);
						}
					}
				}
				break;
			}
		}
	} catch( exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	HandleQueryUpdateMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmNodeMonitor::HandleQueryUpdateMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	status_t err;
	ino_t node;
	ino_t pnode;
	BM_LOG2( BM_LogMailTracking, BString("QueryUpdateMessage nr.") << ++counter << " received.");
	try {
		switch( opcode) {
			case B_ENTRY_CREATED: {
				(err = msg->FindInt64( "node", &node)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'node' not found in msg !?!"));
				(err = msg->FindInt64( "directory", &pnode)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'directory' not found in msg !?!"));
				TheMailFolderList->AddNewFlag( pnode, node);
				break;
			}
			case B_ENTRY_REMOVED: {
				(err = msg->FindInt64( "node", &node)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'node' not found in msg !?!"));
				(err = msg->FindInt64( "directory", &pnode)) == B_OK
													|| BM_THROW_RUNTIME( BString("Field 'directory' not found in msg !?!"));
				TheMailFolderList->RemoveNewFlag( pnode, node);
				break;
			}
		}
	} catch( exception &e) {
		BM_SHOWERR( e.what());
	}
}



/********************************************************************************\
	BmMailFolderList
\********************************************************************************/

BmRef< BmMailFolderList> BmMailFolderList::theInstance( NULL);

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
void BmMailFolderList::AddNewFlag( ino_t pnode, ino_t node) {
	BmRef<BmListModelItem> parentRef = FindItemByKey( BString()<<pnode);
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	mNewMailNodeMap[ node] = parent;
	if (parent)
		parent->BumpNewMailCount();
}

/*------------------------------------------------------------------------------*\
	RemoveNewFlag()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveNewFlag( ino_t pnode, ino_t node) {
	BmRef<BmListModelItem> parentRef = FindItemByKey( BString()<<pnode);
	mNewMailNodeMap.erase( node);
	BmMailFolder* parent = dynamic_cast< BmMailFolder*>( parentRef.Get());
	if (parent)
		parent->BumpNewMailCount( -1);
}

/*------------------------------------------------------------------------------*\
	SetFolderForNodeFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::SetFolderForNodeFlaggedNew( ino_t node, BmMailFolder* folder) {
	BmMailFolder* oldFolder = mNewMailNodeMap[ node];
	if (oldFolder != folder) {
		mNewMailNodeMap[ node] = folder;
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
BmMailFolder* BmMailFolderList::GetFolderForNodeFlaggedNew( ino_t node) {
	return mNewMailNodeMap[ node];
}

/*------------------------------------------------------------------------------*\
	NodeIsFlaggedNew()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::NodeIsFlaggedNew( ino_t node) {
	return mNewMailNodeMap.find( node) != mNewMailNodeMap.end();
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
	BString mailDirName( ThePrefs->GetString("MailboxPath"));
	time_t mtime;

	BM_LOG2( BM_LogMailTracking, "Start of initFolders");

	mailDir.SetTo( mailDirName.String());
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	BEntry entry;
	(err = mailDir.GetEntry( &entry)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get entry-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	entry.GetRef( &eref);
	node_ref nref;
	(err = mailDir.GetNodeRef( &nref)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get node-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));

	{	
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":InitializeMailFolders(): Unable to get lock");
		BM_LOG3( BM_LogMailTracking, BString("Top-folder <") << eref.name << "," << nref.node << "> found");
		mTopFolder = AddMailFolder( eref, nref.node, NULL, mtime);

		// now we process all subfolders of the top-folder recursively:
		int numDirs = 1 + doInitializeMailFolders( mTopFolder.Get(), 1);
		BM_LOG2( BM_LogMailTracking, BString("End of initFolders (") << numDirs << " folders found)");
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
	BString mailDirName;

	// we create a BDirectory from the given mail-folder...
	mailDirName = folder->Name();
	mailDir.SetTo( folder->EntryRefPtr());
	(err = mailDir.InitCheck()) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not access \nmail-dir <") << mailDirName << "> \n\nError:" << strerror(err));

	// ...and scan through all its entries for other mail-folders:
	while ((count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs
			mailDir.GetStatFor( dent->d_name, &st) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get stat-info for \nmail-dir <") << dent->d_name << "> \n\nError:" << strerror(err));
			if (S_ISDIR( st.st_mode)) {
				// we have found a new mail-folder, so we add it as a child of the current folder:
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				BM_LOG3( BM_LogMailTracking, BString("Mail-folder <") << dent->d_name << "," << dent->d_ino << "> found at level " << level);
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
	BM_LOG2( BM_LogMailTracking, BString("Starting to read folder-cache"));
	BMessage msg;
	(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, &msg)) == B_OK
												|| BM_THROW_RUNTIME(BString("BmMailFolderList: Could not find msg-field <") << BmListModelItem::MSG_CHILDREN << "> \n\nError:" << strerror(err));
	{
		BmAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":InstantiateMailFolders(): Unable to get lock");
		mTopFolder = new BmMailFolder( &msg, this, NULL);
		AddItemToList( mTopFolder.Get());
		BM_LOG3( BM_LogMailTracking, BString("Top-folder <") << mTopFolder->EntryRef().name << "," << mTopFolder->Key() << "> read");
		if (mTopFolder->CheckIfModifiedSinceLastTime()) {
			doInitializeMailFolders( mTopFolder.Get(), 1);
		} else {
			doInstantiateMailFolders( mTopFolder.Get(), &msg, 1);
		}
		BM_LOG2( BM_LogMailTracking, BString("End of reading folder-cache (") << size() << " folders found)");
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
													|| BM_THROW_RUNTIME(BString("Could not find mailfolder-child nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmMailFolder* newFolder = new BmMailFolder( &msg, this, folder);
		AddItemToList( newFolder, folder);
		BM_LOG3( BM_LogMailTracking, BString("Mail-folder <") << newFolder->EntryRef().name << "," << newFolder->Key() << "> read");
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
	char buf[4096];

	BmAutolock lock( mModelLocker);
	lock.IsLocked() 							|| BM_THROW_RUNTIME( ModelName() << ":QueryForNewMails(): Unable to get lock");

	BM_LOG2( BM_LogMailTracking, "Start of newMail-query");
	(err = mNewMailQuery.SetVolume( &TheResources->MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BString("SetVolume(): ") << strerror(err));
	(err = mNewMailQuery.SetPredicate( "MAIL:status == 'New'")) == B_OK
													|| BM_THROW_RUNTIME( BString("SetPredicate(): ") << strerror(err));
	(err = mNewMailQuery.SetTarget( BMessenger( TheNodeMonitor))) == B_OK
													|| BM_THROW_RUNTIME( BString("QueryForNewMails(): could not set query target.\n\nError:") << strerror(err));
	(err = mNewMailQuery.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BString("Fetch(): ") << strerror(err));
	while ((count = mNewMailQuery.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			newCount++;
			AddNewFlag( dent->d_pino, dent->d_ino);
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogMailTracking, BString("End of newMail-query (") << newCount << " new mails found)");
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	stores the current state inside cache-file
\*------------------------------------------------------------------------------*/
void BmMailFolderList::RemoveController( BmController* controller) {
	inherited::RemoveController( controller);
	Store();
}

/*------------------------------------------------------------------------------*\
	SettingsFileName()
		-	
\*------------------------------------------------------------------------------*/
const BString BmMailFolderList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) << "/" << "Folder Cache";
}
