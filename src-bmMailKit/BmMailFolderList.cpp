/*
	BmMailFolderList.cpp
		$Id$
*/

#include "BmMailFolderList.h"

/*------------------------------------------------------------------------------*\
	Init()
		-	class-method that initializes and returns the single folderlist-instance
\*------------------------------------------------------------------------------*/
BmMailFolderList *BmMailFolderList::Init() {
	// try to open folder-cache file...
	BmMailFolderList *theList = NULL;
	status_t err;
	BFile cacheFile;

	BString filename = BString(Beam::Prefs->mgPrefsPath.Path()) << "/" << ARCHIVE_FILENAME;

	if ((err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
		// ...ok, folder-cache found, we fetch our data from it:
		BMessage archive;
		(err = archive.Unflatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not fetch folder-cache from file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		theList = new BmMailFolderList( &archive);
	} else {
		// ...no cache file, we fetch the existing mail-folders by hand...
		theList = new BmMailFolderList();
	}
	(err = theList->InitCheck()) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not initialize mailfolder-list\n\n Result: ") << strerror(err));
	return theList;
}

/*------------------------------------------------------------------------------*\
	BmMailFolderList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::BmMailFolderList() 
:	mTopFolder( NULL)
,	mCurrFolder( NULL)
,	mInitCheck( B_NO_INIT)
{
	try {
		InitializeMailFolders();
		QueryForNewMails();
		mInitCheck = B_OK;
	} catch (exception &e) {
		ShowAlert( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	BmMailFolderList( archive)
		-	unarchive c'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::BmMailFolderList( BMessage *archive)
:	mTopFolder( NULL)
,	mCurrFolder( NULL)
,	mInitCheck( B_NO_INIT)
{
	try {
		status_t err;
		BM_LOG2( BM_LogMailFolders, BString("Starting to read folder-cache"));
		BString currFolder = FindMsgString( archive, MSG_CURRFOLDER);
		BMessage msg;
		(err = archive->FindMessage( MSG_TOPFOLDER, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("BmMailFolderList: Could not find msg-field <") << MSG_TOPFOLDER << "> \n\nError:" << strerror(err));
		mTopFolder = new BmMailFolder( &msg, NULL);
		mFolderMap[mTopFolder->ID()] = mTopFolder;
		BM_LOG2( BM_LogMailFolders, BString("Top-folder <") << mTopFolder->EntryRef().name << "," << mTopFolder->ID() << "> read");
		if (mTopFolder->CheckIfModifiedSince()) {
			doInitializeMailFolders( mTopFolder, 1);
		} else {
			doInstantiateMailFolders( mTopFolder, &msg, 1);
		}
		BM_LOG2( BM_LogMailFolders, BString("End of reading folder-cache (") << mFolderMap.size() << " folders found)");

		QueryForNewMails();
		mInitCheck = B_OK;
	} catch (exception &e) {
		ShowAlert( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	~BmMailFolderList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailFolderList::~BmMailFolderList() {
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
	Instantiate( archive)
		-	(re-)creates a MailFolderList from a given BMessage
\*------------------------------------------------------------------------------*/
BArchivable* BmMailFolderList::Instantiate( BMessage *archive) {
	if (!validate_instantiation( archive, "BmMailFolderList"))
		return NULL;
	return new BmMailFolderList( archive);
}

/*------------------------------------------------------------------------------*\
	InitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
void BmMailFolderList::InitializeMailFolders() {
	BDirectory mailDir;
	BString mailDirName;
	entry_ref eref;
	status_t err;

	BM_LOG2( BM_LogMailFolders, "Start of initFolders");

	// We start by finding the mailbox-dir:
	mailDirName = Beam::HomePath << "/mail_beam";		// TODO: change this to '/mail'
	mailDir.SetTo( mailDirName.String());

	time_t mtime;
	(err = mailDir.GetModificationTime( &mtime)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	BEntry entry;
	(err = mailDir.GetEntry( &entry)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get entry-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	entry.GetRef( &eref);
	node_ref nref;
	(err = mailDir.GetNodeRef( &nref)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get node-ref \nfor mailbox-dir <") << mailDirName << "> \n\nError:" << strerror(err));
	
	BM_LOG2( BM_LogMailFolders, BString("Top-folder <") << eref.name << "," << nref.node << "> found");
	mTopFolder = new BmMailFolder( eref, nref.node, NULL, mtime);
	mFolderMap[mTopFolder->ID()] = mTopFolder;

	// now we process all subfolders of the top-folder recursively:
	int numDirs = 1 + doInitializeMailFolders( mTopFolder, 1);
	BM_LOG2( BM_LogMailFolders, BString("End of initFolders (") << numDirs << " folders found)");
}

/*------------------------------------------------------------------------------*\
	doInitializeMailFolders()
		-	
\*------------------------------------------------------------------------------*/
int BmMailFolderList::doInitializeMailFolders( BmMailFolder *folder, int level) {
	BDirectory mailDir;
	entry_ref eref;
	dirent *dent;
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
	while ((count = mailDir.GetNextDirents((dirent *)buf, 4096)) > 0) {
		dent = (dirent *)buf;
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
				BM_LOG2( BM_LogMailFolders, BString("Mail-folder <") << dent->d_name << "," << dent->d_ino << "> found at level " << level);
				BmMailFolder *nextFolder = new BmMailFolder( eref, dent->d_ino, folder, st.st_mtime);
				mFolderMap[nextFolder->ID()] = nextFolder;
				dirCount++;
				// now we process the new sub-folder first:
				dirCount += doInitializeMailFolders( nextFolder, level+1);
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent *)((char *)dent + dent->d_reclen);
		}
	}
	return dirCount;
}

