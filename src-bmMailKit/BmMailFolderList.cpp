/*
	BmMailFolderList.cpp
		$Id$
*/

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <Query.h>

#include "BmLogHandler.h"
#include "BmMailFolderList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"


BmRef< BmMailFolderList> BmMailFolderList::theInstance;

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
	,	mCurrFolder( NULL)
	,	mInitCheck( B_NO_INIT)
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
	// try to open folder-cache file...
	status_t err;
	BFile cacheFile;

	if (InitCheck() == B_OK) {
		return true;
	}

	try {
	
		BString filename = BString(TheResources->SettingsPath.Path()) 
									<< "/" << ARCHIVE_FILENAME;
	
		if ((err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
			// ...ok, folder-cache found, we fetch our data from it:
			BMessage archive;
			(err = archive.Unflatten( &cacheFile)) == B_OK
														|| BM_THROW_RUNTIME( BString("Could not fetch folder-cache from file\n\t<") << filename << ">\n\n Result: " << strerror(err));
			InstantiateMailFolders( &archive);
		} else {
			// ...no cache file found, we fetch the existing mail-folders by hand...
			InitializeMailFolders();
		}
		QueryForNewMails();
		(err = InitCheck()) == B_OK
														|| BM_THROW_RUNTIME( BString("Could not initialize mailfolder-list"));
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	return InitCheck() == B_OK;
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailFolderList::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (ret == B_OK) {
		if (mCurrFolder) {
			ret = archive->AddString( MSG_CURRFOLDER, mCurrFolder->Name().String());
		} else {
			ret = archive->AddString( MSG_CURRFOLDER, "");
		}
	}
	if (deep && mTopFolder) {
		BMessage msg;
		ret = mTopFolder->Archive( &msg, deep)
				|| archive->AddMessage( MSG_TOPFOLDER, &msg);
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	InitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InitializeMailFolders() {
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
		mTopFolder = new BmMailFolder( eref, nref.node, NULL, mtime);
		mModelItemMap[mTopFolder->Key()] = mTopFolder;

		// now we process all subfolders of the top-folder recursively:
		int numDirs = 1 + doInitializeMailFolders( mTopFolder, 1);
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
			eref.device = dent->d_pdev;
			eref.directory = dent->d_pino;
			eref.set_name( dent->d_name);
			mailDir.GetStatFor( dent->d_name, &st) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get stat-info for \nmail-dir <") << dent->d_name << "> \n\nError:" << strerror(err));
			if (S_ISDIR( st.st_mode)) {
				// we have found a new mail-folder, so we add it as a child of the current folder:
				BM_LOG3( BM_LogMailTracking, BString("Mail-folder <") << dent->d_name << "," << dent->d_ino << "> found at level " << level);
				BmMailFolder* nextFolder = new BmMailFolder( eref, dent->d_ino, folder, st.st_mtime);
				dirCount++;
				// now we process the new sub-folder first:
				dirCount += doInitializeMailFolders( nextFolder, level+1);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	return dirCount;
}

/*------------------------------------------------------------------------------*\
	InstantiateMailFolders( archive)
		-	(re-)creates top mail-folder from given archive and proceeds with all
			subfolders recursively
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InstantiateMailFolders( BMessage* archive) {
	status_t err;
	BM_LOG2( BM_LogMailTracking, BString("Starting to read folder-cache"));
	BString currFolder = FindMsgString( archive, MSG_CURRFOLDER);
	BMessage msg;
	(err = archive->FindMessage( MSG_TOPFOLDER, &msg)) == B_OK
												|| BM_THROW_RUNTIME(BString("BmMailFolderList: Could not find msg-field <") << MSG_TOPFOLDER << "> \n\nError:" << strerror(err));
	{
		BAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":InstantiateMailFolders(): Unable to get lock");
		mTopFolder = new BmMailFolder( &msg, NULL);
		mModelItemMap[mTopFolder->Key()] = mTopFolder;
		BM_LOG3( BM_LogMailTracking, BString("Top-folder <") << mTopFolder->EntryRef().name << "," << mTopFolder->Key() << "> read");
		if (mTopFolder->NeedsCacheUpdate()) {
			doInitializeMailFolders( mTopFolder, 1);
		} else {
			doInstantiateMailFolders( mTopFolder, &msg, 1);
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
	inheritedModel::RemoveController( controller);
	Store();
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores FolderList inside Settings-dir:
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	if (mInitCheck != B_OK) return true;
	try {
		BAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":Store(): Unable to get lock");
		BString filename = BString( TheResources->SettingsPath.Path()) << "/" << ARCHIVE_FILENAME;
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive BmFolderList-object");
		(err = cacheFile.SetTo( filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create folder-cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store folder-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}
