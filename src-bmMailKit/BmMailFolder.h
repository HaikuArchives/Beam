/*
	BmMailFolder.h
		$Id$
*/

#ifndef _BmMailFolder_h
#define _BmMailFolder_h

#include <map>

#include <Archivable.h>
#include <Entry.h>
#include <Node.h>
#include <String.h>

#include "BmDataModel.h"

class BmMailFolder;
class BmMailFolderList;

class BmMailRefList;

typedef map<BString, BmMailFolder*> BmFolderMap;

/*------------------------------------------------------------------------------*\
	BmMailFolder
		-	class 
\*------------------------------------------------------------------------------*/
class BmMailFolder : public BmListModelItem {
	typedef BmListModelItem inherited;

public:
	BmMailFolder( BmMailFolderList* model, entry_ref &eref, ino_t node,
					  BmMailFolder* parent, time_t &modified);
	BmMailFolder( BMessage* archive, BmMailFolderList* model, BmMailFolder* parent);
	virtual ~BmMailFolder();
	
	// native methods:
	void BumpNewMailCount( int32 offset=1);
	void BumpNewMailCountForSubfolders( int32 offset=1);
	bool HasNewMail() const					{ return mNewMailCount>0 || mNewMailCountForSubfolders>0; }
	bool CheckIfModifiedSince( time_t when, time_t* storeNewModTime=NULL);
	void CreateMailRefList();
	void RemoveMailRefList();
	void MarkCacheAsDirty();
	void AddMailRef( entry_ref& eref, struct stat& st);
	bool HasMailRef( BString key);
	void RemoveMailRef( ino_t node);
	void CreateSubFolder( BString name);
	void Rename( BString newName);
	void MoveToTrash();

	// overrides of archivable base:
	static BArchivable* Instantiate( BMessage* archive);
	status_t Archive( BMessage* archive, bool deep = true) const;

	// getters:
	const entry_ref& EntryRef() const 	{ return mEntryRef; }
	const entry_ref* EntryRefPtr() const{ return &mEntryRef; }
	const ino_t& Inode() const				{ return mInode; }
	const int NewMailCount() const		{ return mNewMailCount; }
	const int NewMailCountForSubfolders() const		{ return mNewMailCountForSubfolders; }
	const time_t LastModified() const	{ return mLastModified; }
	const bool NeedsCacheUpdate() const	{ return mNeedsCacheUpdate; }
	BmMailFolder* Parent() 					{ return dynamic_cast<BmMailFolder*>( mParent); }
	BmMailRefList* MailRefList();
	const BString& Name() const			{ return mName; }

	// setters:
	void EntryRef( entry_ref &e) 			{ mEntryRef = e; mName = e.name; }

	// archival-fieldnames:
	static const char* const MSG_ENTRYREF = 		"bm:eref";
	static const char* const MSG_INODE = 			"bm:inod";
	static const char* const MSG_LASTMODIFIED = 	"bm:lmod";

	//	message component definitions for status-msgs:
	static const char* const MSG_NAME = 			"bm:fname";

	// flags indicating which parts are to be updated:
	static const BmUpdFlags UPD_NAME	= 1<<1;

private:
	void StartNodeMonitor();
	void StopNodeMonitor();

	// the following members will be archived as part of BmFolderList:
	entry_ref mEntryRef;
	ino_t mInode;
	time_t mLastModified;

	// the following members will be archived into their own files:
	BmRef< BmMailRefList> mMailRefList;

	// the following members will NOT be archived at all:
	bool mNeedsCacheUpdate;
	int mNewMailCount;
	int mNewMailCountForSubfolders;
	BString mName;

	// Hide copy-constructor and assignment:
	BmMailFolder( const BmMailFolder&);
	BmMailFolder operator=( const BmMailFolder&);
};

#endif
