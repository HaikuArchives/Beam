/*
	BmMailFolderList.h
		$Id$
*/

#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include <sys/stat.h>

#include <Locker.h>
#include <Looper.h>
#include <Query.h>

#include "BmDataModel.h"
#include "BmMailFolder.h"

/*------------------------------------------------------------------------------*\*\
	BmNodeMonitor
		-	class 
\*------------------------------------------------------------------------------*/
class BmNodeMonitor : public BLooper {
	typedef BLooper inherited;

public:
	// creator-func and c'tor:
	static BmNodeMonitor* CreateInstance();
	BmNodeMonitor();

	//	native methods:
	void HandleNodeMonitorMsg( BMessage* msg);
	void HandleQueryUpdateMsg( BMessage* msg);

	// overrides of looper base:
	void MessageReceived( BMessage* msg);

	static BmNodeMonitor* theInstance;
	
private:
	BmMailFolder* parent;
	ino_t lastParentInode;
	BmListModelItemRef lastParentRef;
	BmMailFolder* oldParent;
	ino_t lastOldParentInode;
	BmListModelItemRef lastOldParentRef;
	int32 counter;

	// Hide copy-constructor and assignment:
	BmNodeMonitor( const BmNodeMonitor&);
	BmNodeMonitor operator=( const BmNodeMonitor&);
};

/*------------------------------------------------------------------------------*\*\
	BmMailFolderList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolderList : public BmListModel {
	typedef BmListModel inherited;
	typedef map< ino_t, BmMailFolder*> BmNewNodeMap;

	friend class BmMailFolder;
	friend BmNodeMonitor;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME = 	"bm:mboxmtime";

public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderList* CreateInstance();
	BmMailFolderList();
	virtual ~BmMailFolderList();
	
	// native methods:
	void AddNewFlag( ino_t pnode, ino_t node);
	void RemoveNewFlag( ino_t pnode, ino_t node);
	void SetFolderForNodeFlaggedNew( ino_t node, BmMailFolder* folder);
	BmMailFolder* GetFolderForNodeFlaggedNew( ino_t node);
	bool NodeIsFlaggedNew( ino_t node);
	
	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BString SettingsFileName();

	static BmRef< BmMailFolderList> theInstance;
	
private:
	// native methods:
	BmMailFolder* AddMailFolder( entry_ref& eref, int64 node, BmMailFolder* parent, 
										  time_t mtime);
	void InitializeItems();
	int doInitializeMailFolders( BmMailFolder* folder, int level);
	void InstantiateItems( BMessage* archive);
	void doInstantiateMailFolders( BmMailFolder* folder, BMessage* archive, int level);
	void QueryForNewMails();

	// the following members will be archived as part of BmFolderList:
	BmRef<BmMailFolder> mTopFolder;
	
	// the following members will NOT be archived at all:
	BQuery mNewMailQuery;
	BmNewNodeMap mNewMailNodeMap;

	// Hide copy-constructor and assignment:
	BmMailFolderList( const BmMailFolderList&);
	BmMailFolderList operator=( const BmMailFolderList&);
};

#define TheMailFolderList BmMailFolderList::theInstance

#define TheNodeMonitor BmNodeMonitor::theInstance

#endif