/*------------------------------------------------------------------------------*\
	doInstantiateMailFolders( archive)
		-	recursively (re-)creates all mail-folders from given archive
\*------------------------------------------------------------------------------*/
void BmMailFolderList::doInstantiateMailFolders( BmMailFolder *folder, BMessage *archive,
																 int level) {
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmMailFolder::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmMailFolder::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find mailfolder-child nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmMailFolder *newFolder = new BmMailFolder( &msg, folder);
		BM_LOG2( BM_LogMailFolders, BString("Mail-folder <") << newFolder->EntryRef().name << "," << newFolder->ID() << "> read");
		mFolderMap[newFolder->ID()] = newFolder;
		if (newFolder->CheckIfModifiedSince()) {
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
	dirent *dent;
	char buf[4096];
	BmFolderMap::iterator pos;

	BM_LOG2( BM_LogMailFolders, "Start of newMail-query");
	(err = query.SetVolume( &Beam::MailboxVolume)) == B_OK
													|| BM_THROW_RUNTIME( BString("SetVolume(): ") << strerror(err));
	(err = query.SetPredicate( "MAIL:status == 'New'")) == B_OK
													|| BM_THROW_RUNTIME( BString("SetPredicate(): ") << strerror(err));
	(err = query.Fetch()) == B_OK
													|| BM_THROW_RUNTIME( BString("Fetch(): ") << strerror(err));
	while ((count = query.GetNextDirents((dirent *)buf, 4096)) > 0) {
		dent = (dirent *)buf;
		while (count-- > 0) {
			newCount++;
			(pos = mFolderMap.find( dent->d_pino)) != mFolderMap.end()
													|| BM_THROW_RUNTIME( BString("Parent-node <") << dent->d_pino << "> not found for mail " << dent->d_name);
			pos->second->BumpNewMailCount();
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent *)((char *)dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogMailFolders, BString("End of newMail-query (") << newCount << " new mails found)");
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores FolderList inside Settings-dir:
\*------------------------------------------------------------------------------*/
bool BmMailFolderList::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BString filename = BString(Beam::Prefs->mgPrefsPath.Path()) << "/" << ARCHIVE_FILENAME;
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive BmFolderList-object");
		(err = cacheFile.SetTo( filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create folder-cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store folder-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		ShowAlert( e.what());
		return false;
	}
	return true;
}
