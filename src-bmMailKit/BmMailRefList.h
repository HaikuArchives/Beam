/*
	BmMailRefList.h
		$Id$
*/

#ifndef _BmMailRefList_h
#define _BmMailRefList_h

#include <sys/stat.h>

#include <Locker.h>

#include "BmDataModel.h"
#include "BmMailRef.h"

class BmMailFolder;

/*------------------------------------------------------------------------------*\*\
	BmMailRefList
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailRefList : public BArchivable, public BmListModel {
	typedef BArchivable inherited;
	typedef BmListModel inheritedModel;

	// archival-fieldnames:
	static const char* const MSG_MAILCOUNT		= 	"bm:mailcount";

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder, bool updateCache);
	virtual ~BmMailRefList();

	void RemoveController( BmController* controller);

	//	
	void StartJob();

	// archival stuff:
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	status_t InitCheck()						{ return mInitCheck; }

	//
	bool Store();

private:
	// the following members will be archived as part of BmFolderList:
	int32 mMailCount;

	// the following members will NOT be archived at all:
	BmMailFolder* mFolder;
	status_t mInitCheck;
	bool mUpdateCache;

	//
	void InitializeMailRefs();
	//
	void InstantiateMailRefs( BMessage* archive);
	//
	void Cleanup();
};


#endif
