/*
	BmMailFolderList.h
		$Id$
*/

#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include <sys/stat.h>

#include <Locker.h>

#include "BmDataModel.h"
#include "BmMailFolder.h"

/*------------------------------------------------------------------------------*\*\
	BmMailFolderList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolderList : public BArchivable, public BmListModel {
	typedef BArchivable inherited;
	typedef BmListModel inheritedModel;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME = 	"bm:mboxmtime";
	static const char* const MSG_CURRFOLDER = 	"bm:currfolder";
	static const char* const MSG_TOPFOLDER = 		"bm:topfolder";

	static const char* const ARCHIVE_FILENAME = 	"Folder Cache";

public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderList* CreateInstance();
	BmMailFolderList();
	virtual ~BmMailFolderList();
	
	//	native methods:
	bool Store();

	// overrides of datamodel base:
	status_t Archive( BMessage* archive, bool deep = true) const;
	bool StartJob();
	void RemoveController( BmController* controller);

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

	static BmRef< BmMailFolderList> theInstance;

private:
	// the following members will be archived as part of BmFolderList:
	BmMailFolder* mTopFolder;
	BmMailFolder* mCurrFolder;

	// the following members will NOT be archived at all:
	status_t mInitCheck;

	//
	void InitializeMailFolders();
	int doInitializeMailFolders( BmMailFolder* folder, int level);
	//
	void InstantiateMailFolders( BMessage* archive);
	void doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive, int level);
	//
	void QueryForNewMails();
};

#define TheMailFolderList BmMailFolderList::theInstance

#endif
