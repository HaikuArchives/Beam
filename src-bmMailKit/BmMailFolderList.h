/*
	BmMailFolderList.h
		$Id$
*/

#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include <sys/stat.h>

#include <Locker.h>

#include "BmMailFolder.h"

// message constants for BmMailFolderList, all msgs are sent to the looper 
// specified via Init()-function:
#define BM_FOLDER_ADD			'bmfa'
#define BM_FOLDER_REMOVE		'bmfb'

/*------------------------------------------------------------------------------*\
	BmFolderListInfo
		-	this structure represents a BmMailFolderList's connection to the 
			outer world.
\*------------------------------------------------------------------------------*/
struct BmFolderListInfo {
	BLooper* statusLooper;
							// the looper that should receive status-messages from a BmPopper.
							// In Beam, this is the BmConnectionWin-looper. OPTIONAL
	bool (*aliveFunc)();
							// a bool-function that returns true as long as the BmPopper
							// should continue to run. OPTIONAL

	BmFolderListInfo( BLooper* sl, bool (*f)()) 
			: statusLooper(sl)
			, aliveFunc(f)
			{}
};

/*------------------------------------------------------------------------------*\*\
	BmMailFolderList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolderList : public BArchivable {
	typedef BArchivable inherited;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME = 	"bm:mboxmtime";
	static const char* const MSG_CURRFOLDER = 	"bm:currfolder";
	static const char* const MSG_TOPFOLDER = 		"bm:topfolder";

	static const char* const ARCHIVE_FILENAME = 	"Folder Cache";

public:
	//	message component definitions for status-msgs:
	static const char* const MSG_NAME = 		"bm:fname";
	static const char* const MSG_INODE = 		"bm:inode";
	static const char* const MSG_PNODE = 		"bm:pnode";

	// class-method initializer:
	static BmMailFolderList* CreateInstance( BmFolderListInfo* info);

	BmMailFolderList( BmFolderListInfo* info);
	BmMailFolderList( BmFolderListInfo* info, BMessage* archive);
	virtual ~BmMailFolderList();
	
	// archival stuff:
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

	//
	bool Store();

private:
	// the following members will be archived as part of BmFolderList:
	BmFolderMap mFolderMap;
	BmMailFolder* mTopFolder;
	BmMailFolder* mCurrFolder;

	// the following members will NOT be archived at all:
	status_t mInitCheck;
	BmFolderListInfo* mFolderListInfo;
	BLocker mLocker;							// lock used to secure traversal of folder-list

	//
	void InitializeMailFolders();
	int doInitializeMailFolders( BmMailFolder* folder, int level);
	//
	void InstantiateMailFolders( BMessage* archive);
	void doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive, int level);
	//
	void QueryForNewMails();
	//
	void TellAboutAddedFolder( BmMailFolder*);
	void TellAboutRemovedFolder( BmMailFolder*);
	//
	bool ShouldContinue();
};


#endif
