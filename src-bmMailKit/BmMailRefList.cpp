/*
	BmMailRefList.cpp
		$Id$
*/

#include <Autolock.h>
#include <Directory.h>
#include <File.h>
#include <Path.h>
#include <Query.h>

#include "BmBasics.h"
#include "BmLogHandler.h"
#include "BmMailFolder.h"
#include "BmMailRef.h"
#include "BmMailRefList.h"
#include "BmPrefs.h"
#include "BmResources.h"
#include "BmUtil.h"

/*------------------------------------------------------------------------------*\
	BmMailRefList()
		-	standard c'tor
\*------------------------------------------------------------------------------*/
BmMailRefList::BmMailRefList( BmMailFolder* folder, bool updateCache)
	:	BmListModel( BString("MailRefList_") << folder->Key() << " (" << folder->Name()<<")")
	,	mFolder( folder)
	,	mNeedsCacheUpdate( updateCache)
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
	CacheFileName()
		-	
\*------------------------------------------------------------------------------*/
const BString BmMailRefList::SettingsFileName() {
	return BString( TheResources->SettingsPath.Path()) 
				<< "/MailCache/" 
				<< "folder_" 
				<< mFolder->Key()
				<< " ("
				<< mFolder->Name()
				<< ")";
}

/*------------------------------------------------------------------------------*\
	StartJob()
		-	
\*------------------------------------------------------------------------------*/
bool BmMailRefList::StartJob() {
	// try to open folder-cache file...
	status_t err;
	BFile cacheFile;
	
	if (InitCheck() == B_OK) {
		return true;
	}

	Freeze();									// we shut up for better performance
	try {
		BString filename = SettingsFileName();
	
		bool cacheFileUpToDate = false;
		if (ThePrefs->GetBool("CacheRefsOnDisk")
		&& (err = cacheFile.SetTo( filename.String(), B_READ_ONLY)) == B_OK) {
			time_t mtime;
			(err = cacheFile.GetModificationTime( &mtime)) == B_OK
														|| BM_THROW_RUNTIME(BString("Could not get mtime \nfor mail-folder <") << Name() << "> \n\nError:" << strerror(err));
			if (!mNeedsCacheUpdate && !mFolder->CheckIfModifiedSince( mtime))
				cacheFileUpToDate = true;
		}
		if (cacheFileUpToDate) {
			// ...ok, cache-file should contain up-to-date info, we fetch our data from it:
			BMessage archive;
			(err = archive.Unflatten( &cacheFile)) == B_OK
														|| BM_THROW_RUNTIME( BString("Could not fetch mail-cache from file\n\t<") << filename << ">\n\n Result: " << strerror(err));
			InstantiateItems( &archive);
		} else {
			// ...caching disabled or no cache file found or update required, 
			// we fetch the existing mails from disk...
			InitializeItems();
		}
	} catch (exception &e) {
		BM_SHOWERR( e.what());
	}
	Thaw();
	return InitCheck() == B_OK;
}

/*------------------------------------------------------------------------------*\
	AddMailRef()
		-	
\*------------------------------------------------------------------------------*/
BmMailRef* BmMailRefList::AddMailRef( entry_ref& eref, int64 node, struct stat& st) {
	BmMailRef* newMailRef = BmMailRef::CreateInstance( this, eref, node, st);
	if (AddItemToList( newMailRef))
		return newMailRef;
	else
		return NULL;
}

/*------------------------------------------------------------------------------*\
	InitializeMailRefs()
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InitializeItems() {
	BDirectory mailDir;
	entry_ref eref;
	dirent* dent;
	struct stat st;
	status_t err;
	char buf[4096];
	int32 count, refCount = 0;
	bool stopped = false;

	BM_LOG2( BM_LogMailTracking, BString("Start of InitializeMailRefs() for folder ") << mFolder->Name());

	// we create a BDirectory from the given mail-folder...
	mailDir.SetTo( mFolder->EntryRefPtr());

	// ...and scan through all its entries for mails:
	while (!stopped && (count = mailDir.GetNextDirents((dirent* )buf, 4096)) > 0) {
		dent = (dirent* )buf;
		while (!stopped && count-- > 0) {
			if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
				continue;						// ignore . and .. dirs

			mailDir.GetStatFor( dent->d_name, &st) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not get stat-info for \nmail-dir <") << dent->d_name << "> \n\nError:" << strerror(err));
			if (S_ISREG( st.st_mode)) {
				// we have found a new mail, so we add it to our list:
				BM_LOG3( BM_LogMailTracking, BString("Mail <") << dent->d_name << "," << dent->d_ino << "> found ");
				eref.device = dent->d_pdev;
				eref.directory = dent->d_pino;
				eref.set_name( dent->d_name);
				if (AddMailRef( eref, dent->d_ino, st))
					refCount++;
			}
			// Bump the dirent-pointer by length of the dirent just handled:
			dent = (dirent* )((char* )dent + dent->d_reclen);

			if (!ShouldContinue()) {
				stopped = true;
				BM_LOG2( BM_LogMailTracking, BString("InitializeMailRefs() stopped for folder ") << mFolder->Name());
			}
		}
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InitializeMailRefs() for folder ") << mFolder->Name());
	if (stopped) {
		Cleanup();
	} else {
		mNeedsCacheUpdate = false;
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	InstantiateMailRefs( archive)
		-	
\*------------------------------------------------------------------------------*/
void BmMailRefList::InstantiateItems( BMessage* archive) {
	BM_LOG2( BM_LogMailTracking, BString("Start of InstantiateMailRefs() for folder ") << mFolder->Name());
	status_t err;
	int32 numChildren = FindMsgInt32( archive, BmListModelItem::MSG_NUMCHILDREN);
	bool stopped = false;
	for( int i=0; !stopped && i<numChildren; ++i) {
		BMessage msg;
		(err = archive->FindMessage( BmListModelItem::MSG_CHILDREN, i, &msg)) == B_OK
													|| BM_THROW_RUNTIME(BString("Could not find mailcache-entry nr. ") << i+1 << " \n\nError:" << strerror(err));
		BmMailRef* newRef = new BmMailRef( &msg, this);
		BM_LOG3( BM_LogMailTracking, BString("MailRef <") << newRef->TrackerName() << "," << newRef->Key() << "> read");
		AddItemToList( newRef);

		if (!ShouldContinue()) {
			stopped = true;
		}
	}
	BM_LOG2( BM_LogMailTracking, BString("End of InstantiateMailRefs() for folder ") << mFolder->Name());
	if (stopped) {
		Cleanup();
	} else {
		mNeedsCacheUpdate = false;
		mInitCheck = B_OK;
	}
}

/*------------------------------------------------------------------------------*\
	RemoveController()
		-	deletes DataModel if it has no more controllers and if the list-caching
			is deactivated
\*------------------------------------------------------------------------------*/
void BmMailRefList::RemoveController( BmController* controller) {
	if (ThePrefs->GetBool("CacheRefsOnDisk"))
		Store();
	inherited::RemoveController( controller);
	if (!(ThePrefs->GetBool("CacheRefsInMem") || HasControllers())) {
		mFolder->RemoveMailRefList();
	}
}
