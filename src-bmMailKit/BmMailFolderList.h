/*
 * Copyright 2002-2006, project beam (http://sourceforge.net/projects/beam).
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 *
 * Authors:
 *		Oliver Tappe <beam@hirschkaefer.de>
 */

#ifndef _BmMailFolderList_h
#define _BmMailFolderList_h

#include "BmMailKit.h"

#include <sys/stat.h>

#include <map>

#include <Locker.h>
#include <Looper.h>
#include <Query.h>

#include "BmDataModel.h"
#include "BmMailFolder.h"

class BmMailMonitor;
class BmMailRef;
/*------------------------------------------------------------------------------*\
	BmMailFolderList
		-	class 
\*------------------------------------------------------------------------------*/
class IMPEXPBMMAILKIT BmMailFolderList : public BmListModel {
	typedef BmListModel inherited;

	// archival-fieldnames:
	static const char* const MSG_MAILBOXMTIME;
	static const int16 nArchiveVersion;

public:
	// creator-func, c'tors and d'tor:
	static BmMailFolderList* CreateInstance();
	BmMailFolderList();
	virtual ~BmMailFolderList();
	
	// native methods:
	BmMailFolder* AddSpecialFlag( const node_ref& pnref, const node_ref& nref);
	void RemoveSpecialFlag( const node_ref& pnref, const node_ref& nref);
	void QueryForSpecialMails();
	BmRef<BmMailRef> FindMailRefByKey( const node_ref& nref);
	BmRef<BmMailFolder> FindMailFolderBySubPath( const BmString& subPath);
	//
	void InitializeItems();
	int InitializeSubFolders( BmMailFolder* folder, int level);
	void InstantiateItems( BMessage* archive);
	int InstantiateSubFolders( BmMailFolder* folder, BMessage* archive, 
										int level);
	//
	BmMailFolder* AddMailFolder( entry_ref& eref, int64 node, 
										  BmMailFolder* parent, time_t mtime);
	
	// overrides of list-model base:
	bool StartJob();
	void RemoveController( BmController* controller);
	const BmString SettingsFileName();

	// setters:
	void MailboxPathHasChanged( bool b) { mMailboxPathHasChanged = b; }
	
	// getters:
	BmMailFolder* TopFolder() const		{ return mTopFolder.Get(); }

	static BmRef< BmMailFolderList> theInstance;
	
private:
	// native methods:

	// overrides of listmodel base:
	int16 ArchiveVersion() const			{ return nArchiveVersion; }

	// the following members will be archived as part of BmFolderList:
	BmRef<BmMailFolder> mTopFolder;
	
	// the following members will NOT be archived at all:
	BQuery mSpecialMailQuery;
	bool mMailboxPathHasChanged;

	// Hide copy-constructor and assignment:
	BmMailFolderList( const BmMailFolderList&);
	BmMailFolderList operator=( const BmMailFolderList&);
};

#define TheMailFolderList BmMailFolderList::theInstance

#endif
