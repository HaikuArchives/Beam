/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailRefList_h
#define _BmMailRefList_h

#include "BmMailKit.h"

#include <sys/stat.h>

#include "BmDataModel.h"

class BmMailFolder;
class BmMailRef;

/*------------------------------------------------------------------------------*\
	BmMailRefList
		-	class 
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailRefList : public BmListModel {
	typedef BmListModel inherited;

	static const int16 nArchiveVersion;

public:

	// c'tors and d'tor
	BmMailRefList( BmMailFolder* folder);
	virtual ~BmMailRefList();

	// native methods:
	BmRef<BmMailRef> AddMailRef( entry_ref& eref, struct stat& st);
	BmRef<BmListModelItem> RemoveMailRef( const BmString& key);
	void UpdateMailRef( const BmString& key);
	void MarkCacheAsDirty();
	void StoreAndCleanup();

	// overrides of list-model base:
	bool Store();
	bool StartJob();
	void RemoveController( BmController* controller);
	bool IsJobCompleted() const;
	const BmString SettingsFileName();
	int16 ArchiveVersion() const			{ return nArchiveVersion; }
	bool AddItemToList( BmListModelItem* item, 
							  BmListModelItem* parent=NULL);
	void RemoveItemFromList( BmListModelItem* item);
	void SetItemValidity(  BmListModelItem* item, bool isValid);
	void ExecuteAction( BMessage* action);
	
	// getters:
	inline bool NeedsCacheUpdate() const
													{ return mNeedsCacheUpdate; }


protected:

	// native methods:
	void InitializeItems();
	void InstantiateItemsFromStream( BDataIO* dataIO, BMessage* headerMsg = NULL);

private:

	// the following members will NOT be archived at all:
	BmWeakRef<BmMailFolder> mFolder;
	bool mNeedsCacheUpdate;
	BmString mSettingsFileName;

	// Hide copy-constructor and assignment:
	BmMailRefList( const BmMailRefList&);
	BmMailRefList operator=( const BmMailRefList&);
};

#endif
