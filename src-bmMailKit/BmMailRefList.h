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
class BmMailRefList : public BArchivable, public BmListModel {
	typedef BArchivable inherited;
	typedef BmListModel inheritedModel;

	// archival-fieldnames:
	static const char* const MSG_NUMCHILDREN = 	"bm:nChl";
	static const char* const MSG_CHILDREN = 		"bm:chld";

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder, bool updateCache);
	virtual ~BmMailRefList();

	// native methods:
	BmMailRef* AddMailRef( entry_ref& eref, int64 node, struct stat& st);
	bool Store();

	// overrides of archivable base:
	virtual status_t Archive( BMessage* archive, bool deep = true) const;

	//	overrides of datamodel base:
	bool StartJob();
	void RemoveController( BmController* controller);

	// getters:
	status_t InitCheck()						{ return mInitCheck; }
	BString CacheFileName();

private:

	// the following members will NOT be archived at all:
	BmRef<BmMailFolder> mFolder;
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
