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
class BmMailFolderList : public BmListModel {
	typedef BmListModel inherited;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME = 	"bm:mboxmtime";

public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderList* CreateInstance();
	BmMailFolderList();
	virtual ~BmMailFolderList();
	
	//	native methods:
	void HandleNodeMonitorMsg( BMessage* msg);

	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BString SettingsFileName();

	static BmRef< BmMailFolderList> theInstance;

private:
	// the following members will be archived as part of BmFolderList:
	BmRef<BmMailFolder> mTopFolder;

	//
	BmMailFolder* AddMailFolder( entry_ref& eref, int64 node, BmMailFolder* parent, 
										  time_t mtime);
	//
	void InitializeItems();
	int doInitializeMailFolders( BmMailFolder* folder, int level);
	//
	void InstantiateItems( BMessage* archive);
	void doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive, int level);
	//
	void QueryForNewMails();
};

#define TheMailFolderList BmMailFolderList::theInstance

#endif
