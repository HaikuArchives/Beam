/*
	BmMailRefList.h
		$Id$
*/

#ifndef _BmMailRefList_h
#define _BmMailRefList_h

#include <sys/stat.h>

#include <Locker.h>

#include "BmDataModel.h"

class BmMailFolder;
class BmMailRef;

/*------------------------------------------------------------------------------*\*\
	BmMailRefList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailRefList : public BmListModel {
	typedef BmListModel inherited;

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder, bool updateCache);
	virtual ~BmMailRefList();

	// native methods:
	BmMailRef* AddMailRef( entry_ref& eref, int64 node, struct stat& st);

	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BString SettingsFileName();

private:

	// the following members will NOT be archived at all:
	BmRef<BmMailFolder> mFolder;
	bool mUpdateCache;

	//
	void InitializeItems();
	//
	void InstantiateItems( BMessage* archive);
	//
	void Cleanup();
};


#endif
