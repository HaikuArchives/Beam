/*
	BmMailRefList.cpp
		$Id$
*/

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <Query.h>

#include "BmApp.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmMailRefList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRefList::BmMailRefList( BmMailFolder* folder, bool updateCache)
	:	BmListModel( BString("MailRefList_") << folder->Key())
	,	mMailCount( 0)
	,	mFolder( folder)
	,	mInitCheck( B_NO_INIT)
	,	mUpdateCache( updateCache)
{
}

/*------------------------------------------------------------------------------*\
	~BmMailRefList()
		-	standard d'tor
\*------------------------------------------------------------------------------*/
BmMailRefList::~BmMailRefList() {
	Cleanup();
}

/*------------------------------------------------------------------------------*\
	Cleanup()
		-	frees occupied memory
\*------------------------------------------------------------------------------*/
void BmMailRefList::Cleanup() {
	BmModelItemMap::const_iterator iter;
	for( iter = begin(); iter != end(); ++iter) {
		delete iter->second;
	}
	mModelItemMap.clear();
	mInitCheck = B_NO_INIT;
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	deletes DataModel if it has no more controllers and if the list-caching
			is deactivated
\*------------------------------------------------------------------------------*/
void BmMailRefList::RemoveController( BmController* controller) {
	inheritedModel::RemoveController( controller);
	if (IsJobCompleted() && !HasControllers() && 0) {
// 0 -> activate mailreflist-caching
// 1 -> deactivate mailreflist-caching
// TODO: integrate List-caching into prefs
		mFolder->RemoveMailRefList();
	}
}

/*------------------------------------------------------------------------------*\
	Archive( archive)
		-	
\*------------------------------------------------------------------------------*/
status_t BmMailRefList::Archive( BMessage* archive, bool deep) const {
	status_t ret = inherited::Archive( archive, deep);
	if (ret == B_OK) {
		ret = archive->AddInt32( MSG_MAILCOUNT, mMailCount);
	}
	if (deep) {
		BMessage msg;
		BmModelItemMap::const_iterator iter;
		for( iter = begin(); iter != end() && ret == B_OK; ++iter) {
			ret = iter->second->Archive( &msg, deep);
		}
	}
	return ret;
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::StartJob() {
	// try to open folder-cache file...
	status_t err;
	BFile cacheFile;
	
	try {
		if (InitCheck() == B_OK) {
			return;
		}
	
		BString filename = BString(bmApp->SettingsPath.Path()) << "/mailcache_" << mFolder->Key();
	
		if (!mUpdateCache && (err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
			// ...ok, cache-file found, we fetch our data from it:
			BMessage archive;
			(err = archive.Unflatten( &cacheFile)) == B_OK
														|| BM_THROW_RUNTIME( BString("Could not fetch mail-cache from file\n\t<") << filename << ">\n\n Result: " << strerror(err));
			InstantiateMailRefs( &archive);
		} else {
			// ...no cache file found or update required, we fetch the existing mails by hand...
			InitializeMailRefs();
		}
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
}

/*------------------------------------------------------------------------------*\
	InitializeMailRefs()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InitializeMailRefs() {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count, refCount = 0;
	bool stopped = false;

	BM_LOG2( BM_LogMailFolders, BString("Start of initMailRefs for folder ") << mFolder->Name());

	// we create a BDirectory from the given mail-folder...
	mailDir.SetTo( mFolder->EntryRefPtr());

	// ...and scan through all its entries for mails:
	while (!stopped && (count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (!stopped && count-- > 0) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs

			if (!ShouldContinue()) {
				stopped = true;
			}
			
			mailDir.GetStatFor( dent->d_name, &st) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get stat-info for \nmail-dir <") << dent->d_name << "> \n\nError:" << strerror(err));
			if (S_ISREG( st.st_mode)) {
				// we have found a new mail, so we add it to our list:
				BM_LOG2( BM_LogMailFolders, BString("Mail <") << dent->d_name << "," << dent->d_ino << "> found ");
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				BmMailRef* nextMailRef = BmMailRef::CreateInstance( eref, dent->d_ino, st);
				if (nextMailRef) {
					mModelItemMap[nextMailRef->Key()] = nextMailRef;
					refCount++;
				}
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);
		}
	}
	BM_LOG2( BM_LogMailFolders, BString("End of initMailRefs for folder ") << mFolder->Name());
	if (stopped) {
		mMailCount = 0;
		Cleanup();
	} else {
		mMailCount = refCount;
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InstantiateMailRefs( BMessage* archive) {
/*
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmMailFolder::MSG_NUMCHILDREN);
	for( int i=0; i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmMailFolder::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find mailfolder-child nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmMailFolder* newFolder = new BmMailFolder( &msg, folder);
		BM_LOG2( BM_LogMailFolders, BString("Mail-folder <") << newFolder->EntryRef().name << "," << newFolder->Key() << "> read");
		mModelItemList[newFolder->Key()] = newFolder;
//		TellModelItemAdded( newFolder);
		if (newFolder->CheckIfModifiedSince()) {
			doInitializeMailFolders( newFolder, level+1);
		} else {
			doInstantiateMailFolders( newFolder, &msg, level+1);
		}
	}
	mInitCheck = B_OK;
*/
}

/*------------------------------------------------------------------------------*\
	Store()
		-	stores MailRefList inside Settings-dir:
\*------------------------------------------------------------------------------*/
bool BmMailRefList::Store() {
	BMessage archive;
	BFile cacheFile;
	status_t err;

	try {
		BAutolock lock( mModelLocker);
		lock.IsLocked() 						|| BM_THROW_RUNTIME( ModelName() << ":Store(): Unable to get lock");
		BString filename = BString( bmApp->SettingsPath.Path()) << "/mailcache_" << mFolder->Key();
		this->Archive( &archive, true) == B_OK
													|| BM_THROW_RUNTIME("Unable to archive BmMailRefList-object");
		(err = cacheFile.SetTo( filename.String(), 
										B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not create mail-cache file\n\t<") << filename << ">\n\n Result: " << strerror(err));
		(err = archive.Flatten( &cacheFile)) == B_OK
													|| BM_THROW_RUNTIME( BString("Could not store mail-cache into file\n\t<") << filename << ">\n\n Result: " << strerror(err));
	} catch( exception &e) {
		BM_SHOWERR( e.what());
		return false;
	}
	return true;
}

