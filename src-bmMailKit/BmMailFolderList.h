/*
	BmMailFolderList.h
		$Id$
*/

#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include <sys/stat.h>

#include "BmMailFolder.h"

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
	BmMailFolderList();
	BmMailFolderList( BMessage *archive);
	virtual ~BmMailFolderList();
	
	// archival stuff:
	static BArchivable *Instantiate( BMessage *archive);
	virtual status_t Archive( BMessage *archive, bool deep = true) const;

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

	//
	bool Store();

	// class-method initializer:
	static BmMailFolderList *Init();

private:
	// the following members will be archived as part of BmFolderList:
	BmFolderMap mFolderMap;
	BmMailFolder *mTopFolder;
	BmMailFolder *mCurrFolder;

	// the following members will NOT be archived at all:
	status_t mInitCheck;

	//
	void InitializeMailFolders();
	int doInitializeMailFolders( BmMailFolder *folder, int level);
	void doInstantiateMailFolders( BmMailFolder *folder, BMessage *archive, int level);
	//
	void QueryForNewMails();
};


#endif
