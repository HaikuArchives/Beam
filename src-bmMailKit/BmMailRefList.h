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

	static const int16 nArchiveVersion = 1;

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder);
	virtual ~BmMailRefList();

	// native methods:
	BmMailRef* AddMailRef( entry_ref& eref, struct stat& st);
	void MarkCacheAsDirty()					{ mNeedsCacheUpdate = true; }

	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BString SettingsFileName();
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	
	// getters:
	bool NeedsCacheUpdate() const 		{ return mNeedsCacheUpdate; }

private:

	// native methods:
	void InitializeItems();
	void InstantiateItems( BMessage* archive);

	// the following members will NOT be archived at all:
	BmRef<BmMailFolder> mFolder;
	bool mNeedsCacheUpdate;

	// Hide copy-constructor and assignment:
	BmMailRefList( const BmMailRefList&);
	BmMailRefList operator=( const BmMailRefList&);
};


#endif
