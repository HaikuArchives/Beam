/*
	BmMailFolderList.cpp
		$Id$
*/

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Query.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"


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
	AddMailFolder()
		-	
\*------------------------------------------------------------------------------*/
BmMailFolder* BmMailFolderList::AddMailFolder( entry_ref& eref, int64 node, 
															  BmMailFolder* parent, time_t mtime) {
	BmMailFolder* newFolder = new BmMailFolder( eref, node, parent, mtime);
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
	BString mailDirName( ThePrefs->MailboxPath());
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
		BAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":InitializeMailFolders(): Unable to get lock");
		BM_LOG3( BM_LogMailTracking, BString("Top-folder <") << eref.name << "," << nref.node << "> found");
		mTopFolder = AddMailFolder( eref, nref.node, NULL, mtime);
		mModelItemMap[mTopFolder->Key()] = mTopFolder.Get();

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
		BAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":InstantiateMailFolders(): Unable to get lock");
		mTopFolder = new BmMailFolder( &msg, NULL);
		mModelItemMap[mTopFolder->Key()] = mTopFolder.Get();
		BM_LOG3( BM_LogMailTracking, BString("Top-folder <") << mTopFolder->EntryRef().name << "," << mTopFolder->Key() << "> read");
		if (mTopFolder->NeedsCacheUpdate()) {
			doInitializeMailFolders( mTopFolder.Get(), 1);
		} else {
			doInstantiateMailFolders( mTopFolder.Get(), &msg, 1);
		}
		BM_LOG2( BM_LogMailTracking, BString("End of reading folder-cache (") << mModelItemMap.size() << " folders found)");
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
		BmMailFolder* newFolder = new BmMailFolder( &msg, folder);
		folder->AddSubItem( newFolder);
		BM_LOG3( BM_LogMailTracking, BString("Mail-folder <") << newFolder->EntryRef().name << "," << newFolder->Key() << "> read");
		if (newFolder->NeedsCacheUpdate()) {
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
	BQuery query;
	int32 count, newCount=0;
	status_t err;
	dirent* dent;
	char buf[4096];

	BM_LOG2( BM_LogMailTracking, "Start of newMail-query");
	(err = query.SetVolume( &TheResources->MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BString("SetVolume(): ") << strerror(err));
	(err = query.SetPredicate( "MAIL:status == 'New'")) == B_OK
													|| BM_THROW_RUNTIME( BString("SetPredicate(): ") << strerror(err));
	(err = query.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BString("Fetch(): ") << strerror(err));
	while ((count = query.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (count-- > 0) {
			newCount++;
			BmListModelItem *parent;
			if (!mTopFolder || !(parent=mTopFolder->FindItemByKey( BString()<<dent->d_pino))) {
				// we don't know the parent folder of this one, maybe it lives some other place (like trash):
				entry_ref eref;
				BEntry entry;
				BPath path;
				BString dirPath;

				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				(err = entry.SetTo( &eref)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create entry for unread mail <") << dent->d_name << ">\n\nError: "<< strerror(err));
				entry.GetPath( &path) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not get path for unread mail <") << dent->d_name << ">\n\nError: "<< strerror(err));
				dirPath = path.Path();
				BString mboxPath = ThePrefs->MailboxPath();
				mboxPath << "/";
				if (dirPath.FindFirst( mboxPath) != 0) {
					// mail lives somewhere else (not under /boot/home/mail), we ignore it:
					BM_LOG2( BM_LogMailTracking, BString("Mail ") << dent->d_name << " ignored because it doesn't live under our mailbox-folder");
				} else {
					// oops, we should really be knowing the parent folder, but we don't. Bark:
					throw BM_runtime_error( BString("QueryForNewMails(): Parent node ") << dent->d_pino << " not found for unread mail\n<" << dent->d_name << ">");
				}
			} else {
				BAutolock lock( mModelLocker);
				lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":QueryForNewMails(): Unable to get lock");
				BmMailFolder* folder = dynamic_cast<BmMailFolder*>( parent);
				folder->BumpNewMailCount();
			}
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

/*------------------------------------------------------------------------------*\
	HandleNodeMonitorMsg()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::HandleNodeMonitorMsg( BMessage* msg) {
	int32 opcode = msg->FindInt32( "opcode");
	entry_ref eref;
	const char *name;
	BEntry entry;
	struct stat st;
	status_t err;
	ino_t node;
	static int counter=0;
	BM_LOG2( BM_LogMailTracking, BString("NodeMonitorMessage nr.") << ++counter << " received.");
	BmMailFolder* parent = NULL;
	BmMailFolder* oldParent = NULL;
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
					(err = msg->FindInt64( directory, &node)) == B_OK
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
				// try to find new parent dir in our own structure:
				parent = dynamic_cast<BmMailFolder*>( FindItemByKey(BString()<<eref.directory));

				if (opcode == B_ENTRY_CREATED) {
					if (!parent)
						BM_THROW_RUNTIME( BString("Folder with inode <") << eref.directory << "> is unknown.");
					if (entry.IsDirectory()) {
						// a new mail-folder has been created, we add it to our list:
						BM_LOG2( BM_LogMailTracking, BString("New mail-folder <") << eref.name << "," << node << "> detected.");
						AddMailFolder( eref, node, parent, st.st_mtime);
					} else {
						BM_LOG2( BM_LogMailTracking, BString("New mail <") << eref.name << "," << node << "> detected.");
						parent->AddMailRef( eref, node, st);
					}
				} else if (opcode == B_ENTRY_REMOVED) {
					// we have no entry that could tell us what kind of item was removed,
					// so we have to find out by ourselves:
					BmListModelItem* item;
					item = FindItemByKey(BString()<<node);
					if (item) {
						// a folder has been deleted, we remove it from our list:
						BM_LOG2( BM_LogMailTracking, BString("Removal of mail-folder <") << node << "> detected.");
						RemoveItemFromList( item);
					} else {
						// a mail has been deleted, we remove it from the parent-folder:
						BM_LOG2( BM_LogMailTracking, BString("Removal of mail <") << node << "> detected.");
						if (parent)
							parent->RemoveMailRef( BString()<<node);
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
					oldParent = dynamic_cast<BmMailFolder*>( FindItemByKey(BString()<<erefFrom.directory));

					if (entry.IsDirectory()) {
						// it's a mail-folder, we check for type of change:
						folder = dynamic_cast<BmMailFolder*>( FindItemByKey(BString()<<node));
						if (erefFrom.directory == eref.directory) {
							// rename only, we take the short path:
							BM_LOG2( BM_LogMailTracking, BString("Rename of mail-folder <") << eref.name << "," << node << "> detected.");
							folder->EntryRef( eref);
							TellModelItemUpdated( folder, UPD_NAME);
						} else {
							if (oldParent && folder && oldParent != folder->Parent())
								return;			// folder not there anymore (e.g. in 2nd msg for move?)
							if (!folder) {
								// folder was unknown before, probably because it has been moved from
								// a place outside the /boot/home/mail substructure inside it.
								// We create the new folder...
								folder = AddMailFolder( eref, node, parent, st.st_mtime);
								// ...and scan for potential sub-folders:
								doInitializeMailFolders( folder, 1);
								BM_LOG2( BM_LogMailTracking, BString("Move of mail-folder <") << eref.name << "," << node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
							} else {
								// folder exists in our structure
								BM_LOG2( BM_LogMailTracking, BString("Move of mail-folder <") << eref.name << "," << node << "> detected.\nFrom: "<<(oldParent?oldParent->Key():"<outside>")<<" to: "<<(parent?parent->Key():"<outside>"));
								// we have to keep a reference to our folder around, otherwise the
								// folder might just disappear in RemoveItemFromList():
								BmRef<BmListModelItem> folderRef( folder);
								RemoveItemFromList( folder);
								if (parent) {
									// new position is still underneath /boot/home/mail, we simply move the folder:
									folder->EntryRef( eref);
									AddItemToList( folder, parent);
								}
							}
						}
					} else {
						// it's a mail-ref, we simply remove it from old parent and add it
						// to its new parent:
						BM_LOG2( BM_LogMailTracking, BString("Move of mail <") << eref.name << "," << node << "> detected.");
						if (oldParent)
							oldParent->RemoveMailRef( BString()<<node);
						if (parent && !parent->HasMailRef( BString()<<node))
							parent->AddMailRef( eref, node, st);
					}
				}
				break;
			}
		}
	} catch( exception &e) {
		BM_SHOWERR( e.what());
	}
}
