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
	void HandleNodeMonitorMsg( BMessage* msg);
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
	BmRef<BmMailFolder> mTopFolder;

	// the following members will NOT be archived at all:
	status_t mInitCheck;

	//
	BmMailFolder* AddMailFolder( entry_ref& eref, int64 node, BmMailFolder* parent, 
										  time_t mtime);
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
